/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#include "util_raytracing/mesh.hpp"
#include "util_raytracing/scene.hpp"
#include "util_raytracing/shader.hpp"
#include "util_raytracing/ccl_shader.hpp"
#include <sharedutils/datastream.h>
#include <render/mesh.h>
#include <render/scene.h>

#pragma optimize("",off)
static const std::string TANGENT_POSTFIX = ".tangent";
static const std::string TANGENT_SIGN_POSTIFX = ".tangent_sign";
raytracing::PMesh raytracing::Mesh::Create(Scene &scene,const std::string &name,uint64_t numVerts,uint64_t numTris,Flags flags)
{
	auto *mesh = new ccl::Mesh{}; // Object will be removed automatically by cycles
	mesh->name = name;
	auto *attrN = mesh->attributes.add(ccl::ATTR_STD_VERTEX_NORMAL);
	if(attrN)
		attrN->resize(numVerts);

	auto *attrUV = mesh->attributes.add(ccl::ATTR_STD_UV);
	if(attrUV)
		attrUV->resize(numTris *3);

	auto *attrT = mesh->attributes.add(ccl::ATTR_STD_UV_TANGENT);
	if(attrT)
	{
		attrT->resize(numTris *3);
		attrT->name = name +TANGENT_POSTFIX;
	}

	auto *attrTS = mesh->attributes.add(ccl::ATTR_STD_UV_TANGENT_SIGN);
	if(attrTS)
	{
		attrTS->resize(numTris *3);
		attrTS->name = name +TANGENT_SIGN_POSTIFX;
	}

	if(umath::is_flag_set(flags,Flags::HasAlphas) || umath::is_flag_set(flags,Flags::HasWrinkles))
	{
		auto *attrAlpha = mesh->attributes.add(Mesh::ALPHA_ATTRIBUTE_TYPE);
		attrAlpha->resize(numVerts);
	}

	// TODO: Add support for hair/curves

	mesh->reserve_mesh(numVerts,numTris);
	scene->meshes.push_back(mesh);
	auto meshWrapper = PMesh{new Mesh{scene,*mesh,numVerts,numTris,flags}};
	meshWrapper->m_perVertexUvs.reserve(numVerts);
	meshWrapper->m_perVertexTangents.reserve(numVerts);
	meshWrapper->m_perVertexTangentSigns.reserve(numVerts);
	meshWrapper->m_perVertexAlphas.reserve(numVerts);

	auto &meshes = scene.GetMeshes();
	if(meshes.size() == meshes.capacity())
		meshes.reserve(meshes.size() +100);
	meshes.push_back(meshWrapper);
	return meshWrapper;
}

raytracing::PMesh raytracing::Mesh::Create(Scene &scene,DataStream &dsIn)
{
	auto name = dsIn->ReadString();
	auto flags = dsIn->Read<decltype(m_flags)>();
	auto numVerts = dsIn->Read<decltype(m_numVerts)>();
	auto numTris = dsIn->Read<decltype(m_numTris)>();
	auto mesh = Create(scene,name,numVerts,numTris,flags);
	mesh->Deserialize(dsIn);
	return mesh;
}

raytracing::Mesh::Mesh(Scene &scene,ccl::Mesh &mesh,uint64_t numVerts,uint64_t numTris,Flags flags)
	: SceneObject{scene},m_mesh{mesh},m_numVerts{numVerts},
	m_numTris{numTris},m_flags{flags}
{
	auto *normals = m_mesh.attributes.find(ccl::ATTR_STD_VERTEX_NORMAL);
	m_normals = normals ? normals->data_float4() : nullptr;

	auto *uvs = m_mesh.attributes.find(ccl::ATTR_STD_UV);
	m_uvs = uvs ? uvs->data_float2() : nullptr;

	auto *tangents = m_mesh.attributes.find(ccl::ATTR_STD_UV_TANGENT);
	m_tangents = tangents ? tangents->data_float4() : nullptr;

	auto *tangentSigns = m_mesh.attributes.find(ccl::ATTR_STD_UV_TANGENT_SIGN);
	m_tangentSigns = tangentSigns ? tangentSigns->data_float() : nullptr;

	if(HasAlphas() || HasWrinkles())
	{
		// Note: There's no option to supply user-data for vertices in Cycles, so we're (ab)using ATTR_STD_POINTINESS arbitrarily,
		// which is currently only used for Fluid Domain in Cycles, which we don't use anyway (State: 2020-02-25). This may change in the future!
		auto *alphas = m_mesh.attributes.find(ALPHA_ATTRIBUTE_TYPE);
		m_alphas = alphas ? alphas->data_float() : nullptr;
		if(alphas)
		{
			// Clear alpha values to 0
			for(auto i=decltype(numVerts){0u};i<numVerts;++i)
				m_alphas[i] = 0.f;
		}
	}
}

util::WeakHandle<raytracing::Mesh> raytracing::Mesh::GetHandle()
{
	return util::WeakHandle<raytracing::Mesh>{shared_from_this()};
}

enum class SerializationFlags : uint8_t
{
	None = 0u,
	UseAlphas = 1u,
	UseSubdivFaces = UseAlphas<<1u
};
REGISTER_BASIC_BITWISE_OPERATORS(SerializationFlags)
void raytracing::Mesh::Serialize(DataStream &dsOut) const
{
	auto numVerts = umath::min(m_numVerts,m_mesh.verts.size());
	auto numTris = umath::min(m_numTris,m_mesh.triangles.size());
	dsOut->WriteString(m_mesh.name.c_str());
	dsOut->Write(m_flags);
	dsOut->Write(numVerts);
	dsOut->Write(numTris);

	auto flags = SerializationFlags::None;
	if(m_alphas)
		flags |= SerializationFlags::UseAlphas;
	if(m_mesh.subd_faces.empty() == false)
		flags |= SerializationFlags::UseSubdivFaces;

	dsOut->Write<SerializationFlags>(flags);
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.verts.data()),numVerts *sizeof(m_mesh.verts[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexUvs.data()),numVerts *sizeof(m_perVertexUvs.front()));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexTangents.data()),numVerts *sizeof(m_perVertexTangents.front()));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexTangentSigns.data()),numVerts *sizeof(m_perVertexTangentSigns.front()));
	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexAlphas.data()),numVerts *sizeof(m_perVertexAlphas.front()));

	if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.subd_faces.data()),numVerts *sizeof(m_mesh.subd_faces[0]));

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.triangles.data()),numTris *3 *sizeof(m_mesh.triangles[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.shader.data()),numTris *sizeof(m_mesh.shader[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.smooth.data()),numTris *sizeof(m_mesh.smooth[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.triangle_patch.data()),numTris *sizeof(m_mesh.triangle_patch[0]));

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_normals),numVerts *sizeof(m_normals[0]));

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_uvs),numTris *3 *sizeof(m_uvs[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_tangents),numTris *3 *sizeof(m_tangents[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_tangentSigns),numTris *3 *sizeof(m_tangentSigns[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_alphas),numVerts *sizeof(m_alphas[0]));

	dsOut->Write<size_t>(m_subMeshShaders.size());
	for(auto &shader : m_subMeshShaders)
	{
		auto &shaders = GetScene().GetShaders();
		auto it = std::find(shaders.begin(),shaders.end(),shader);
		assert(it != shaders.end());
		dsOut->Write<uint32_t>(it -shaders.begin());
	}

	dsOut->Write<size_t>(m_lightmapUvs.size());
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_lightmapUvs.data()),m_lightmapUvs.size() *sizeof(m_lightmapUvs.front()));
}
void raytracing::Mesh::Deserialize(DataStream &dsIn)
{
	auto flags = dsIn->Read<SerializationFlags>();

	m_mesh.verts.resize(m_numVerts);
	dsIn->Read(m_mesh.verts.data(),m_numVerts *sizeof(m_mesh.verts[0]));

	m_perVertexUvs.resize(m_numVerts);
	dsIn->Read(m_perVertexUvs.data(),m_numVerts *sizeof(m_perVertexUvs.front()));
	m_perVertexTangents.resize(m_numVerts);
	dsIn->Read(m_perVertexTangents.data(),m_numVerts *sizeof(m_perVertexTangents.front()));
	m_perVertexTangentSigns.resize(m_numVerts);
	dsIn->Read(m_perVertexTangentSigns.data(),m_numVerts *sizeof(m_perVertexTangentSigns.front()));
	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
	{
		m_perVertexAlphas.resize(m_numVerts);
		dsIn->Read(m_perVertexAlphas.data(),m_numVerts *sizeof(m_perVertexAlphas.front()));
	}

	if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	{
		m_mesh.subd_faces.resize(m_numVerts);
		dsIn->Read(m_mesh.subd_faces.data(),m_numVerts *sizeof(m_mesh.subd_faces[0]));
	}

	m_mesh.triangles.resize(m_numTris *3);
	dsIn->Read(m_mesh.triangles.data(),m_numTris *3 *sizeof(m_mesh.triangles[0]));
	m_mesh.shader.resize(m_numTris);
	dsIn->Read(m_mesh.shader.data(),m_numTris *sizeof(m_mesh.shader[0]));
	m_mesh.smooth.resize(m_numTris);
	dsIn->Read(m_mesh.smooth.data(),m_numTris *sizeof(m_mesh.smooth[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	{
		m_mesh.triangle_patch.resize(m_numTris);
		dsIn->Read(m_mesh.triangle_patch.data(),m_numTris *sizeof(m_mesh.triangle_patch[0]));
	}

	m_mesh.attributes.find(ccl::ATTR_STD_VERTEX_NORMAL)->resize(m_numVerts);
	dsIn->Read(m_normals,m_numVerts *sizeof(m_normals[0]));

	m_mesh.attributes.find(ccl::ATTR_STD_UV)->resize(m_numTris *3);
	dsIn->Read(m_uvs,m_numTris *3 *sizeof(m_uvs[0]));

	m_mesh.attributes.find(ccl::ATTR_STD_UV_TANGENT)->resize(m_numTris *3);
	dsIn->Read(m_tangents,m_numTris *3 *sizeof(m_tangents[0]));

	m_mesh.attributes.find(ccl::ATTR_STD_UV_TANGENT_SIGN)->resize(m_numTris *3);
	dsIn->Read(m_tangentSigns,m_numTris *3 *sizeof(m_tangentSigns[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
	{
		m_mesh.attributes.find(ALPHA_ATTRIBUTE_TYPE)->resize(m_numVerts);
		dsIn->Read(m_alphas,m_numVerts *sizeof(m_alphas[0]));
	}

	auto numMeshShaders = dsIn->Read<size_t>();
	m_subMeshShaders.resize(numMeshShaders);
	for(auto i=decltype(m_subMeshShaders.size()){0u};i<m_subMeshShaders.size();++i)
	{
		auto shaderIdx = dsIn->Read<uint32_t>();
		auto &shaders = GetScene().GetShaders();
		assert(shaderIdx < shaders.size());
		m_subMeshShaders.at(i) = shaders.at(shaderIdx);
	}

	auto numLightmapUvs = dsIn->Read<size_t>();
	m_lightmapUvs.resize(numLightmapUvs);
	dsIn->Read(m_lightmapUvs.data(),m_lightmapUvs.size() *sizeof(m_lightmapUvs.front()));
}

void raytracing::Mesh::DoFinalize()
{
	SceneObject::DoFinalize();
	auto &shaders = GetSubMeshShaders();
	(*this)->used_shaders.resize(shaders.size());
	for(auto i=decltype(shaders.size()){0u};i<shaders.size();++i)
	{
		auto cclShader = shaders.at(i)->GenerateCCLShader();
		if(cclShader == nullptr)
			throw std::logic_error{"Mesh shader must never be NULL!"};
		if(cclShader)
			(*this)->used_shaders.at(i) = **cclShader;
	}
}

const ccl::float4 *raytracing::Mesh::GetNormals() const {return m_normals;}
const ccl::float4 *raytracing::Mesh::GetTangents() const {return m_tangents;}
const float *raytracing::Mesh::GetTangentSigns() const {return m_tangentSigns;}
const float *raytracing::Mesh::GetAlphas() const {return m_alphas;}
const float *raytracing::Mesh::GetWrinkleFactors() const {return GetAlphas();}
const ccl::float2 *raytracing::Mesh::GetUVs() const {return m_uvs;}
const ccl::float2 *raytracing::Mesh::GetLightmapUVs() const {return m_lightmapUvs.data();}
void raytracing::Mesh::SetLightmapUVs(std::vector<ccl::float2> &&lightmapUvs) {m_lightmapUvs = std::move(lightmapUvs);}
const std::vector<raytracing::PShader> &raytracing::Mesh::GetSubMeshShaders() const {return const_cast<Mesh*>(this)->GetSubMeshShaders();}
std::vector<raytracing::PShader> &raytracing::Mesh::GetSubMeshShaders() {return m_subMeshShaders;}
uint64_t raytracing::Mesh::GetVertexCount() const {return m_numVerts;}
uint64_t raytracing::Mesh::GetTriangleCount() const {return m_numTris;}
uint32_t raytracing::Mesh::GetVertexOffset() const {return m_mesh.verts.size();}
std::string raytracing::Mesh::GetName() const {return m_mesh.name.string();}
bool raytracing::Mesh::HasAlphas() const {return umath::is_flag_set(m_flags,Flags::HasAlphas);}
bool raytracing::Mesh::HasWrinkles() const {return umath::is_flag_set(m_flags,Flags::HasWrinkles);}

static ccl::float4 to_float4(const ccl::float3 &v)
{
	return ccl::float4{v.x,v.y,v.z,0.f};
}

bool raytracing::Mesh::AddVertex(const Vector3 &pos,const Vector3 &n,const Vector3 &t,const Vector2 &uv)
{
	auto idx = m_mesh.verts.size();
	if(idx >= m_numVerts)
		return false;
	if(m_normals)
		m_normals[idx] = to_float4(Scene::ToCyclesNormal(n));
	m_mesh.add_vertex(Scene::ToCyclesPosition(pos));
	m_perVertexUvs.push_back(uv);
	m_perVertexTangents.push_back(t);
	return true;
}

bool raytracing::Mesh::AddAlpha(float alpha)
{
	if(HasAlphas() == false)
		return false;
	m_alphas[m_perVertexAlphas.size()] = alpha;
	m_perVertexAlphas.push_back(alpha);
	return true;
}

bool raytracing::Mesh::AddWrinkleFactor(float factor)
{
	if(HasWrinkles() == false)
		return false;
	m_alphas[m_perVertexAlphas.size()] = factor;
	m_perVertexAlphas.push_back(factor);
	return true;
}

bool raytracing::Mesh::AddTriangle(uint32_t idx0,uint32_t idx1,uint32_t idx2,uint32_t shaderIndex)
{
	// Winding order has to be inverted for cycles
#ifndef ENABLE_TEST_AMBIENT_OCCLUSION
	umath::swap(idx1,idx2);
#endif

	auto numCurMeshTriIndices = m_mesh.triangles.size();
	auto idx = numCurMeshTriIndices /3;
	if(idx >= m_numTris)
		return false;
	constexpr auto smooth = true;
	m_mesh.add_triangle(idx0,idx1,idx2,shaderIndex,smooth);

	if(m_uvs == nullptr)
		return false;
	if(idx0 >= m_perVertexUvs.size() || idx1 >= m_perVertexUvs.size() || idx2 >= m_perVertexUvs.size())
		return false;
	auto &uv0 = m_perVertexUvs.at(idx0);
	auto &uv1 = m_perVertexUvs.at(idx1);
	auto &uv2 = m_perVertexUvs.at(idx2);
	auto offset = numCurMeshTriIndices;
	m_uvs[offset] = Scene::ToCyclesUV(uv0);
	m_uvs[offset +1] = Scene::ToCyclesUV(uv1);
	m_uvs[offset +2] = Scene::ToCyclesUV(uv2);

	auto &t0 = m_perVertexTangents.at(idx0);
	auto &t1 = m_perVertexTangents.at(idx1);
	auto &t2 = m_perVertexTangents.at(idx2);
	m_tangents[offset] = to_float4(Scene::ToCyclesNormal(t0));
	m_tangents[offset +1] = to_float4(Scene::ToCyclesNormal(t1));
	m_tangents[offset +2] = to_float4(Scene::ToCyclesNormal(t2));

	m_tangentSigns[offset] = m_tangentSigns[offset +1] = m_tangentSigns[offset +2] = 1.f;
	/*if((HasAlphas() || HasWrinkles()) && idx0 < m_perVertexAlphas.size() && idx1 < m_perVertexAlphas.size() && idx2 < m_perVertexAlphas.size())
	{
	m_alphas[offset] = m_perVertexAlphas.at(idx0);
	m_alphas[offset +1] = m_perVertexAlphas.at(idx1);
	m_alphas[offset +2] = m_perVertexAlphas.at(idx2);
	}*/
	return true;
}

uint32_t raytracing::Mesh::AddSubMeshShader(Shader &shader)
{
	m_subMeshShaders.push_back(shader.shared_from_this());
	return m_subMeshShaders.size() -1;
}

ccl::Mesh *raytracing::Mesh::operator->() {return &m_mesh;}
ccl::Mesh *raytracing::Mesh::operator*() {return &m_mesh;}
#pragma optimize("",on)
