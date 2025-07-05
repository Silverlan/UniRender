// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __UTIL_RAYTRACING_DEFINITIONS_HPP__
#define __UTIL_RAYTRACING_DEFINITIONS_HPP__

#ifdef RTUTIL_STATIC
#define DLLRTUTIL
#elif RTUTIL_DLL
#ifdef __linux__
#define DLLRTUTIL __attribute__((visibility("default")))
#else
#define DLLRTUTIL __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define DLLRTUTIL
#else
#define DLLRTUTIL __declspec(dllimport)
#endif
#endif

namespace uimg {
	class ImageBuffer;
	struct ImageLayerSet;
};
namespace util::ocio {
	class ColorProcessor;
};
namespace oidn {
	class DeviceRef;
};

namespace ccl {
	class Session;
	class Scene;
	class ShaderInput;
	class ShaderNode;
	class ShaderOutput;
	class ShaderGraph;
	struct float3;
	struct float2;
	struct Transform;
	class ImageTextureNode;
	class EnvironmentTextureNode;
	class BufferParams;
	class SessionParams;
};
namespace udm {
	struct Property;
};
namespace OpenImageIO_v2_1 {
	class ustring;
};
namespace umath {
	class Transform;
	class ScaledTransform;
};
namespace spdlog {
	class logger;
};
namespace OpenSubdiv::v3_6_0::Far {
	class PrimvarRefiner;
};
class DataStream;

#endif
