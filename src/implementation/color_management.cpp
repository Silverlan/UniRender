// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <sharedutils/util.h>
#include <sharedutils/util_path.hpp>
#include <util_image_buffer.hpp>

module pragma.scenekit;

import pragma.ocio;

import :color_management;

bool pragma::scenekit::apply_color_transform(uimg::ImageBuffer &imgBuf, const ColorTransformProcessorCreateInfo &createInfo, std::string &outErr, float exposure, float gamma)
{
	auto processor = create_color_transform_processor(createInfo, outErr, exposure, gamma);
	if(!processor)
		return false;
	return processor->Apply(imgBuf, outErr);
}
std::shared_ptr<pragma::ocio::ColorProcessor> pragma::scenekit::create_color_transform_processor(const ColorTransformProcessorCreateInfo &createInfo, std::string &outErr, float exposure, float gamma)
{
	auto ocioConfigLocation = util::Path::CreatePath(util::get_program_path());
	ocioConfigLocation += "modules/open_color_io/configs/";
	ocioConfigLocation.Canonicalize();

	pragma::ocio::ColorProcessor::CreateInfo ocioCreateInfo {};
	ocioCreateInfo.configLocation = ocioConfigLocation.GetString();
	ocioCreateInfo.config = createInfo.config;
	ocioCreateInfo.lookName = createInfo.lookName;
	ocioCreateInfo.bitDepth = static_cast<pragma::ocio::ColorProcessor::CreateInfo::BitDepth>(createInfo.bitDepth);
	return pragma::ocio::ColorProcessor::Create(ocioCreateInfo, outErr, exposure, gamma);
}
