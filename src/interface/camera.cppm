// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.scenekit:camera;

import :world_object;
import :scene_object;

export namespace pragma::scenekit {
	class Camera;
	using PCamera = std::shared_ptr<Camera>;
	class Scene;
	class DLLRTUTIL Camera : public WorldObject, public SceneObject, public std::enable_shared_from_this<Camera> {
	  public:
		enum class CameraType : uint8_t { Perspective = 0, Orthographic, Panorama };
		enum class PanoramaType : uint8_t { Equirectangular = 0, FisheyeEquidistant, FisheyeEquisolid, Mirrorball };

		static PCamera Create(Scene &scene);
		util::WeakHandle<Camera> GetHandle();

		void SetInterocularDistance(math::Millimeter dist);
		void SetEquirectangularHorizontalRange(math::Degree range);
		void SetEquirectangularVerticalRange(math::Degree range);
		void SetStereoscopic(bool stereo);

		void Serialize(udm::LinkedPropertyWrapper &data) const;
		void Deserialize(udm::LinkedPropertyWrapper &data);

		void SetResolution(uint32_t width, uint32_t height);
		void GetResolution(uint32_t &width, uint32_t &height) const;
		void SetFarZ(math::Meter farZ);
		void SetNearZ(math::Meter nearZ);
		void SetFOV(math::Degree fov);
		void SetCameraType(CameraType type);
		void SetPanoramaType(PanoramaType type);

		void SetDepthOfFieldEnabled(bool enabled);
		void SetFocalDistance(math::Meter focalDistance);
		void SetApertureSize(float size);
		void SetApertureSizeFromFStop(float fstop, math::Millimeter focalLength);
		void SetFOVFromFocalLength(math::Millimeter focalLength, math::Millimeter sensorSize);
		void SetBokehRatio(float ratio);
		void SetBladeCount(uint32_t numBlades);
		void SetBladesRotation(math::Degree rotation);

		CameraType GetType() const { return m_type; }
		uint32_t GetWidth() const { return m_width; }
		uint32_t GetHeight() const { return m_height; }
		math::Meter GetNearZ() const { return m_nearZ; }
		math::Meter GetFarZ() const { return m_farZ; }
		math::Degree GetFov() const { return m_fov; }
		math::Meter GetFocalDistance() const { return m_focalDistance; }
		float GetApertureSize() const { return m_apertureSize; }
		float GetApertureRatio() const { return m_apertureRatio; }
		uint32_t GetBladeCount() const { return m_numBlades; }
		math::Degree GetBladesRotation() const { return m_bladesRotation; }
		PanoramaType GetPanoramaType() const { return m_panoramaType; }
		bool IsDofEnabled() const { return m_dofEnabled; }
		bool IsStereoscopic() const;
		float GetInterocularDistance() const { return m_interocularDistance; }
		float GetAspectRatio() const;
		math::Degree GetLongitudeMin() const { return m_longitudeMin; }
		math::Degree GetLongitudeMax() const { return m_longitudeMax; }
		math::Degree GetLatitudeMin() const { return m_latitudeMin; }
		math::Degree GetLatitudeMax() const { return m_latitudeMax; }
	  private:
		Camera(Scene &scene);

		// Note: All of these are automatically serialized/deserialized!
		// There must be no unserializable data after this point!
		CameraType m_type = CameraType::Perspective;
		uint32_t m_width = 1'024;
		uint32_t m_height = 512;
		math::Meter m_nearZ = 0.1f;
		math::Meter m_farZ = 1'000.f;
		math::Degree m_fov = 39.6f;
		math::Meter m_focalDistance = 10.f;
		float m_apertureSize = 0.f;
		float m_apertureRatio = 1.f;
		uint32_t m_numBlades = 3;
		math::Degree m_bladesRotation = 0;
		PanoramaType m_panoramaType = PanoramaType::Equirectangular;
		math::Millimeter m_interocularDistance = 65.f;
		math::Degree m_longitudeMin = -90.f;
		math::Degree m_longitudeMax = 90.f;
		math::Degree m_latitudeMin = -90.f;
		math::Degree m_latitudeMax = 90.f;
		bool m_dofEnabled = false;
		bool m_stereoscopic = false;
	};
};
