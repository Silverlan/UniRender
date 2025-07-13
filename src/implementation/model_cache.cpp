// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>
#include <iostream>
#include <mathutil/umath.h>
#include <sharedutils/util.h>
#include <sharedutils/datastream.h>
#include <udm.hpp>

#undef GetObject

module pragma.scenekit;

import :model_cache;
import :shader;
import :scene;
import :object;
import :mesh;

#pragma clang optimize off
std::shared_ptr<pragma::scenekit::ShaderCache> pragma::scenekit::ShaderCache::Create() { return std::shared_ptr<ShaderCache> {new ShaderCache {}}; }
std::shared_ptr<pragma::scenekit::ShaderCache> pragma::scenekit::ShaderCache::Create(udm::LinkedPropertyWrapper &data, NodeManager &nodeManager)
{
	auto cache = Create();
	cache->Deserialize(data, nodeManager);
	return cache;
}

const std::vector<std::shared_ptr<pragma::scenekit::Shader>> &pragma::scenekit::ShaderCache::GetShaders() const { return m_shaders; }
std::vector<std::shared_ptr<pragma::scenekit::Shader>> &pragma::scenekit::ShaderCache::GetShaders() { return m_shaders; }

size_t pragma::scenekit::ShaderCache::AddShader(Shader &shader)
{
	if(m_shaders.size() == m_shaders.capacity())
		m_shaders.reserve(m_shaders.size() * 1.5 + 50);
	m_shaders.push_back(shader.shared_from_this());
	return m_shaders.size() - 1;
}

pragma::scenekit::PShader pragma::scenekit::ShaderCache::GetShader(uint32_t idx) const { return (idx < m_shaders.size()) ? m_shaders.at(idx) : nullptr; }

void pragma::scenekit::ShaderCache::Merge(const ShaderCache &other)
{
	m_shaders.reserve(m_shaders.size() + other.m_shaders.size());
	for(auto &s : other.m_shaders)
		m_shaders.push_back(s);
}

std::unordered_map<const pragma::scenekit::Shader *, size_t> pragma::scenekit::ShaderCache::GetShaderToIndexTable() const
{
	std::unordered_map<const Shader *, size_t> shaderToIndex;
	shaderToIndex.reserve(m_shaders.size());
	for(auto i = decltype(m_shaders.size()) {0u}; i < m_shaders.size(); ++i)
		shaderToIndex[m_shaders.at(i).get()] = i;
	return shaderToIndex;
}

void pragma::scenekit::ShaderCache::Serialize(udm::LinkedPropertyWrapper &data)
{
	auto udmShaders = data.AddArray("shaders", m_shaders.size());

	uint32_t shaderIdx = 0;
	for(auto &s : m_shaders) {
		auto udmShader = udmShaders[shaderIdx++];
		s->Serialize(udmShader);
	}
}
void pragma::scenekit::ShaderCache::Deserialize(udm::LinkedPropertyWrapper &data, NodeManager &nodeManager)
{
	auto udmShaders = data["shaders"];
	auto n = udmShaders.GetSize();
	m_shaders.resize(n);
	for(auto i = decltype(n) {0u}; i < n; ++i) {
		auto udmShader = udmShaders[i];
		auto shader = Shader::Create<GenericShader>();
		shader->Deserialize(udmShader, nodeManager);
		m_shaders.at(i) = shader;
	}
}

//////////

pragma::scenekit::ModelCacheChunk::ModelCacheChunk(ShaderCache &shaderCache) : m_shaderCache {shaderCache.shared_from_this()} {}
pragma::scenekit::ModelCacheChunk::ModelCacheChunk(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager) { Deserialize(data, nodeManager); }
const std::vector<udm::PProperty> &pragma::scenekit::ModelCacheChunk::GetBakedObjectData() const { return m_bakedObjects; }
const std::vector<udm::PProperty> &pragma::scenekit::ModelCacheChunk::GetBakedMeshData() const { return m_bakedMeshes; }
std::unordered_map<const pragma::scenekit::Mesh *, size_t> pragma::scenekit::ModelCacheChunk::GetMeshToIndexTable() const
{
	std::unordered_map<const Mesh *, size_t> meshToIndex;
	meshToIndex.reserve(m_meshes.size());
	for(auto i = decltype(m_meshes.size()) {0u}; i < m_meshes.size(); ++i)
		meshToIndex[m_meshes.at(i).get()] = i;
	return meshToIndex;
}

static std::string toHexString(util::MurmurHash3 const& h) {
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (uint8_t byte : h)
		oss << std::setw(2) << static_cast<int>(byte);
	return oss.str();  // 32 chars: two hex digits per byte
}

static util::MurmurHash3 fromHexString(std::string const& hex) {
	util::MurmurHash3 h;
	if (hex.size() != h.size()*2)
		throw std::runtime_error{"bad hex length"};
	for (size_t i = 0; i < h.size(); ++i) {
		unsigned int byte;
		std::istringstream(hex.substr(2*i, 2)) >> std::hex >> byte;
		h[i] = static_cast<uint8_t>(byte);
	}
	return h;
}

void pragma::scenekit::ModelCacheChunk::Bake()
{
	if(umath::is_flag_set(m_flags, Flags::HasBakedData))
		return;
	auto meshToIndexTable = GetMeshToIndexTable();
	m_bakedObjects.reserve(m_objects.size());
	for(auto &o : m_objects) {
		auto prop = udm::Property::Create<udm::Element>();
		udm::LinkedPropertyWrapper udm {*prop};
		o->Serialize(udm, meshToIndexTable);
		auto hash = prop->CalcHash();
		udm["hash"] = toHexString(hash);
		o->SetHash(std::move(hash));

		m_bakedObjects.push_back(prop);
	}

	auto shaderToIndexTable = m_shaderCache->GetShaderToIndexTable();
	m_bakedMeshes.reserve(m_meshes.size());
	for(auto &m : m_meshes) {
		auto prop = udm::Property::Create<udm::Element>();
		udm::LinkedPropertyWrapper udm {*prop};
		m->Serialize(udm, shaderToIndexTable);
		auto hash = prop->CalcHash();
		udm["hash"] = toHexString(hash);
		m->SetHash(std::move(hash));

		m_bakedMeshes.push_back(prop);
	}
	m_flags |= Flags::HasBakedData;
}

const std::vector<std::shared_ptr<pragma::scenekit::Mesh>> &pragma::scenekit::ModelCacheChunk::GetMeshes() const { return const_cast<ModelCacheChunk *>(this)->GetMeshes(); }
std::vector<std::shared_ptr<pragma::scenekit::Mesh>> &pragma::scenekit::ModelCacheChunk::GetMeshes() { return m_meshes; }
const std::vector<std::shared_ptr<pragma::scenekit::Object>> &pragma::scenekit::ModelCacheChunk::GetObjects() const { return const_cast<ModelCacheChunk *>(this)->GetObjects(); }
std::vector<std::shared_ptr<pragma::scenekit::Object>> &pragma::scenekit::ModelCacheChunk::GetObjects() { return m_objects; }

size_t pragma::scenekit::ModelCacheChunk::AddMesh(Mesh &mesh)
{
	Unbake();
	if(m_meshes.size() == m_meshes.capacity())
		m_meshes.reserve(m_meshes.size() * 1.5 + 50);
	m_meshes.push_back(mesh.shared_from_this());
	return m_meshes.size() - 1;
}
size_t pragma::scenekit::ModelCacheChunk::AddObject(Object &obj)
{
	Unbake();
	if(m_objects.size() == m_objects.capacity())
		m_objects.reserve(m_objects.size() * 1.5 + 50);
	m_objects.push_back(obj.shared_from_this());
	return m_objects.size() - 1;
}
void pragma::scenekit::ModelCacheChunk::RemoveMesh(Mesh &mesh)
{
	auto it = std::find_if(m_meshes.begin(), m_meshes.end(), [&mesh](const std::shared_ptr<Mesh> &other) { return other.get() == &mesh; });
	if(it == m_meshes.end())
		return;
	m_meshes.erase(it);
}
void pragma::scenekit::ModelCacheChunk::RemoveObject(Object &obj)
{
	auto it = std::find_if(m_objects.begin(), m_objects.end(), [&obj](const std::shared_ptr<Object> &other) { return other.get() == &obj; });
	if(it == m_objects.end())
		return;
	m_objects.erase(it);
}

pragma::scenekit::PMesh pragma::scenekit::ModelCacheChunk::GetMesh(uint32_t idx) const { return (idx < m_meshes.size()) ? m_meshes.at(idx) : nullptr; }
pragma::scenekit::PObject pragma::scenekit::ModelCacheChunk::GetObject(uint32_t idx) const { return (idx < m_objects.size()) ? m_objects.at(idx) : nullptr; }

void pragma::scenekit::ModelCacheChunk::GenerateUnbakedData(bool force)
{
	if(umath::is_flag_set(m_flags, Flags::HasUnbakedData) && force == false)
		return;
	auto &shaders = m_shaderCache->GetShaders();
	m_meshes.resize(m_bakedMeshes.size());
	for(auto i = decltype(m_bakedMeshes.size()) {0u}; i < m_bakedMeshes.size(); ++i) {
		auto &prop = m_bakedMeshes.at(i);
		udm::LinkedPropertyWrapper data {*prop};
		auto mesh = Mesh::Create(data, [&](uint32_t idx) -> PShader { return (idx < shaders.size()) ? shaders.at(idx) : nullptr; });
		std::string strHash;
		data["hash"] >> strHash;
		auto hash = fromHexString(strHash);
		mesh->SetHash(std::move(hash));
		m_meshes.at(i) = mesh;
	}

	m_objects.resize(m_bakedObjects.size());
	for(auto i = decltype(m_bakedObjects.size()) {0u}; i < m_bakedObjects.size(); ++i) {
		auto &prop = m_bakedObjects.at(i);
		udm::LinkedPropertyWrapper data {*prop};

		auto obj = Object::Create(data, [this](uint32_t idx) -> PMesh { return (idx < m_meshes.size()) ? m_meshes.at(idx) : nullptr; });
		std::string strHash;
		data["hash"] >> strHash;
		auto hash = fromHexString(strHash);
		obj->SetHash(std::move(hash));
		m_objects.at(i) = obj;
	}
	m_flags |= Flags::HasUnbakedData;
}

void pragma::scenekit::ModelCacheChunk::Unbake()
{
	if(umath::is_flag_set(m_flags, Flags::HasBakedData) == false)
		return;
	if(umath::is_flag_set(m_flags, Flags::HasUnbakedData) == false)
		GenerateUnbakedData();
	m_bakedObjects.clear();
	m_bakedMeshes.clear();
	umath::remove_flag(m_flags, Flags::HasBakedData);
}

void pragma::scenekit::ModelCacheChunk::Serialize(udm::LinkedPropertyWrapper &data)
{
	Bake();

	GetShaderCache().Serialize(data);

	auto fWriteList = [&data](const std::string &identifier, const std::vector<udm::PProperty> &list) {
		auto udmData = data.AddArray(identifier, list.size());
		size_t idx = 0;
		for(auto &prop : list) {
			udm::LinkedPropertyWrapper src {*prop};
			auto udmObj = udmData[idx++];
			udmObj.Merge(src);
		}
	};
	fWriteList("objects", m_bakedObjects);
	fWriteList("meshes", m_bakedMeshes);
}
void pragma::scenekit::ModelCacheChunk::Deserialize(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager)
{
	m_shaderCache = ShaderCache::Create(data, nodeManager);

	auto fReadList = [&data](const std::string &identifier, std::vector<udm::PProperty> &list) {
		auto udmData = data[identifier];
		auto numObjects = udmData.GetSize();
		list.resize(numObjects);
		for(auto i = decltype(numObjects) {0u}; i < numObjects; ++i) {
			auto udmObj = udmData[i];
			auto prop = udm::Property::Create(udm::Type::Element);
			udm::LinkedPropertyWrapper dst {*prop};
			dst.Merge(udmObj);
			list.at(i) = prop;
		}
	};
	fReadList("objects", m_bakedObjects);
	fReadList("meshes", m_bakedMeshes);
	m_flags = Flags::HasBakedData;
}

//////////

std::shared_ptr<pragma::scenekit::ModelCache> pragma::scenekit::ModelCache::Create() { return std::shared_ptr<ModelCache> {new ModelCache {}}; }

std::shared_ptr<pragma::scenekit::ModelCache> pragma::scenekit::ModelCache::Create(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager)
{
	auto cache = Create();
	cache->Deserialize(data, nodeManager);
	return cache;
}

void pragma::scenekit::ModelCache::SetUnique(bool unique) { m_unique = unique; }
bool pragma::scenekit::ModelCache::IsUnique() const { return m_unique; }

void pragma::scenekit::ModelCache::Merge(ModelCache &other)
{
	m_chunks.reserve(m_chunks.size() + other.m_chunks.size());
	for(auto &chunk : other.m_chunks)
		m_chunks.push_back(chunk);
}

void pragma::scenekit::ModelCache::Bake()
{
	for(auto &chunk : m_chunks)
		chunk.Bake();
}

void pragma::scenekit::ModelCache::GenerateData()
{
	for(auto &chunk : m_chunks)
		chunk.GenerateUnbakedData(true);
}

void pragma::scenekit::ModelCache::Serialize(udm::LinkedPropertyWrapper &data)
{
	Bake();

	auto udmChunks = data.AddArray("chunks", m_chunks.size());
	size_t idx = 0;
	for(auto &chunk : m_chunks) {
		auto udmChunk = udmChunks[idx++];
		chunk.Serialize(udmChunk);
	}
}
void pragma::scenekit::ModelCache::Deserialize(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager)
{
	auto udmChunks = data["chunks"];
	auto numChunks = udmChunks.GetSize();
	m_chunks.reserve(numChunks);
	for(auto i = decltype(numChunks) {0u}; i < numChunks; ++i) {
		auto udmChunk = udmChunks[i];
		m_chunks.emplace_back(udmChunk, nodeManager);
	}
}
pragma::scenekit::ModelCacheChunk &pragma::scenekit::ModelCache::AddChunk(ShaderCache &shaderCache)
{
	if(m_chunks.size() == m_chunks.capacity())
		m_chunks.reserve(m_chunks.size() * 1.5 + 10);
	m_chunks.emplace_back(shaderCache);
	return m_chunks.back();
}
