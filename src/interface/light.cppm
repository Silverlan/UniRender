// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include "util_enum_flags.hpp"

export module pragma.scenekit:light;

import :world_object;
import :scene_object;

export namespace pragma::scenekit {
	using Lumen = float;
	class Light;
	using PLight = std::shared_ptr<Light>;
	class DLLRTUTIL Light : public WorldObject, public BaseObject, public std::enable_shared_from_this<Light> {
	  public:
		enum class Flags : uint8_t { None = 0u };
		enum class Type : uint8_t {
			Point = 0u,
			Spot,
			Directional,

			Area,
			Background,
			Triangle
		};
		static PLight Create();
		static PLight Create(udm::LinkedPropertyWrapper &data);
		util::WeakHandle<Light> GetHandle();

		void SetType(Type type);
		void SetConeAngle(umath::Degree outerAngle, umath::Fraction blendFraction);
		void SetColor(const Color &color);
		void SetIntensity(Lumen intensity);
		void SetSize(float size);
		virtual void DoFinalize(Scene &scene) override;

		void SetAxisU(const Vector3 &axisU);
		void SetAxisV(const Vector3 &axisV);
		void SetSizeU(float sizeU);
		void SetSizeV(float sizeV);

		void Serialize(udm::LinkedPropertyWrapper &data) const;
		void Deserialize(udm::LinkedPropertyWrapper &data);

		Type GetType() const { return m_type; }
		float GetSize() const { return m_size; }
		const Vector3 &GetColor() const { return m_color; }
		Lumen GetIntensity() const { return m_intensity; }
		umath::Fraction GetBlendFraction() const { return m_blendFraction; }
		umath::Degree GetOuterConeAngle() const { return m_spotOuterAngle; }
		const Vector3 &GetAxisU() const { return m_axisU; }
		const Vector3 &GetAxisV() const { return m_axisV; }
		float GetSizeU() const { return m_sizeU; }
		float GetSizeV() const { return m_sizeV; }
		bool IsRound() const { return m_bRound; }
		Flags GetFlags() const { return m_flags; }
	  private:
		Light();

		// Note: All of these are automatically serialized/deserialized!
		// There must be no unserializable data after this point!
		float m_size = pragma::metres_to_units(1.f);
		Vector3 m_color = {1.f, 1.f, 1.f};
		Lumen m_intensity = 1'600.f;
		Type m_type = Type::Point;
		umath::Fraction m_blendFraction = 0.f;
		umath::Degree m_spotOuterAngle = 0.f;

		Vector3 m_axisU = {};
		Vector3 m_axisV = {};
		float m_sizeU = pragma::metres_to_units(1.f);
		float m_sizeV = pragma::metres_to_units(1.f);
		bool m_bRound = false;
		Flags m_flags = Flags::None;
	};
	using namespace umath::scoped_enum::bitwise;
};
export {
	REGISTER_ENUM_FLAGS(pragma::scenekit::Light::Flags)
}
