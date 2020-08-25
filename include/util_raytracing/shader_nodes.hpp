/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#ifndef __PR_CYCLES_NODES_HPP__
#define __PR_CYCLES_NODES_HPP__

#include "definitions.hpp"
#include <mathutil/uvec.h>
#include <optional>

namespace ccl
{
	class PrincipledBsdfNode; class NormalMapNode; class ToonBsdfNode; class GlassBsdfNode; class MixClosureNode; class AddClosureNode; class TransparentBsdfNode; class TranslucentBsdfNode; class MixNode;
	class SeparateXYZNode; class CombineXYZNode; class SeparateRGBNode; class CombineRGBNode; class BackgroundNode; class TextureCoordinateNode; class MappingNode;
	class EnvironmentTextureNode; class ImageTextureNode; class ColorNode; class EmissionNode; class MathNode; class AttributeNode; class LightPathNode; class DiffuseBsdfNode;
	class CameraNode; class HSVNode; class ScatterVolumeNode;
	enum AttributeStandard : int32_t;
	enum NodeMathType : int32_t;
};
namespace raytracing {struct NumberSocket;};
raytracing::NumberSocket operator+(float value,const raytracing::NumberSocket &socket);
raytracing::NumberSocket operator-(float value,const raytracing::NumberSocket &socket);
raytracing::NumberSocket operator*(float value,const raytracing::NumberSocket &socket);
raytracing::NumberSocket operator/(float value,const raytracing::NumberSocket &socket);
namespace raytracing
{
	class Shader;
	class CCLShader;
	struct MathNode;
	struct DLLRTUTIL Socket
	{
		Socket();
		Socket(CCLShader &shader,const std::string &nodeName="",const std::string &socketName="",bool output=true);
		Socket(const Socket &other)=default;
		Socket &operator=(const Socket &other)=default;
		CCLShader &GetShader() const;
		bool IsOutput() const;
		bool IsInput() const;
		std::string nodeName;
		std::string socketName;

		MathNode operator+(float value) const;
		MathNode operator+(const Socket &socket) const;

		MathNode operator-(float value) const;
		MathNode operator-(const Socket &socket) const;

		MathNode operator*(float value) const;
		MathNode operator*(const Socket &socket) const;

		MathNode operator/(float value) const;
		MathNode operator/(const Socket &socket) const;
	private:
		CCLShader *m_shader = nullptr;
		bool m_bOutput = false;
	};

	struct DLLRTUTIL Node
	{
		// Note: An instance of this type should never be created manually
		Node(CCLShader &shader);
		CCLShader &GetShader() const;
	private:
		CCLShader *m_shader = nullptr;
	};
	// Special socket type for numeric types
	struct DLLRTUTIL NumberSocket
	{
		NumberSocket();
		NumberSocket(const Socket &socket);
		NumberSocket(float value);

		CCLShader &GetShader() const;
		NumberSocket operator+(const NumberSocket &value) const;
		NumberSocket operator-(const NumberSocket &value) const;
		NumberSocket operator*(const NumberSocket &value) const;
		NumberSocket operator/(const NumberSocket &value) const;
		NumberSocket operator-() const;

		NumberSocket pow(const NumberSocket &exponent) const;
		NumberSocket sqrt() const;
		NumberSocket clamp(const NumberSocket &min,const NumberSocket &max) const;
		NumberSocket lerp(const NumberSocket &to,const NumberSocket &by) const;
		NumberSocket min(const NumberSocket &other) const;
		NumberSocket max(const NumberSocket &other) const;

		static NumberSocket len(const std::array<const NumberSocket,2> &v);
		static NumberSocket dot(
			const std::array<const NumberSocket,4> &v0,
			const std::array<const NumberSocket,4> &v1
		);
	protected:
		friend CCLShader;
		friend MathNode;
		friend NumberSocket (::operator+)(float value,const NumberSocket &socket);
		friend NumberSocket (::operator-)(float value,const NumberSocket &socket);
		friend NumberSocket (::operator*)(float value,const NumberSocket &socket);
		friend NumberSocket (::operator/)(float value,const NumberSocket &socket);
		mutable CCLShader *m_shader = nullptr;
		std::optional<Socket> m_socket = {};
		float m_value = 0.f;
	};
	struct DLLRTUTIL MathNode
		: public Node,
		public NumberSocket
	{
		MathNode(CCLShader &shader,const std::string &nodeName,ccl::MathNode &node);
		Socket inValue1;
		Socket inValue2;

		NumberSocket outValue;

		operator const NumberSocket&() const;

		void SetValue1(float value);
		void SetValue2(float value);
		void SetType(ccl::NodeMathType type);
	private:
		ccl::MathNode *m_node = nullptr;
	};
	struct DLLRTUTIL HSVNode
		: public Node
	{
		HSVNode(CCLShader &shader,const std::string &nodeName,ccl::HSVNode &hsvNode);
		NumberSocket inHue;
		NumberSocket inSaturation;
		NumberSocket inValue;
		NumberSocket inFac;
		Socket inColor;

		Socket outColor;

		operator const Socket&() const;
	private:
		ccl::HSVNode *m_node = nullptr;
	};
	struct DLLRTUTIL SeparateXYZNode
		: public Node
	{
		SeparateXYZNode(CCLShader &shader,const std::string &nodeName,ccl::SeparateXYZNode &node);
		Socket inVector;

		NumberSocket outX;
		NumberSocket outY;
		NumberSocket outZ;

		void SetVector(const Vector3 &v);
	private:
		ccl::SeparateXYZNode *m_node = nullptr;
	};
	struct DLLRTUTIL CombineXYZNode
		: public Node
	{
		CombineXYZNode(CCLShader &shader,const std::string &nodeName,ccl::CombineXYZNode &node);
		Socket inX;
		Socket inY;
		Socket inZ;

		Socket outVector;

		operator const Socket&() const;

		void SetX(float x);
		void SetY(float y);
		void SetZ(float Z);
	private:
		ccl::CombineXYZNode *m_node = nullptr;
	};
	struct DLLRTUTIL SeparateRGBNode
		: public Node
	{
		SeparateRGBNode(CCLShader &shader,const std::string &nodeName,ccl::SeparateRGBNode &node);
		Socket inColor;

		NumberSocket outR;
		NumberSocket outG;
		NumberSocket outB;

		void SetColor(const Vector3 &c);
	private:
		ccl::SeparateRGBNode *m_node = nullptr;
	};
	struct DLLRTUTIL CombineRGBNode
		: public Node
	{
		CombineRGBNode(CCLShader &shader,const std::string &nodeName,ccl::CombineRGBNode &node);
		Socket inR;
		Socket inG;
		Socket inB;

		Socket outColor;

		operator const Socket&() const;

		void SetR(float r);
		void SetG(float g);
		void SetB(float b);
	private:
		ccl::CombineRGBNode *m_node = nullptr;
	};
	struct DLLRTUTIL GeometryNode
		: public Node
	{
		GeometryNode(CCLShader &shader,const std::string &nodeName);
		Socket inNormal;

		Socket outPosition;
		Socket outNormal;
		Socket outTangent;
		Socket outTrueNormal;
		Socket outIncoming;
		Socket outParametric;
		NumberSocket outBackfacing;
		NumberSocket outPointiness;
	};
	struct DLLRTUTIL CameraDataNode
		: public Node
	{
		CameraDataNode(CCLShader &shader,const std::string &nodeName,ccl::CameraNode &node);

		Socket outViewVector;
		NumberSocket outViewZDepth;
		NumberSocket outViewDistance;
	private:
		ccl::CameraNode *m_node = nullptr;
	};
	struct DLLRTUTIL ImageTextureNode
		: public Node
	{
		ImageTextureNode(CCLShader &shader,const std::string &nodeName);
		Socket inUVW;

		Socket outColor;
		NumberSocket outAlpha;

		operator const Socket&() const;
	};
	struct DLLRTUTIL EnvironmentTextureNode
		: public Node
	{
		EnvironmentTextureNode(CCLShader &shader,const std::string &nodeName);
		Socket inVector;
		Socket outColor;
		NumberSocket outAlpha;

		operator const Socket&() const;
	};
	struct DLLRTUTIL MixClosureNode
		: public Node
	{
		MixClosureNode(CCLShader &shader,const std::string &nodeName,ccl::MixClosureNode &node);
		NumberSocket inFac;
		Socket inClosure1;
		Socket inClosure2;

		Socket outClosure;

		operator const Socket&() const;

		void SetFactor(float fac);
	private:
		ccl::MixClosureNode *m_node = nullptr;
	};
	struct DLLRTUTIL AddClosureNode
		: public Node
	{
		AddClosureNode(CCLShader &shader,const std::string &nodeName,ccl::AddClosureNode &node);
		Socket inClosure1;
		Socket inClosure2;

		Socket outClosure;

		operator const Socket&() const;
	private:
		ccl::AddClosureNode *m_node = nullptr;
	};
	struct DLLRTUTIL BackgroundNode
		: public Node
	{
		BackgroundNode(CCLShader &shader,const std::string &nodeName,ccl::BackgroundNode &node);
		Socket inColor;
		NumberSocket inStrength;
		NumberSocket inSurfaceMixWeight;

		Socket outBackground;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetStrength(float strength);
		void SetSurfaceMixWeight(float surfaceMixWeight);
	private:
		ccl::BackgroundNode *m_node = nullptr;
	};
	struct DLLRTUTIL TextureCoordinateNode
		: public Node
	{
		TextureCoordinateNode(CCLShader &shader,const std::string &nodeName,ccl::TextureCoordinateNode &node);
		Socket inNormal;
		Socket outGenerated;
		Socket outNormal;
		Socket outUv;
		Socket outObject;
		Socket outCamera;
		Socket outWindow;
		Socket outReflection;
	private:
		ccl::TextureCoordinateNode *m_node = nullptr;
	};
	struct DLLRTUTIL MappingNode
		: public Node
	{
		enum class Type : uint8_t
		{
			Point = 0u,
			Texture,
			Vector,
			Normal
		};

		MappingNode(CCLShader &shader,const std::string &nodeName,ccl::MappingNode &node);
		Socket inVector;
		Socket outVector;

		operator const Socket&() const;

		void SetType(Type type);
		void SetRotation(const EulerAngles &ang);
	private:
		ccl::MappingNode *m_node = nullptr;
	};
	struct DLLRTUTIL ScatterVolumeNode
		: public Node
	{
		ScatterVolumeNode(CCLShader &shader,const std::string &nodeName,ccl::ScatterVolumeNode &node);
		Socket inColor;
		NumberSocket inDensity;
		NumberSocket inAnisotropy;
		NumberSocket inVolumeMixWeight;

		Socket outVolume;

		operator const Socket&() const;
	private:
		ccl::ScatterVolumeNode *m_node = nullptr;
	};
	struct DLLRTUTIL EmissionNode
		: public Node
	{
		EmissionNode(CCLShader &shader,const std::string &nodeName,ccl::EmissionNode &node);
		Socket inColor;
		NumberSocket inStrength;
		NumberSocket inSurfaceMixWeight;
		Socket outEmission;

		void SetColor(const Vector3 &color);
		void SetStrength(float strength);

		operator const Socket&() const;
	private:
		ccl::EmissionNode *m_node = nullptr;
	};
	struct DLLRTUTIL ColorNode
		: public Node
	{
		ColorNode(CCLShader &shader,const std::string &nodeName,ccl::ColorNode &node);
		Socket outColor;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
	private:
		ccl::ColorNode *m_node = nullptr;
	};
	struct DLLRTUTIL AttributeNode
		: public Node
	{
		AttributeNode(CCLShader &shader,const std::string &nodeName,ccl::AttributeNode &node);
		Socket outColor;
		Socket outVector;
		Socket outFactor;

		void SetAttribute(ccl::AttributeStandard attrType);
	private:
		ccl::AttributeNode *m_node = nullptr;
	};
	struct DLLRTUTIL LightPathNode
		: public Node
	{
		LightPathNode(CCLShader &shader,const std::string &nodeName,ccl::LightPathNode &node);
		NumberSocket outIsCameraRay;
		NumberSocket outIsShadowRay;
		NumberSocket outIsDiffuseRay;
		NumberSocket outIsGlossyRay;
		NumberSocket outIsSingularRay;
		NumberSocket outIsReflectionRay;
		NumberSocket outIsTransmissionRay;
		NumberSocket outIsVolumeScatterRay;
		NumberSocket outRayLength;
		NumberSocket outRayDepth;
		NumberSocket outDiffuseDepth;
		NumberSocket outGlossyDepth;
		NumberSocket outTransparentDepth;
		NumberSocket outTransmissionDepth;
	};
	struct DLLRTUTIL MixNode
		: public Node
	{
		enum class Type : uint8_t
		{
			Mix = 0u,
			Add,
			Multiply,
			Screen,
			Overlay,
			Subtract,
			Divide,
			Difference,
			Darken,
			Lighten,
			Dodge,
			Burn,
			Hue,
			Saturation,
			Value,
			Color,
			SoftLight,
			LinearLight
		};

		MixNode(CCLShader &shader,const std::string &nodeName,ccl::MixNode &node);
		NumberSocket inFac;
		Socket inColor1;
		Socket inColor2;

		Socket outColor;

		operator const Socket&() const;

		void SetType(Type type);
		void SetUseClamp(bool useClamp);
		void SetFactor(float fac);
		void SetColor1(const Vector3 &color1);
		void SetColor2(const Vector3 &color2);
	private:
		ccl::MixNode *m_node = nullptr;
	};
	struct DLLRTUTIL TransparentBsdfNode
		: public Node
	{
		TransparentBsdfNode(CCLShader &shader,const std::string &nodeName,ccl::TransparentBsdfNode &node);
		Socket inColor;
		NumberSocket inSurfaceMixWeight;

		Socket outBsdf;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetSurfaceMixWeight(float weight);
	private:
		ccl::TransparentBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL TranslucentBsdfNode
		: public Node
	{
		TranslucentBsdfNode(CCLShader &shader,const std::string &nodeName,ccl::TranslucentBsdfNode &node);
		Socket inColor;
		Socket inNormal;
		NumberSocket inSurfaceMixWeight;

		Socket outBsdf;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetSurfaceMixWeight(float weight);
	private:
		ccl::TranslucentBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL DiffuseBsdfNode
		: public Node
	{
		DiffuseBsdfNode(CCLShader &shader,const std::string &nodeName,ccl::DiffuseBsdfNode &node);
		Socket inColor;
		Socket inNormal;
		NumberSocket inSurfaceMixWeight;
		NumberSocket inRoughness;

		Socket outBsdf;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetNormal(const Vector3 &normal);
		void SetSurfaceMixWeight(float weight);
		void SetRoughness(float roughness);
	private:
		ccl::DiffuseBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL NormalMapNode
		: public Node
	{
		enum class Space : uint8_t
		{
			Tangent = 0u,
			Object,
			World
		};

		NormalMapNode(CCLShader &shader,const std::string &nodeName,ccl::NormalMapNode &normalMapNode);

		NumberSocket inStrength;
		Socket inColor;

		Socket outNormal;

		operator const Socket&() const;

		void SetStrength(float strength);
		void SetColor(const Vector3 &color);
		void SetSpace(Space space);
		void SetAttribute(const std::string &attribute);
	private:
		ccl::NormalMapNode *m_node = nullptr;
	};
	struct DLLRTUTIL PrincipledBSDFNode
		: public Node
	{
		enum class Distribution : uint8_t
		{
			GGX = 0u,
			MultiscaterGGX
		};

		enum class SubsurfaceMethod : uint8_t
		{
			Cubic = 0,
			Gaussian,
			Principled,
			Burley,
			RandomWalk,
			PrincipledRandomWalk
		};

		PrincipledBSDFNode(CCLShader &shader,const std::string &nodeName,ccl::PrincipledBsdfNode &principledBSDF);
		Socket inBaseColor;
		Socket inSubsurfaceColor;	
		NumberSocket inMetallic;
		NumberSocket inSubsurface;
		Socket inSubsurfaceRadius;
		NumberSocket inSpecular;
		NumberSocket inRoughness;
		NumberSocket inSpecularTint;
		NumberSocket inAnisotropic;
		NumberSocket inSheen;
		NumberSocket inSheenTint;
		NumberSocket inClearcoat;
		NumberSocket inClearcoatRoughness;
		NumberSocket inIOR;
		NumberSocket inTransmission;
		NumberSocket inTransmissionRoughness;
		NumberSocket inAnisotropicRotation;
		Socket inEmission;
		NumberSocket inAlpha;
		Socket inNormal;
		Socket inClearcoatNormal;
		Socket inTangent;
		NumberSocket inSurfaceMixWeight;

		Socket outBSDF;

		operator const Socket&() const;

		void SetBaseColor(const Vector3 &color);
		void SetSubsurfaceColor(const Vector3 &color);
		void SetMetallic(float metallic);
		void SetSubsurface(float subsurface);
		void SetSubsurfaceRadius(const Vector3 &subsurfaceRadius);
		void SetSpecular(float specular);
		void SetRoughness(float roughness);
		void SetSpecularTint(float specularTint);
		void SetAnisotropic(float anisotropic);
		void SetSheen(float sheen);
		void SetSheenTint(float sheenTint);
		void SetClearcoat(float clearcoat);
		void SetClearcoatRoughness(float clearcoatRoughness);
		void SetIOR(float ior);
		void SetTransmission(float transmission);
		void SetTransmissionRoughness(float transmissionRoughness);
		void SetAnisotropicRotation(float anisotropicRotation);
		void SetEmission(const Vector3 &emission);
		void SetAlpha(float alpha);
		void SetNormal(const Vector3 &normal);
		void SetClearcoatNormal(const Vector3 &normal);
		void SetTangent(const Vector3 &tangent);
		void SetSurfaceMixWeight(float weight);

		void SetDistribution(Distribution distribution);
		void SetSubsurfaceMethod(SubsurfaceMethod method);
	private:
		ccl::PrincipledBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL ToonBSDFNode
		: public Node
	{
		enum class Component : uint8_t
		{
			Diffuse = 0u,
			Glossy
		};

		ToonBSDFNode(CCLShader &shader,const std::string &nodeName,ccl::ToonBsdfNode &toonBsdf);
		Socket inColor;
		Socket inNormal;
		NumberSocket inSurfaceMixWeight;
		NumberSocket inSize;
		NumberSocket inSmooth;

		Socket outBSDF;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetNormal(const Vector3 &normal);
		void SetSurfaceMixWeight(float weight);
		void SetSize(float size);
		void SetSmooth(float smooth);
		void SetComponent(Component component);
	private:
		ccl::ToonBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL GlassBSDFNode
		: public Node
	{
		enum class Distribution : uint8_t
		{
			Sharp = 0u,
			Beckmann,
			GGX,
			MultiscatterGGX
		};

		GlassBSDFNode(CCLShader &shader,const std::string &nodeName,ccl::GlassBsdfNode &toonBsdf);
		Socket inColor;
		Socket inNormal;
		NumberSocket inSurfaceMixWeight;
		NumberSocket inRoughness;
		NumberSocket inIOR;

		Socket outBSDF;

		operator const Socket&() const;

		void SetColor(const Vector3 &color);
		void SetNormal(const Vector3 &normal);
		void SetSurfaceMixWeight(float weight);
		void SetRoughness(float roughness);
		void SetIOR(float ior);
		void SetDistribution(Distribution distribution);
	private:
		ccl::GlassBsdfNode *m_node = nullptr;
	};
	struct DLLRTUTIL OutputNode
		: public Node
	{
		OutputNode(CCLShader &shader,const std::string &nodeName);
		Socket inSurface;
		Socket inVolume;
		Socket inDisplacement;
		Socket inNormal;
	};
};

#endif