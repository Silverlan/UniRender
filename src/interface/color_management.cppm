// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.scenekit:color_management;

import pragma.ocio;

export namespace pragma::scenekit {
	struct DLLRTUTIL ColorTransformProcessorCreateInfo {
		enum class BitDepth : uint8_t { Float32 = 0, Float16, UInt8 };
		std::string config = "filmic-blender";
		std::optional<std::string> lookName {};
		BitDepth bitDepth = BitDepth::Float32;
	};
	DLLRTUTIL std::shared_ptr<pragma::ocio::ColorProcessor> create_color_transform_processor(const ColorTransformProcessorCreateInfo &createInfo, std::string &outErr, float exposure = 0.f, float gamma = 2.2f);
	DLLRTUTIL bool apply_color_transform(uimg::ImageBuffer &imgBuf, const ColorTransformProcessorCreateInfo &createInfo, std::string &outErr, float exposure = 0.f, float gamma = 2.2f);
};
