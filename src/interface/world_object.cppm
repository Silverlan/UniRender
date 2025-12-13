// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.scenekit:world_object;

export import pragma.udm;

export namespace pragma::scenekit {
	class WorldObject;
	using PWorldObject = std::shared_ptr<WorldObject>;
	class DLLRTUTIL WorldObject {
	  public:
		virtual ~WorldObject() = default;
		void SetPos(const Vector3 &pos);
		const Vector3 &GetPos() const;

		void SetRotation(const Quat &rot);
		const Quat &GetRotation() const;

		void SetScale(const Vector3 &scale);
		const Vector3 &GetScale() const;

		pragma::math::ScaledTransform &GetPose();
		const pragma::math::ScaledTransform &GetPose() const;

		void SetUuid(const pragma::util::Uuid &uuid) { m_uuid = uuid; }
		const pragma::util::Uuid &GetUuid() const { return m_uuid; }

		void Serialize(udm::LinkedPropertyWrapper &data) const;
		void Deserialize(udm::LinkedPropertyWrapper &data);
	  protected:
		WorldObject();
	  private:
		pragma::math::ScaledTransform m_pose = {};
		pragma::util::Uuid m_uuid = {0, 0};
	};
};
