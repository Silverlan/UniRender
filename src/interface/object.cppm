// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include "util_enum_flags.hpp"

export module pragma.scenekit:object;

import :world_object;
import :scene_object;

export namespace pragma::scenekit {
	class Scene;
	class Mesh;
	using PMesh = std::shared_ptr<Mesh>;
	class Object;
	using PObject = std::shared_ptr<Object>;
	class ModelCache;
	class DLLRTUTIL Object : public WorldObject, public BaseObject, public std::enable_shared_from_this<Object> {
	  public:
		enum class Flags : uint8_t { None = 0u, EnableSubdivision = 1u };
		static PObject Create(Mesh &mesh);
		static PObject Create(udm::LinkedPropertyWrapper &data, const std::function<PMesh(uint32_t)> &fGetMesh);
		util::WeakHandle<Object> GetHandle();
		virtual void DoFinalize(Scene &scene) override;

		void SetSubdivisionEnabled(bool enabled) { math::set_flag(m_flags, Flags::EnableSubdivision, enabled); }
		bool IsSubdivisionEnabled() const { return math::is_flag_set(m_flags, Flags::EnableSubdivision); }

		const Mesh &GetMesh() const;
		Mesh &GetMesh();

		void Serialize(udm::LinkedPropertyWrapper &data, const std::function<std::optional<uint32_t>(const Mesh &)> &fGetMeshIndex) const;
		void Serialize(udm::LinkedPropertyWrapper &data, const std::unordered_map<const Mesh *, size_t> &meshToIndexTable) const;
		void Deserialize(udm::LinkedPropertyWrapper &data, const std::function<PMesh(uint32_t)> &fGetMesh);

		const math::Transform &GetMotionPose() const;
		void SetMotionPose(const math::Transform &pose);

		void SetName(const std::string &name);
		const std::string &GetName() const;
	  protected:
		static PObject Create(Mesh *mesh);
		Object(Mesh *mesh);
		PMesh m_mesh = nullptr;
		Flags m_flags = Flags::None;
		std::string m_name;

		// TODO
		math::Transform m_motionPose = {};
	};
	using namespace pragma::math::scoped_enum::bitwise;
};
export {
	REGISTER_ENUM_FLAGS(pragma::scenekit::Object::Flags)
}
