// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <sharedutils/datastream.h>
#include <mathutil/uvec.h>
#include <mathutil/transform.hpp>
#include <udm.hpp>

module pragma.scenekit;

import :world_object;

pragma::scenekit::WorldObject::WorldObject() {}

void pragma::scenekit::WorldObject::SetPos(const Vector3 &pos) { m_pose.SetOrigin(pos); }
const Vector3 &pragma::scenekit::WorldObject::GetPos() const { return m_pose.GetOrigin(); }

void pragma::scenekit::WorldObject::SetRotation(const Quat &rot) { m_pose.SetRotation(rot); }
const Quat &pragma::scenekit::WorldObject::GetRotation() const { return m_pose.GetRotation(); }

void pragma::scenekit::WorldObject::SetScale(const Vector3 &scale) { m_pose.SetScale(scale); }
const Vector3 &pragma::scenekit::WorldObject::GetScale() const { return m_pose.GetScale(); }

umath::ScaledTransform &pragma::scenekit::WorldObject::GetPose() { return m_pose; }
const umath::ScaledTransform &pragma::scenekit::WorldObject::GetPose() const { return const_cast<WorldObject *>(this)->GetPose(); }

void pragma::scenekit::WorldObject::Serialize(udm::LinkedPropertyWrapper &data) const
{
	data["pose"] << m_pose;
	data["uuid"] << util::uuid_to_string(m_uuid);
}
void pragma::scenekit::WorldObject::Deserialize(udm::LinkedPropertyWrapper &data)
{
	data["pose"] >> m_pose;
	std::string uuid;
	data["uuid"] >> uuid;
	m_uuid = util::uuid_string_to_bytes(uuid);
}
