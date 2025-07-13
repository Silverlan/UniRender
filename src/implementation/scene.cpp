// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <optional>
#include <iostream>
#include <fsys/filesystem.h>
#include <fsys/ifile.hpp>
#include <sharedutils/datastream.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util_ifile.hpp>
#include <sharedutils/util.h>
#include <sharedutils/util_path.hpp>
#include <sharedutils/magic_enum.hpp>
#include <sharedutils/util_hash.hpp>
#include <sharedutils/util_log.hpp>
#include <util_image.hpp>
#include <util_image_buffer.hpp>
#include <util_texture_info.hpp>
#include <udm.hpp>
#include <random>
#include "interface/definitions.hpp"

#ifdef ENABLE_CYCLES_LOGGING
#pragma comment(lib, "shlwapi.lib")
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ccl happens to have the same include guard name as sharedutils, so we have to undef it here
#undef __UTIL_STRING_H__
#include <sharedutils/util_string.h>

module pragma.scenekit;

import pragma.ocio;

import :scene;
import :camera;
import :renderer;
import :constants;
import :light;
import :shader;
import :denoise;
import :model_cache;
import :object;
import :mesh;

#pragma clang optimize off
static std::shared_ptr<spdlog::logger> g_logger = nullptr;
void pragma::scenekit::set_logger(const std::shared_ptr<spdlog::logger> &logger) { g_logger = logger; }
const std::shared_ptr<spdlog::logger> &pragma::scenekit::get_logger() { return g_logger; }
bool pragma::scenekit::should_log() { return g_logger != nullptr; }

static std::function<void(bool)> g_kernelCompileCallback = nullptr;
void pragma::scenekit::set_kernel_compile_callback(const std::function<void(bool)> &f) { g_kernelCompileCallback = f; }
const std::function<void(bool)> &pragma::scenekit::get_kernel_compile_callback() { return g_kernelCompileCallback; }

pragma::scenekit::Scene::CreateInfo::CreateInfo() {}

void pragma::scenekit::Scene::CreateInfo::Serialize(udm::LinkedPropertyWrapper &data) const
{
	auto &udm = data;
	udm["renderer"] = renderer;
	if(samples.has_value())
		udm["samples"] = *samples;
	udm["hdrOutput"] = hdrOutput;
	udm["denoiseMode"] = udm::enum_to_string(denoiseMode);
	udm["progressive"] = progressive;
	udm["progressiveRefine"] = progressiveRefine;
	udm["deviceType"] = udm::enum_to_string(deviceType);
	udm["exposure"] = exposure;
	udm["preCalculateLight"] = preCalculateLight;

	if(colorTransform.has_value()) {
		auto udmColorTransform = udm["colorTransform"];
		udmColorTransform["config"] = colorTransform->config;
		if(colorTransform->lookName.has_value())
			udmColorTransform["lookName"] = *colorTransform->lookName;
	}
}
void pragma::scenekit::Scene::CreateInfo::Deserialize(udm::LinkedPropertyWrapper &data)
{
	auto &udm = data;
	udm["renderer"](renderer);
	if(udm["samples"]) {
		samples = uint32_t {};
		udm["samples"](*samples);
	}
	udm["hdrOutput"](hdrOutput);
	denoiseMode = udm::string_to_enum(udm["denoiseMode"], util::declvalue(&CreateInfo::denoiseMode));
	udm["progressive"](progressive);
	udm["progressiveRefine"](progressiveRefine);
	deviceType = udm::string_to_enum(udm["deviceType"], util::declvalue(&CreateInfo::deviceType));
	udm["exposure"](exposure);
	udm["preCalculateLight"](preCalculateLight);

	auto udmColorTransform = udm["colorTransform"];
	if(udmColorTransform) {
		colorTransform = ColorTransformInfo {};
		udmColorTransform["config"](colorTransform->config);
		auto udmLookName = udmColorTransform["lookName"];
		if(udmLookName) {
			colorTransform->lookName = std::string {};
			udmLookName(*colorTransform->lookName);
		}
	}
}

///////////////////

void pragma::scenekit::Scene::PrintLogInfo()
{
	auto &logHandler = pragma::scenekit::get_log_handler();
	if(logHandler == nullptr)
		return;
	std::stringstream ss;

	auto &sceneInfo = GetSceneInfo();
	ss << "Scene Info\n";
	ss << "Sky: " << sceneInfo.sky << "\n";
	ss << "Sky angles: " << sceneInfo.skyAngles << "\n";
	ss << "Sky strength: " << sceneInfo.skyStrength << "\n";
	ss << "Transparent sky: " << sceneInfo.transparentSky << "\n";
	ss << "Emission strength: " << sceneInfo.emissionStrength << "\n";
	ss << "Light intensity factor: " << sceneInfo.lightIntensityFactor << "\n";
	ss << "Motion blur strength: " << sceneInfo.motionBlurStrength << "\n";
	ss << "Max transparency bounces: " << sceneInfo.maxTransparencyBounces << "\n";
	ss << "Max bounces: " << sceneInfo.maxBounces << "\n";
	ss << "Max diffuse bounces: " << sceneInfo.maxDiffuseBounces << "\n";
	ss << "Max glossy bounces: " << sceneInfo.maxGlossyBounces << "\n";
	ss << "Max transmission bounces: " << sceneInfo.maxTransmissionBounces << "\n";
	ss << "Exposure: " << sceneInfo.exposure << "\n";
	logHandler(ss.str());

	ss = {};
	auto &createInfo = GetCreateInfo();
	ss << "Create Info\n";
	ss << "Renderer: " << createInfo.renderer << "\n";
	ss << "Samples: ";
	if(createInfo.samples.has_value())
		ss << *createInfo.samples;
	else
		ss << "-";
	ss << "\n";
	ss << "HDR output: " << createInfo.hdrOutput << "\n";
	ss << "Denoise mode: " << magic_enum::enum_name(createInfo.denoiseMode) << "\n";
	ss << "Progressive: " << createInfo.progressive << "\n";
	ss << "Progressive refine: " << createInfo.progressiveRefine << "\n";
	ss << "Device type: " << magic_enum::enum_name(createInfo.deviceType) << "\n";
	ss << "Exposure: " << createInfo.exposure << "\n";
	ss << "Color transform: ";
	if(createInfo.colorTransform.has_value()) {
		ss << createInfo.colorTransform->config;
		if(createInfo.colorTransform->lookName.has_value())
			ss << "; Look: " << *createInfo.colorTransform->lookName;
	}
	else
		ss << "-";
	ss << "\n";
	ss << "Render mode: " << magic_enum::enum_name(m_renderMode) << "\n";
	logHandler(ss.str());

	ss = {};
	auto &cam = GetCamera();
	uint32_t w, h;
	cam.GetResolution(w, h);
	ss << "Camera:\n";
	ss << "Name: " << cam.GetName() << "\n";
	ss << "Resolution: " << w << "x" << h << "\n";
	ss << "FarZ: " << cam.GetFarZ() << "\n";
	ss << "NearZ: " << cam.GetNearZ() << "\n";
	ss << "Fov: " << cam.GetFov() << "\n";
	ss << "Type: " << magic_enum::enum_name(cam.GetType()) << "\n";
	ss << "Panorama Type: " << magic_enum::enum_name(cam.GetPanoramaType()) << "\n";
	ss << "Depth of field enabled: " << cam.IsDofEnabled() << "\n";
	ss << "Focal distance: " << cam.GetFocalDistance() << "\n";
	ss << "Aperture size: " << cam.GetApertureSize() << "\n";
	ss << "Bokeh ratio: " << cam.GetApertureRatio() << "\n";
	ss << "Blae count: " << cam.GetBladeCount() << "\n";
	ss << "Blades rotation: " << cam.GetBladesRotation() << "\n";
	ss << "Stereoscopic: " << cam.IsStereoscopic() << "\n";
	ss << "Interocular distance: " << cam.GetInterocularDistance() << "\n";
	ss << "Aspect ratio: " << cam.GetAspectRatio() << "\n";
	ss << "Longitude: " << cam.GetLongitudeMin() << "," << cam.GetLongitudeMax() << "\n";
	ss << "Latitude: " << cam.GetLatitudeMin() << "," << cam.GetLatitudeMax() << "\n";
	logHandler(ss.str());

	ss = {};
	ss << "Lights:\n";
	auto first = true;
	for(auto &l : GetLights()) {
		if(first)
			first = false;
		else
			ss << "\n";
		ss << "Name: " << l->GetName() << "\n";
		ss << "Type: " << magic_enum::enum_name(l->GetType()) << "\n";
		ss << "Outer cone angle: " << l->GetOuterConeAngle() << "\n";
		ss << "Blend fraction: " << l->GetBlendFraction() << "\n";
		ss << "Color: " << l->GetColor() << "\n";
		ss << "Intensity: " << l->GetIntensity() << "\n";
		ss << "Size: " << l->GetSize() << "\n";
		ss << "U Axis: " << l->GetAxisU() << "\n";
		ss << "V Axis: " << l->GetAxisV() << "\n";
		ss << "U Size: " << l->GetSizeU() << "\n";
		ss << "V Size: " << l->GetSizeV() << "\n";
		ss << "Round: " << l->IsRound() << "\n";
	}
	logHandler(ss.str());
}

bool pragma::scenekit::Scene::IsLightmapRenderMode(RenderMode renderMode) { return umath::to_integral(renderMode) >= umath::to_integral(RenderMode::LightmapBakingStart) && umath::to_integral(renderMode) <= umath::to_integral(RenderMode::LightmapBakingEnd); }

bool pragma::scenekit::Scene::IsBakingRenderMode(RenderMode renderMode) { return umath::to_integral(renderMode) >= umath::to_integral(RenderMode::BakingStart) && umath::to_integral(renderMode) <= umath::to_integral(RenderMode::BakingEnd); }

bool pragma::scenekit::Scene::IsRenderSceneMode(RenderMode renderMode) { return !IsBakingRenderMode(renderMode); }

bool pragma::scenekit::Scene::ReadHeaderInfo(udm::AssetDataArg data, RenderMode &outRenderMode, CreateInfo &outCreateInfo, SerializationData &outSerializationData, uint32_t &outVersion, SceneInfo *optOutSceneInfo)
{
	return ReadSerializationHeader(data, outRenderMode, outCreateInfo, outSerializationData, outVersion, optOutSceneInfo);
}
std::shared_ptr<pragma::scenekit::Scene> pragma::scenekit::Scene::Create(NodeManager &nodeManager, udm::AssetDataArg data, const std::string &rootDir, RenderMode renderMode, const CreateInfo &createInfo)
{
	auto scene = Create(nodeManager, renderMode, createInfo);
	if(scene == nullptr || scene->Load(data, rootDir) == false)
		return nullptr;
	return scene;
}
std::shared_ptr<pragma::scenekit::Scene> pragma::scenekit::Scene::Create(NodeManager &nodeManager, udm::AssetDataArg data, const std::string &rootDir)
{
	RenderMode renderMode;
	CreateInfo createInfo;
	SerializationData serializationData;
	uint32_t version;
	if(ReadSerializationHeader(data, renderMode, createInfo, serializationData, version) == false)
		return nullptr;
	return Create(nodeManager, data, rootDir, renderMode, createInfo);
}

std::shared_ptr<pragma::scenekit::Scene> pragma::scenekit::Scene::Create(NodeManager &nodeManager, RenderMode renderMode, const CreateInfo &createInfo)
{
	auto scene = std::shared_ptr<Scene> {new Scene {nodeManager, renderMode}};

	scene->m_camera = Camera::Create(*scene);
	scene->m_createInfo = createInfo;
	umath::set_flag(scene->m_stateFlags, StateFlags::OutputResultWithHDRColors, createInfo.hdrOutput);
	return scene;
}

pragma::scenekit::Scene::Scene(NodeManager &nodeManager, RenderMode renderMode) : m_renderMode {renderMode}, m_nodeManager {nodeManager.shared_from_this()} {}

pragma::scenekit::Scene::~Scene() {}

pragma::scenekit::Camera &pragma::scenekit::Scene::GetCamera() { return *m_camera; }

bool pragma::scenekit::Scene::IsValidTexture(const std::string &filePath) const
{
	std::string ext;
	if(ufile::get_extension(filePath, &ext) == false || ustring::compare<std::string>(ext, "dds", false) == false)
		return false;
	return FileManager::Exists(filePath, fsys::SearchFlags::Local);
}

std::optional<std::string> pragma::scenekit::Scene::GetAbsSkyPath(const std::string &skyTex)
{
	if(skyTex.empty())
		return {};
	std::string absPath = util::FilePath("materials", skyTex).GetString();
	if (!filemanager::find_local_path(absPath, absPath))
		return {};
	return absPath;
}

static uint32_t calc_pixel_offset(uint32_t imgWidth, uint32_t xOffset, uint32_t yOffset) { return yOffset * imgWidth + xOffset; }

static bool row_contains_visible_pixels(const float *inOutImgData, uint32_t pxStartOffset, uint32_t w)
{
	for(auto x = decltype(w) {0u}; x < w; ++x) {
		if(inOutImgData[(pxStartOffset + x) * 4 + 3] > 0.f)
			return true;
	}
	return false;
}

static bool col_contains_visible_pixels(const float *inOutImgData, uint32_t pxStartOffset, uint32_t h, uint32_t imgWidth)
{
	for(auto y = decltype(h) {0u}; y < h; ++y) {
		if(inOutImgData[(pxStartOffset + (y * imgWidth)) * 4 + 3] > 0.f)
			return true;
	}
	return false;
}

static void shrink_area_to_fit(const float *inOutImgData, uint32_t imgWidth, uint32_t &xOffset, uint32_t &yOffset, uint32_t &w, uint32_t &h)
{
	while(h > 0 && row_contains_visible_pixels(inOutImgData, calc_pixel_offset(imgWidth, xOffset, yOffset), w) == false) {
		++yOffset;
		--h;
	}
	while(h > 0 && row_contains_visible_pixels(inOutImgData, calc_pixel_offset(imgWidth, xOffset, yOffset + h - 1), w) == false)
		--h;

	while(w > 0 && col_contains_visible_pixels(inOutImgData, calc_pixel_offset(imgWidth, xOffset, yOffset), h, imgWidth) == false) {
		++xOffset;
		--w;
	}
	while(w > 0 && col_contains_visible_pixels(inOutImgData, calc_pixel_offset(imgWidth, xOffset + w - 1, yOffset), h, imgWidth) == false)
		--w;
}

void pragma::scenekit::Scene::DenoiseHDRImageArea(uimg::ImageBuffer &imgBuffer, uint32_t imgWidth, uint32_t imgHeight, uint32_t xOffset, uint32_t yOffset, uint32_t w, uint32_t h) const
{
	// In some cases the borders may not contain any image data (i.e. fully transparent) if the pixels are not actually
	// being used by any geometry. Since the denoiser does not know transparency, we have to shrink the image area to exclude the
	// transparent borders to avoid artifacts.
	auto *imgData = static_cast<float *>(imgBuffer.GetData());
	shrink_area_to_fit(imgData, imgWidth, xOffset, yOffset, w, h);

	if(w == 0 || h == 0)
		return; // Nothing for us to do

	// Sanity check
	auto pxStartOffset = calc_pixel_offset(imgWidth, xOffset, yOffset);
	for(auto y = decltype(h) {0u}; y < h; ++y) {
		for(auto x = decltype(w) {0u}; x < w; ++x) {
			auto srcPxIdx = pxStartOffset + y * imgWidth + x;
			auto a = imgData[srcPxIdx * 4 + 3];
			if(a < 1.f) {
				// This should be unreachable, but just in case...
				// If this case does occur, that means there are transparent pixels WITHIN the image area, which are not
				// part of a transparent border!
				std::cerr << "ERROR: Image area for denoising contains transparent pixel at (" << x << "," << y << ") with alpha of " << a << "! This is not allowed!" << std::endl;
			}
		}
	}

	// White areas
	/*for(auto y=decltype(h){0u};y<h;++y)
	{
	for(auto x=decltype(w){0u};x<w;++x)
	{
	auto srcPxIdx = pxStartOffset +y *imgWidth +x;
	auto dstPxIdx = y *w +x;
	if(inOutImgData[srcPxIdx *4 +3] == 0.f)
	{
	inOutImgData[srcPxIdx *4 +0] = 0.f;
	inOutImgData[srcPxIdx *4 +1] = 0.f;
	inOutImgData[srcPxIdx *4 +2] = 0.f;
	inOutImgData[srcPxIdx *4 +3] = 1.f;
	}
	else
	{
	inOutImgData[srcPxIdx *4 +0] = 1.f;
	inOutImgData[srcPxIdx *4 +1] = 1.f;
	inOutImgData[srcPxIdx *4 +2] = 1.f;
	inOutImgData[srcPxIdx *4 +3] = 1.f;
	}
	}
	}*/

	std::vector<float> imgAreaData {};
	imgAreaData.resize(w * h * 3);
	// Extract the area from the image data
	for(auto y = decltype(h) {0u}; y < h; ++y) {
		for(auto x = decltype(w) {0u}; x < w; ++x) {
			auto srcPxIdx = pxStartOffset + y * imgWidth + x;
			auto dstPxIdx = y * w + x;
			for(uint8_t i = 0; i < 3; ++i)
				imgAreaData.at(dstPxIdx * 3 + i) = imgData[srcPxIdx * 4 + i];
		}
	}

	// Denoise the extracted area
	denoise::Info denoiseInfo {};
	denoiseInfo.width = w;
	denoiseInfo.height = h;

	denoise::ImageData denoiseImgData {};
	denoiseImgData.data = reinterpret_cast<uint8_t *>(imgAreaData.data());
	denoiseImgData.format = uimg::Format::RGB32;

	denoise::ImageInputs inputs {};
	inputs.beautyImage = denoiseImgData;
	denoise::denoise(denoiseInfo, inputs, denoiseImgData);

	// Copy the denoised area back into the original image
	for(auto y = decltype(h) {0u}; y < h; ++y) {
		for(auto x = decltype(w) {0u}; x < w; ++x) {
			auto srcPxIdx = pxStartOffset + y * imgWidth + x;
			//if(inOutImgData[srcPxIdx *4 +3] == 0.f)
			//	continue; // Alpha is zero; Skip this one
			auto dstPxIdx = y * w + x;
			//for(uint8_t i=0;i<3;++i)
			//	inOutImgData[srcPxIdx *4 +i] = imgAreaData.at(dstPxIdx *3 +i);
			/*if(inOutImgData[srcPxIdx *4 +3] == 0.f)
			{
			inOutImgData[srcPxIdx *4 +0] = 0.f;
			inOutImgData[srcPxIdx *4 +1] = 0.f;
			inOutImgData[srcPxIdx *4 +2] = 0.f;
			inOutImgData[srcPxIdx *4 +3] = 1.f;
			}
			else
			{
			inOutImgData[srcPxIdx *4 +0] = 1.f;
			inOutImgData[srcPxIdx *4 +1] = 1.f;
			inOutImgData[srcPxIdx *4 +2] = 1.f;
			inOutImgData[srcPxIdx *4 +3] = 1.f;
			}*/
		}
	}
}

void pragma::scenekit::Scene::Close()
{
	m_mdlCaches.clear();
	m_camera = nullptr;
}

float pragma::scenekit::Scene::GetGamma() const { return m_createInfo.hdrOutput ? 1.f : DEFAULT_GAMMA; }

void pragma::scenekit::Scene::AddActorToActorMap(std::unordered_map<size_t, WorldObject *> &map, WorldObject &obj) { map[util::get_uuid_hash(obj.GetUuid())] = &obj; }
std::unordered_map<size_t, pragma::scenekit::WorldObject *> pragma::scenekit::Scene::BuildActorMap() const
{
	std::unordered_map<size_t, pragma::scenekit::WorldObject *> map;
	auto addActor = [&map](pragma::scenekit::WorldObject &obj) { map[util::get_uuid_hash(obj.GetUuid())] = &obj; };
	uint32_t numActors = m_lights.size() + 1u /* camera */;
	for(auto &mdlCache : m_mdlCaches) {
		for(auto &chunk : mdlCache->GetChunks())
			numActors += chunk.GetObjects().size();
	}
	map.reserve(numActors);
	for(auto &light : m_lights)
		addActor(*light);
	addActor(*m_camera);
	for(auto &mdlCache : m_mdlCaches) {
		for(auto &chunk : mdlCache->GetChunks()) {
			for(auto &obj : chunk.GetObjects())
				addActor(*obj);
		}
	}
	return map;
}

const std::vector<pragma::scenekit::PLight> &pragma::scenekit::Scene::GetLights() const { return const_cast<Scene *>(this)->GetLights(); }
std::vector<pragma::scenekit::PLight> &pragma::scenekit::Scene::GetLights() { return m_lights; }

void pragma::scenekit::Scene::Finalize()
{
	m_camera->Finalize(*this);

	m_camera->SetId(0);
	uint32_t id = 0;
	for(auto &light : m_lights)
		light->SetId(id++);

	uint32_t meshId = 0;
	uint32_t objId = 0;
	uint32_t shaderId = 0;
	for(auto &mdlCache : GetModelCaches()) {
		for(auto &chunk : mdlCache->GetChunks()) {
			for(auto &mesh : chunk.GetMeshes())
				mesh->SetId(meshId++);
			for(auto &obj : chunk.GetObjects())
				obj->SetId(objId++);
			for(auto &shader : chunk.GetShaderCache().GetShaders())
				shader->SetId(shaderId++);
		}
	}
}

bool pragma::scenekit::Scene::IsProgressive() const { return m_createInfo.progressive; }
bool pragma::scenekit::Scene::IsProgressiveRefine() const { return m_createInfo.progressiveRefine; }
//pragma::scenekit::Scene::RenderMode pragma::scenekit::Scene::GetRenderMode() const {return m_renderMode;}

void pragma::scenekit::Scene::SetDebugHandler(const std::string &identifier, const std::function<void(const std::shared_ptr<void> &)> &f) { m_debugHandlers[identifier] = f; }
std::function<void(const std::shared_ptr<void> &)> pragma::scenekit::Scene::GetDebugHandler(const std::string &identifier) { return m_debugHandlers[identifier]; }

void pragma::scenekit::Scene::SetLightIntensityFactor(float f) { m_sceneInfo.lightIntensityFactor = f; }
float pragma::scenekit::Scene::GetLightIntensityFactor() const { return m_sceneInfo.lightIntensityFactor; }

static bool g_verbose = false;
void pragma::scenekit::Scene::SetVerbose(bool verbose) { g_verbose = verbose; }
bool pragma::scenekit::Scene::IsVerbose() { return g_verbose; }

void pragma::scenekit::Scene::Save(udm::AssetDataArg outData, const std::string &rootDir, const SerializationData &serializationData) const {
	auto modelCachePath = util::DirPath(rootDir, "cache").GetString();
	filemanager::create_path(modelCachePath);

	outData.SetAssetType(PRT_IDENTIFIER);
	outData.SetAssetVersion(PRT_VERSION);
	auto udm = *outData;

	m_createInfo.Serialize(udm);
	udm["renderMode"] << m_renderMode;
	udm["outputFileName"] << serializationData.outputFileName;

	auto udmScene = udm["sceneInfo"];
	auto udmSky = udmScene["sky"];
	if(!m_sceneInfo.sky.empty()) {
		auto absSky = GetAbsSkyPath(m_sceneInfo.sky);
		if(absSky.has_value())
			udmSky["texture"] = *absSky;
		else
			udmSky["texture"] = m_sceneInfo.sky;
	}
	udmSky["angles"] = m_sceneInfo.skyAngles;
	udmSky["strength"] = m_sceneInfo.skyStrength;
	udmSky["transparent"] = m_sceneInfo.transparentSky;

	udmScene["emissionStrength"] = m_sceneInfo.emissionStrength;
	udmScene["lightIntensityFactor"] = m_sceneInfo.lightIntensityFactor;
	udmScene["motionBlurStrength"] = m_sceneInfo.motionBlurStrength;

	auto udmLimits = udmScene["limits"];
	udmLimits["maxTransparencyBounces"] = m_sceneInfo.maxTransparencyBounces;
	udmLimits["maxBounces"] = m_sceneInfo.maxBounces;
	udmLimits["maxDiffuseBounces"] = m_sceneInfo.maxDiffuseBounces;
	udmLimits["maxGlossyBounces"] = m_sceneInfo.maxGlossyBounces;
	udmLimits["maxTransmissionBounces"] = m_sceneInfo.maxTransmissionBounces;

	udmScene["exposure"] = m_sceneInfo.exposure;
	udmScene["useAdaptiveSampling"] = m_sceneInfo.useAdaptiveSampling;
	udmScene["adaptiveSamplingThreshold"] = m_sceneInfo.adaptiveSamplingThreshold;
	udmScene["adaptiveMinSamples"] = m_sceneInfo.adaptiveMinSamples;

	udm["stateFlags"] << udm::enum_to_string(m_stateFlags);

	auto propResources = udm::Property::Create<udm::Element>();
	auto udmModelCaches = udm.AddArray("modelCaches", m_mdlCaches.size());
	size_t idx = 0;
	for(auto &mdlCache : m_mdlCaches) {
		auto udmModelCache = udmModelCaches[idx++];
		// Try to create a reasonable hash to identify the cache
		auto &chunks = mdlCache->GetChunks();
		size_t hash = 0;
		if(mdlCache->IsUnique()) {
			std::random_device rd;
			std::default_random_engine generator(rd());
			std::uniform_int_distribution<long long unsigned> distribution(0, 0xFFFFFFFFFFFFFFFF);
			hash = util::hash_combine<uint64_t>(0u, distribution(generator)); // Not the best solution, but extremely unlikely to cause collisions
		}
		else {
			hash = util::hash_combine<uint64_t>(0u, chunks.size());
			for(auto &chunk : chunks) {
				auto &objects = chunk.GetObjects();
				auto &meshes = chunk.GetMeshes();
				hash = util::hash_combine<uint64_t>(hash, objects.size());
				hash = util::hash_combine<uint64_t>(hash, meshes.size());
				for(auto &m : meshes) {
					hash = util::hash_combine<std::string>(hash, m->GetName());
					hash = util::hash_combine<uint64_t>(hash, m->GetVertexCount());
					hash = util::hash_combine<uint64_t>(hash, m->GetTriangleCount());
				}
			}
		}
		auto mdlCachePath = util::FilePath(modelCachePath, std::to_string(hash) + "." +std::string {PRTMC_EXTENSION_BINARY}).GetString();
		if(filemanager::exists(mdlCachePath) == false) {
			filemanager::create_path(ufile::get_path_from_filename(mdlCachePath));
			auto f = filemanager::open_file(mdlCachePath, filemanager::FileMode::Write | filemanager::FileMode::Binary);
			if (f) {
				auto data = udm::Data::Create(PRTMC_IDENTIFIER, PRTMC_VERSION);
				auto assetData = data->GetAssetData();

				auto udmData = assetData.GetData();
				mdlCache->Serialize(udmData);

				fsys::File fptr {f};
				data->Save(fptr);
			}
		}
		udmModelCache["hash"] << hash;
	}

	//for(auto &mdlCache : m_mdlCaches)
	//	m_renderData.modelCache->Merge(*mdlCache);
	//m_renderData.modelCache->Bake();

	auto udmLights = udm.AddArray("lights", m_lights.size());
	idx = 0;
	for(auto &light : m_lights) {
		auto udmLight = udmLights[idx++];
		light->Serialize(udmLight);
	}

	{
		auto udmCamera = udm["camera"];
		m_camera->Serialize(udmCamera);
	}
	udm["bakeTargetName"] << m_bakeTargetName;
}
bool pragma::scenekit::Scene::ReadSerializationHeader(udm::AssetDataArg data, RenderMode &outRenderMode, CreateInfo &outCreateInfo, SerializationData &outSerializationData, uint32_t &outVersion, SceneInfo *optOutSceneInfo)
{
	//if (data.GetAssetType() != PRT_IDENTIFIER)
	//	return false;
	auto version = data.GetAssetVersion();
	//if (version != PRT_VERSION)
	//	return false;
	outVersion = version;

	auto udm = data.GetData();
	outCreateInfo.Deserialize(udm);

	udm["renderMode"] >> outRenderMode;
	udm["outputFileName"] >> outSerializationData.outputFileName;

	if(optOutSceneInfo) {
		auto &sceneInfo = *optOutSceneInfo;
		auto udmScene = udm["sceneInfo"];
		auto udmSky = udmScene["sky"];

		udmSky["texture"] >> sceneInfo.sky;
		udmSky["angles"](sceneInfo.skyAngles);
		udmSky["strength"](sceneInfo.skyStrength);
		udmSky["transparent"](sceneInfo.transparentSky);

		udmScene["emissionStrength"](sceneInfo.emissionStrength);
		udmScene["lightIntensityFactor"](sceneInfo.lightIntensityFactor);
		udmScene["motionBlurStrength"](sceneInfo.motionBlurStrength);

		auto udmLimits = udmScene["limits"];
		udmLimits["maxTransparencyBounces"](sceneInfo.maxTransparencyBounces);
		udmLimits["maxBounces"](sceneInfo.maxBounces);
		udmLimits["maxDiffuseBounces"](sceneInfo.maxDiffuseBounces);
		udmLimits["maxGlossyBounces"](sceneInfo.maxGlossyBounces);
		udmLimits["maxTransmissionBounces"](sceneInfo.maxTransmissionBounces);

		udmScene["exposure"](sceneInfo.exposure);
		udmScene["useAdaptiveSampling"](sceneInfo.useAdaptiveSampling);
		udmScene["adaptiveSamplingThreshold"](sceneInfo.adaptiveSamplingThreshold);
		udmScene["adaptiveMinSamples"](sceneInfo.adaptiveMinSamples);
	}
	return true;
}
bool pragma::scenekit::Scene::Load(udm::AssetDataArg data, const std::string &rootDir)
{
	auto modelCachePath = rootDir + "cache/";

	SerializationData serializationData;
	uint32_t version;
	CreateInfo createInfo {};
	if(ReadSerializationHeader(data, m_renderMode, createInfo, serializationData, version, &m_sceneInfo) == false)
		return false;
	auto udm = data.GetData();
	m_stateFlags = udm::string_to_flags<StateFlags>(udm["stateFlags"], StateFlags::None);

	auto udmModelCaches = udm["modelCaches"];
	auto numCaches = udmModelCaches.GetSize();
	m_mdlCaches.reserve(numCaches);
	for(auto i = decltype(numCaches) {0u}; i < numCaches; ++i) {
		auto udmCache = udmModelCaches[i];
		size_t hash = 0;
		udmCache["hash"] >> hash;
		auto baseMdlCachePath = modelCachePath + std::to_string(hash);
		auto mdlCachePath = baseMdlCachePath + "." +std::string{PRTMC_EXTENSION_BINARY};
		auto f = filemanager::open_system_file(mdlCachePath, filemanager::FileMode::Read | filemanager::FileMode::Binary);
		if (!f) {
			mdlCachePath = baseMdlCachePath + "." +std::string{PRTMC_EXTENSION_ASCII};
			f = filemanager::open_system_file(mdlCachePath, filemanager::FileMode::Read | filemanager::FileMode::Binary);
		}
		if(f) {
			auto fptr = std::make_unique<fsys::File>(f);
			auto udmData = udm::Data::Load(std::move(fptr));
			auto assetData = udmData->GetAssetData();
			auto udmCacheData = assetData.GetData();
			auto mdlCache = ModelCache::Create(udmCacheData, GetShaderNodeManager());
			if(mdlCache)
				m_mdlCaches.push_back(mdlCache);
		}
	}

	auto udmLights = udm["lights"];
	m_lights.reserve(udmLights.GetSize());
	for (auto &udmLight : udmLights)
		m_lights.push_back(Light::Create(udmLight));

	auto udmCamera = udm["camera"];
	m_camera->Deserialize(udmCamera);

	udm["bakeTargetName"] >> m_bakeTargetName;
	return true;
}

void pragma::scenekit::Scene::HandleError(const std::string &errMsg) const { std::cerr << errMsg << std::endl; }
pragma::scenekit::NodeManager &pragma::scenekit::Scene::GetShaderNodeManager() const { return *m_nodeManager; }

void pragma::scenekit::Scene::SetSky(const std::string &skyPath) { m_sceneInfo.sky = skyPath; }
void pragma::scenekit::Scene::SetSkyAngles(const EulerAngles &angSky) { m_sceneInfo.skyAngles = angSky; }
void pragma::scenekit::Scene::SetSkyStrength(float strength) { m_sceneInfo.skyStrength = strength; }
void pragma::scenekit::Scene::SetEmissionStrength(float strength) { m_sceneInfo.emissionStrength = strength; }
float pragma::scenekit::Scene::GetEmissionStrength() const { return m_sceneInfo.emissionStrength; }
void pragma::scenekit::Scene::SetMaxTransparencyBounces(uint32_t maxBounces) { m_sceneInfo.maxTransparencyBounces = maxBounces; }
void pragma::scenekit::Scene::SetMaxBounces(uint32_t maxBounces) { m_sceneInfo.maxBounces = maxBounces; }
void pragma::scenekit::Scene::SetMaxDiffuseBounces(uint32_t bounces) { m_sceneInfo.maxDiffuseBounces = bounces; }
void pragma::scenekit::Scene::SetMaxGlossyBounces(uint32_t bounces) { m_sceneInfo.maxGlossyBounces = bounces; }
void pragma::scenekit::Scene::SetMaxTransmissionBounces(uint32_t bounces) { m_sceneInfo.maxTransmissionBounces = bounces; }
void pragma::scenekit::Scene::SetMotionBlurStrength(float strength) { m_sceneInfo.motionBlurStrength = strength; }
void pragma::scenekit::Scene::SetAdaptiveSampling(bool enabled, float adaptiveSamplingThreshold, uint32_t adaptiveMinSamples)
{
	m_sceneInfo.useAdaptiveSampling = enabled;
	m_sceneInfo.adaptiveSamplingThreshold = adaptiveSamplingThreshold;
	m_sceneInfo.adaptiveMinSamples = adaptiveMinSamples;
}
const std::string *pragma::scenekit::Scene::GetBakeTargetName() const { return m_bakeTargetName.has_value() ? &*m_bakeTargetName : nullptr; }
bool pragma::scenekit::Scene::HasBakeTarget() const { return m_bakeTargetName.has_value(); }
void pragma::scenekit::Scene::SetBakeTarget(Object &o)
{
	o.SetName("bake_target");
	m_bakeTargetName = "bake_target";
}
Vector2i pragma::scenekit::Scene::GetResolution() const { return {m_camera->GetWidth(), m_camera->GetHeight()}; }

std::string pragma::scenekit::Scene::ToRelativePath(const std::string &absPath)
{
	util::Path path {absPath};
	path.MakeRelative(FileManager::GetRootPath());
	while(path.GetFront() != "materials")
		path.PopFront();
	return path.GetString();
}

std::string pragma::scenekit::Scene::ToAbsolutePath(const std::string &relPath)
{
	std::string rpath;
	auto result = FileManager::FindAbsolutePath(relPath, rpath);
	if(IsVerbose()) {
		if(result)
			std::cout << "Resolved relative path '" << relPath << "' to absolute path '" << rpath << "'..." << std::endl;
		//else
		//	std::cout<<"Unable to resolve relative path '"<<relPath<<"': File not found!"<<std::endl;
	}
	if(result == false)
		std::cout << "WARNING: Unable to locate file '" << relPath << "': File not found!" << std::endl;
	return result ? rpath : (FileManager::GetRootPath() + relPath);
}

void pragma::scenekit::Scene::AddModelsFromCache(const ModelCache &cache)
{
	if(m_mdlCaches.size() == m_mdlCaches.capacity())
		m_mdlCaches.reserve(m_mdlCaches.size() * 1.5 + 10);
	m_mdlCaches.push_back(const_cast<ModelCache &>(cache).shared_from_this());
}
void pragma::scenekit::Scene::AddLight(Light &light)
{
	if(m_lights.size() == m_lights.capacity())
		m_lights.reserve(m_lights.size() * 1.5 + 50);
	m_lights.push_back(light.shared_from_this());
}

std::optional<pragma::scenekit::PassType> pragma::scenekit::get_main_pass_type(Scene::RenderMode renderMode)
{
	switch(renderMode) {
	case Scene::RenderMode::RenderImage:
		return PassType::Combined;
	case Scene::RenderMode::BakeAmbientOcclusion:
		return PassType::Ao;
	case Scene::RenderMode::BakeNormals:
		return PassType::Normals;
	case Scene::RenderMode::BakeDiffuseLighting:
		return PassType::Diffuse;
	case Scene::RenderMode::BakeDiffuseLightingSeparate:
		return {};
	case Scene::RenderMode::SceneAlbedo:
		return PassType::Albedo;
	case Scene::RenderMode::SceneNormals:
		return PassType::Normals;
	case Scene::RenderMode::SceneDepth:
		return PassType::Depth;

	case Scene::RenderMode::Alpha:
	case Scene::RenderMode::GeometryNormal:
	case Scene::RenderMode::ShadingNormal:
	case Scene::RenderMode::DirectDiffuseReflect:
	case Scene::RenderMode::DirectDiffuseTransmit:
	case Scene::RenderMode::DirectGlossyReflect:
	case Scene::RenderMode::DirectGlossyTransmit:
	case Scene::RenderMode::IndirectDiffuseReflect:
	case Scene::RenderMode::IndirectDiffuseTransmit:
	case Scene::RenderMode::IndirectGlossyReflect:
	case Scene::RenderMode::IndirectGlossyTransmit:
	case Scene::RenderMode::IndirectSpecular:
	case Scene::RenderMode::IndirectSpecularReflect:
	case Scene::RenderMode::IndirectSpecularTransmit:
	case Scene::RenderMode::Noise:
	case Scene::RenderMode::Irradiance:
	case Scene::RenderMode::Caustic:
		return {};
	case Scene::RenderMode::DirectDiffuse:
		return PassType::DiffuseDirect;
	case Scene::RenderMode::DirectGlossy:
		return PassType::GlossyDirect;
	case Scene::RenderMode::Emission:
		return PassType::Emission;
	case Scene::RenderMode::IndirectDiffuse:
		return PassType::DiffuseIndirect;
	case Scene::RenderMode::IndirectGlossy:
		return PassType::GlossyIndirect;
	case Scene::RenderMode::Uv:
		return PassType::Uv;
	}
	static_assert(umath::to_integral(Scene::RenderMode::Count) == 31, "Update this implementation when new render modes are added!");
	return {};
}
