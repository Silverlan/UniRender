// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <sharedutils/datastream.h>
#include <sharedutils/util_weak_handle.hpp>
#include <mathutil/transform.hpp>
#include <udm.hpp>
#include <optional>
#include <cassert>

module pragma.scenekit;

import :object;
import :mesh;

pragma::scenekit::PObject pragma::scenekit::Object::Create(Mesh *mesh) { return PObject {new Object {mesh}}; }
pragma::scenekit::PObject pragma::scenekit::Object::Create(Mesh &mesh) { return Create(&mesh); }

pragma::scenekit::PObject pragma::scenekit::Object::Create(udm::LinkedPropertyWrapper &data, const std::function<PMesh(uint32_t)> &fGetMesh)
{
	auto o = Create(nullptr);
	o->Deserialize(data, fGetMesh);
	return o;
}

pragma::scenekit::Object::Object(Mesh *mesh) : WorldObject {}, BaseObject {}, m_mesh {mesh ? mesh->shared_from_this() : nullptr} {}

void pragma::scenekit::Object::Serialize(udm::LinkedPropertyWrapper &data, const std::function<std::optional<uint32_t>(const Mesh &)> &fGetMeshIndex) const
{
	WorldObject::Serialize(data);
	auto idx = fGetMeshIndex(*m_mesh);
	assert(idx.has_value());
	data["meshIndex"] << *idx;
	data["name"] << GetName();
}
void pragma::scenekit::Object::Serialize(udm::LinkedPropertyWrapper &data, const std::unordered_map<const Mesh *, size_t> &meshToIndexTable) const
{
	Serialize(data, [&meshToIndexTable](const Mesh &mesh) -> std::optional<uint32_t> {
		auto it = meshToIndexTable.find(&mesh);
		return (it != meshToIndexTable.end()) ? it->second : std::optional<uint32_t> {};
	});
}
void pragma::scenekit::Object::Deserialize(udm::LinkedPropertyWrapper &data, const std::function<PMesh(uint32_t)> &fGetMesh)
{
	WorldObject::Deserialize(data);
	uint32_t meshIdx = 0;
	data["meshIndex"] >> meshIdx;
	data["name"] >> m_name;
	auto mesh = fGetMesh(meshIdx);
	assert(mesh);
	m_mesh = mesh;
}

util::WeakHandle<pragma::scenekit::Object> pragma::scenekit::Object::GetHandle() { return util::WeakHandle<pragma::scenekit::Object> {shared_from_this()}; }

void pragma::scenekit::Object::DoFinalize(Scene &scene) { m_mesh->Finalize(scene); }

const pragma::scenekit::Mesh &pragma::scenekit::Object::GetMesh() const { return const_cast<Object *>(this)->GetMesh(); }
pragma::scenekit::Mesh &pragma::scenekit::Object::GetMesh() { return *m_mesh; }

const umath::Transform &pragma::scenekit::Object::GetMotionPose() const { return m_motionPose; }
void pragma::scenekit::Object::SetMotionPose(const umath::Transform &pose) { m_motionPose = pose; }

void pragma::scenekit::Object::SetName(const std::string &name) { m_name = name; }
const std::string &pragma::scenekit::Object::GetName() const { return m_name; }
