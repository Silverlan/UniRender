// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.scenekit;

import :scene_object;

static pragma::scenekit::BaseObject *target = nullptr;
pragma::scenekit::BaseObject::BaseObject() {}
pragma::scenekit::BaseObject::~BaseObject() {}
pragma::scenekit::Scene &pragma::scenekit::SceneObject::GetScene() const { return m_scene; }
pragma::scenekit::SceneObject::SceneObject(Scene &scene) : m_scene {scene} {}

void pragma::scenekit::BaseObject::Finalize(Scene &scene, bool force)
{
	if(m_bFinalized && force == false)
		return;
	m_bFinalized = true;
	DoFinalize(scene);
}
void pragma::scenekit::BaseObject::DoFinalize(Scene &scene) {}
