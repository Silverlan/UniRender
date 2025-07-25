// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include <memory>
#include <unordered_map>
#include <mathutil/umath.h>
#include <sharedutils/datastream.h>
#include <udm.hpp>

#undef GetObject

export module pragma.scenekit:model_cache;

export namespace pragma::scenekit {
	class NodeManager;
	class Scene;
	class Mesh;
	class Object;
	class Shader;
	using PShader = std::shared_ptr<Shader>;
	using PMesh = std::shared_ptr<Mesh>;
	using PObject = std::shared_ptr<Object>;
	class DLLRTUTIL ShaderCache : public std::enable_shared_from_this<ShaderCache> {
	  public:
		static std::shared_ptr<ShaderCache> Create();
		static std::shared_ptr<ShaderCache> Create(udm::LinkedPropertyWrapper &data, NodeManager &nodeManager);

		const std::vector<std::shared_ptr<Shader>> &GetShaders() const;
		std::vector<std::shared_ptr<Shader>> &GetShaders();

		size_t AddShader(Shader &shader);

		void Merge(const ShaderCache &other);

		PShader GetShader(uint32_t idx) const;

		std::unordered_map<const Shader *, size_t> GetShaderToIndexTable() const;

		void Serialize(udm::LinkedPropertyWrapper &data);
		void Deserialize(udm::LinkedPropertyWrapper &data, NodeManager &nodeManager);
	  private:
		std::vector<std::shared_ptr<Shader>> m_shaders;
	};

	class ModelCache;
	class DLLRTUTIL ModelCacheChunk {
	  public:
		static constexpr uint32_t MURMUR_SEED = 195574;
		enum class Flags : uint8_t { None = 0u, HasBakedData = 1u, HasUnbakedData = HasBakedData << 1u };
		ModelCacheChunk(ShaderCache &shaderCache);
		ModelCacheChunk(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager);
		void Bake();
		void GenerateUnbakedData(bool force = false);

		size_t AddMesh(Mesh &mesh);
		size_t AddObject(Object &obj);

		void RemoveMesh(Mesh &mesh);
		void RemoveObject(Object &obj);

		PMesh GetMesh(uint32_t idx) const;
		PObject GetObject(uint32_t idx) const;

		const std::vector<std::shared_ptr<Mesh>> &GetMeshes() const;
		std::vector<std::shared_ptr<Mesh>> &GetMeshes();
		const std::vector<std::shared_ptr<Object>> &GetObjects() const;
		std::vector<std::shared_ptr<Object>> &GetObjects();

		void Serialize(udm::LinkedPropertyWrapper &data);
		void Deserialize(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager);

		const std::vector<udm::PProperty> &GetBakedObjectData() const;
		const std::vector<udm::PProperty> &GetBakedMeshData() const;

		ShaderCache &GetShaderCache() const { return *m_shaderCache; }

		std::unordered_map<const Mesh *, size_t> GetMeshToIndexTable() const;
	  private:
		void Unbake();

		std::shared_ptr<ShaderCache> m_shaderCache = nullptr;

		Flags m_flags = Flags::HasUnbakedData;
		std::vector<std::shared_ptr<Object>> m_objects;
		std::vector<std::shared_ptr<Mesh>> m_meshes;

		std::vector<udm::PProperty> m_bakedObjects;
		std::vector<udm::PProperty> m_bakedMeshes;
	};

	class DLLRTUTIL ModelCache : public std::enable_shared_from_this<ModelCache> {
	  public:
		static std::shared_ptr<ModelCache> Create();
		static std::shared_ptr<ModelCache> Create(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager);

		void Merge(ModelCache &other);

		void Serialize(udm::LinkedPropertyWrapper &data);
		void Deserialize(udm::LinkedPropertyWrapper &data, pragma::scenekit::NodeManager &nodeManager);

		ModelCacheChunk &AddChunk(ShaderCache &shaderCache);
		const std::vector<ModelCacheChunk> &GetChunks() const { return const_cast<ModelCache *>(this)->GetChunks(); }
		std::vector<ModelCacheChunk> &GetChunks() { return m_chunks; }

		void SetUnique(bool unique);
		bool IsUnique() const;

		void Bake();
		void GenerateData();
	  private:
		ModelCache() = default;
		std::vector<ModelCacheChunk> m_chunks {};
		bool m_unique = false;
	};
};
export {REGISTER_BASIC_BITWISE_OPERATORS(pragma::scenekit::ModelCacheChunk::Flags)};
