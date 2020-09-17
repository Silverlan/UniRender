/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#ifndef __RT_COLOR_MANAGEMENT_HPP__
#define __RT_COLOR_MANAGEMENT_HPP__

#include "definitions.hpp"
#include <cinttypes>
#include <functional>

namespace uimg {class ImageBuffer;};
namespace util::ocio {class ColorProcessor;};
namespace raytracing
{
	enum class ColorTransform : uint8_t
	{
		FilmicBlender = 0
	};
	DLLRTUTIL std::shared_ptr<util::ocio::ColorProcessor> create_color_transform_processor(ColorTransform transform,std::string &outErr);
};

#endif
