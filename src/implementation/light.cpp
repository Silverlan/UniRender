// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <udm.hpp>
#include <mathutil/umath_lighting.hpp>
#include <mathutil/color.h>
#include <sharedutils/util_pragma.hpp>
#include <sharedutils/datastream.h>
#include <sharedutils/util_weak_handle.hpp>

module pragma.scenekit;

import :light;
import :scene;

pragma::scenekit::PLight pragma::scenekit::Light::Create()
{
	auto pLight = PLight {new Light {}};
	return pLight;
}

pragma::scenekit::PLight pragma::scenekit::Light::Create(udm::LinkedPropertyWrapper &data)
{
	auto light = Create();
	light->Deserialize(data);
	return light;
}

pragma::scenekit::Light::Light() : WorldObject {} {}

util::WeakHandle<pragma::scenekit::Light> pragma::scenekit::Light::GetHandle() { return util::WeakHandle<pragma::scenekit::Light> {shared_from_this()}; }

void pragma::scenekit::Light::SetType(Type type) { m_type = type; }

void pragma::scenekit::Light::SetConeAngle(umath::Degree outerAngle, umath::Fraction blendFraction)
{
	m_blendFraction = blendFraction;
	m_spotOuterAngle = outerAngle;
}

void pragma::scenekit::Light::SetColor(const Color &color)
{
	m_color = color.ToVector3();
	// Alpha is ignored
}
void pragma::scenekit::Light::SetIntensity(Lumen intensity) { m_intensity = intensity; }

void pragma::scenekit::Light::SetSize(float size) { m_size = size; }

void pragma::scenekit::Light::SetAxisU(const Vector3 &axisU) { m_axisU = axisU; }
void pragma::scenekit::Light::SetAxisV(const Vector3 &axisV) { m_axisV = axisV; }
void pragma::scenekit::Light::SetSizeU(float sizeU) { m_sizeU = sizeU; }
void pragma::scenekit::Light::SetSizeV(float sizeV) { m_sizeV = sizeV; }

void pragma::scenekit::Light::Serialize(udm::LinkedPropertyWrapper &data) const
{
	WorldObject::Serialize(data);
	auto &udm = data;
	udm["size"] << m_size;
	udm["color"] << m_color;
	udm["intensity"] << m_intensity;
	udm["type"] << m_type;
	udm["blendFraction"] << m_blendFraction;
	udm["spotOuterAngle"] << m_spotOuterAngle;
	udm["axisU"] << m_axisU;
	udm["axisV"] << m_axisV;
	udm["sizeU"] << m_sizeU;
	udm["sizeV"] << m_sizeV;
	udm["round"] << m_bRound;
	// udm["flags"] << udm::flags_to_string(m_flags);
}

void pragma::scenekit::Light::Deserialize(udm::LinkedPropertyWrapper &data)
{
	WorldObject::Deserialize(data);
	auto &udm = data;
	udm["size"] >> m_size;
	udm["color"] >> m_color;
	udm["intensity"] >> m_intensity;
	udm["type"] >> m_type;
	udm["blendFraction"] >> m_blendFraction;
	udm["spotOuterAngle"] >> m_spotOuterAngle;
	udm["axisU"] >> m_axisU;
	udm["axisV"] >> m_axisV;
	udm["sizeU"] >> m_sizeU;
	udm["sizeV"] >> m_sizeV;
	udm["round"] >> m_bRound;
	m_flags = Flags::None;
	// m_flags = udm::string_to_flags(udm["flags"], Flags::None);
}

void pragma::scenekit::Light::DoFinalize(Scene &scene) {}
