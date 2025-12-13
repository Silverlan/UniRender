// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include "util_enum_flags.hpp"

export module pragma.scenekit:renderer;

import :tile_manager;
export import pragma.udm;

export namespace pragma::scenekit {
	DLLRTUTIL void set_log_handler(const std::function<void(const std::string)> &logHandler = nullptr);
	DLLRTUTIL const std::function<void(const std::string)> &get_log_handler();

	DLLRTUTIL void set_module_lookup_location(const std::string &location);

	class ModelCache;
	class ShaderCache;
	class Renderer;
	using PRenderer = std::shared_ptr<Renderer>;
	class DLLRTUTIL RenderWorker : public util::ParallelWorker<image::ImageLayerSet> {
	  public:
		friend Renderer;
		RenderWorker(Renderer &renderer);
		using ParallelWorker<image::ImageLayerSet>::Cancel;
		virtual void Wait() override;
		virtual image::ImageLayerSet GetResult() override;

		using ParallelWorker<image::ImageLayerSet>::SetResultMessage;
		using ParallelWorker<image::ImageLayerSet>::AddThread;
		using ParallelWorker<image::ImageLayerSet>::UpdateProgress;
	  private:
		virtual void DoCancel(const std::string &resultMsg, std::optional<int32_t> resultCode) override;
		PRenderer m_renderer = nullptr;
	};

	enum class PassType : uint32_t {
		Combined = 0,
		Albedo,
		Normals,
		Depth,
		Emission,
		Background,
		Ao,
		Diffuse,
		DiffuseDirect,
		DiffuseIndirect,
		Glossy,
		GlossyDirect,
		GlossyIndirect,
		Transmission,
		TransmissionDirect,
		TransmissionIndirect,
		Volume,
		VolumeDirect,
		VolumeIndirect,

		Position,
		Roughness,
		Uv,
		DiffuseColor,
		GlossyColor,
		TransmissionColor,

		Count
	};

	class Scene;
	class Mesh;
	class Shader;
	class Object;
	class WorldObject;
	class DLLRTUTIL Renderer : public std::enable_shared_from_this<Renderer> {
	  public:
		enum class Flags : uint32_t {
			None = 0u,
			EnableLiveEditing = 1u,
			DisableDisplayDriver = EnableLiveEditing << 1u,
			CompilingKernels = DisableDisplayDriver << 1u,
		};
		enum class StereoEye : uint8_t {
			Left = 0,
			Right,
			Count,

			None = std::numeric_limits<uint8_t>::max()
		};
		enum class Feature : uint32_t { None = 0, OptiXAvailable = 1 };
		static std::shared_ptr<Renderer> Create(const Scene &scene, const std::string &rendererIdentifier, std::string &outErr, Flags flags = Flags::None);
		static bool UnloadRendererLibrary(const std::string &rendererIdentifier);
		static void Close();

		virtual ~Renderer() = default;
		virtual void Wait() = 0;
		virtual void Start() = 0;
		virtual float GetProgress() const = 0;
		virtual void Reset() = 0;
		virtual void Restart() = 0;
		virtual bool Stop() = 0;
		virtual bool Pause() = 0;
		virtual bool Resume() = 0;
		virtual bool Suspend() = 0;
		virtual bool BeginSceneEdit() { return false; }
		virtual bool EndSceneEdit() { return false; }
		virtual bool SyncEditedActor(const util::Uuid &uuid) = 0;
		virtual bool AddLiveActor(WorldObject &actor) = 0;
		virtual bool Export(const std::string &path) = 0;
		virtual std::optional<std::string> SaveRenderPreview(const std::string &path, std::string &outErr) const = 0;
		virtual util::ParallelJob<image::ImageLayerSet> StartRender() = 0;
		void StopRendering();
		virtual bool IsFeatureEnabled(Feature feature) const;

		const std::unordered_map<size_t, WorldObject *> &GetActorMap() const { return m_actorMap; }
		WorldObject *FindActor(const util::Uuid &uuid);

		std::shared_ptr<Mesh> FindRenderMeshByHash(const util::MurmurHash3 &hash) const;
		udm::PropertyWrapper GetApiData() const;
		Flags GetFlags() const { return m_flags; }

		virtual bool ShouldUseProgressiveFloatFormat() const;
		bool ShouldUseTransparentSky() const;
		bool IsDisplayDriverEnabled() const;
		void SetIsBuildingKernels(bool compiling);
		bool IsBuildingKernels() const;
		Scene &GetScene() { return *m_scene; }
		const Scene &GetScene() const { return const_cast<Renderer *>(this)->GetScene(); }
		TileManager &GetTileManager() { return m_tileManager; }
		const TileManager &GetTileManager() const { return const_cast<Renderer *>(this)->GetTileManager(); }
		std::vector<TileManager::TileData> GetRenderedTileBatch();
		void AddActorToActorMap(WorldObject &obj);

		const std::unordered_map<PassType, std::array<std::shared_ptr<image::ImageBuffer>, math::to_integral(StereoEye::Count)>> &GetResultImageBuffers() const { return m_resultImageBuffers; }
	  protected:
		Renderer(const Scene &scene, Flags flags);
		bool Initialize();
		Object *FindObject(const std::string &objectName) const;
		friend RenderWorker;
		enum class ImageRenderStage : uint8_t {
			InitializeScene = 0,
			Lighting,
			Albedo,
			Normal,
			Denoise,
			FinalizeImage,
			MergeStereoscopic,

			SceneAlbedo,
			SceneNormals,
			SceneDepth,

			Bake,

			Finalize,
			Count
		};
		enum class RenderStageResult : uint8_t { Complete = 0, Continue };
		void OnParallelWorkerCancelled();
		RenderStageResult StartNextRenderStage(RenderWorker &worker, ImageRenderStage stage, StereoEye eyeStage);
		virtual util::EventReply HandleRenderStage(RenderWorker &worker, ImageRenderStage stage, StereoEye eyeStage, RenderStageResult *optResult = nullptr);
		virtual void PrepareCyclesSceneForRendering();
		virtual bool UpdateStereoEye(RenderWorker &worker, ImageRenderStage stage, StereoEye &eyeStage) = 0;
		virtual void SetCancelled(const std::string &msg = "Cancelled by application.") = 0;
		virtual void CloseRenderScene() = 0;
		virtual void FinalizeImage(image::ImageBuffer &imgBuf, StereoEye eyeStage) {};
		void UpdateActorMap();
		std::pair<uint32_t, PassType> AddPass(PassType passType);
		void DumpImage(const std::string &renderStage, image::ImageBuffer &imgBuffer, image::ImageFormat format = image::ImageFormat::HDR, const std::optional<std::string> &fileName = {}) const;
		bool ShouldDumpRenderStageImages() const;

		std::shared_ptr<Scene> m_scene = nullptr;
		std::atomic<Flags> m_flags = Flags::None;
		TileManager m_tileManager {};
		udm::PProperty m_apiData = nullptr;

		struct {
			std::shared_ptr<ShaderCache> shaderCache;
			std::shared_ptr<ModelCache> modelCache;
		} m_renderData;

		std::atomic<bool> m_progressiveRunning = false;
		std::condition_variable m_progressiveCondition {};
		std::mutex m_progressiveMutex {};
		std::shared_ptr<ocio::ColorProcessor> m_colorTransformProcessor = nullptr;
		std::unordered_map<size_t, WorldObject *> m_actorMap;

		std::shared_ptr<image::ImageBuffer> &GetResultImageBuffer(PassType type, StereoEye eye = StereoEye::Left);
		image::ImageBuffer *FindResultImageBuffer(PassType type, StereoEye eye = StereoEye::Left);
		std::unordered_map<PassType, std::array<std::shared_ptr<image::ImageBuffer>, math::to_integral(StereoEye::Count)>> m_resultImageBuffers = {};

		std::unordered_map<PassType, uint32_t> m_passes {};
		uint32_t m_nextOutputIndex = 0;
	};
	using namespace pragma::math::scoped_enum::bitwise;
};
export {
	REGISTER_ENUM_FLAGS(pragma::scenekit::Renderer::Flags)
	REGISTER_ENUM_FLAGS(pragma::scenekit::Renderer::Feature)
}
