/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#include "util_raytracing/mesh.hpp"
#include "util_raytracing/scene.hpp"
#include "util_raytracing/shader.hpp"
#include "util_raytracing/model_cache.hpp"
#include <sharedutils/datastream.h>

#pragma optimize("",off)
const std::string unirender::Mesh::TANGENT_POSTFIX = ".tangent";
const std::string unirender::Mesh::TANGENT_SIGN_POSTIFX = ".tangent_sign";
unirender::PMesh unirender::Mesh::Create(const std::string &name,uint64_t numVerts,uint64_t numTris,Flags flags)
{
	auto meshWrapper = PMesh{new Mesh{numVerts,numTris,flags}};
	meshWrapper->SetName(name);
	meshWrapper->m_vertexNormals.resize(numVerts);
	meshWrapper->m_uvs.resize(numTris *3);
	meshWrapper->m_uvTangents.resize(numTris *3);
	meshWrapper->m_uvTangentSigns.resize(numTris *3);
	
	meshWrapper->m_verts.reserve(numVerts);
	meshWrapper->m_triangles.reserve(numTris *3);
	meshWrapper->m_shader.reserve(numTris);
	meshWrapper->m_smooth.reserve(numTris);

	if(umath::is_flag_set(flags,Flags::HasAlphas) || umath::is_flag_set(flags,Flags::HasWrinkles))
	{
		// Note: There's no option to supply user-data for vertices in Cycles, so we're (ab)using ATTR_STD_POINTINESS arbitrarily,
		// which is currently only used for Fluid Domain in Cycles, which we don't use anyway (State: 2020-02-25). This may change in the future!
		meshWrapper->m_alphas = std::vector<float>{};
		meshWrapper->m_alphas->resize(numVerts);
	}

	// TODO: Add support for hair/curves

	meshWrapper->m_perVertexUvs.reserve(numVerts);
	meshWrapper->m_perVertexTangents.reserve(numVerts);
	meshWrapper->m_perVertexTangentSigns.reserve(numVerts);
	meshWrapper->m_perVertexAlphas.reserve(numVerts);
	return meshWrapper;
}

unirender::PMesh unirender::Mesh::Create(DataStream &dsIn,const std::function<PShader(uint32_t)> &fGetShader)
{
	auto name = dsIn->ReadString();
	auto flags = dsIn->Read<decltype(m_flags)>();
	auto numVerts = dsIn->Read<decltype(m_numVerts)>();
	auto numTris = dsIn->Read<decltype(m_numTris)>();
	auto mesh = Create(name,numVerts,numTris,flags);
	mesh->Deserialize(dsIn,fGetShader);
	return mesh;
}

unirender::PMesh unirender::Mesh::Create(DataStream &dsIn,const ShaderCache &cache)
{
	auto &shaders = cache.GetShaders();
	return Create(dsIn,[&shaders](uint32_t idx) -> PShader {
		return (idx < shaders.size()) ? shaders.at(idx) : nullptr;
	});
}

unirender::Mesh::Mesh(uint64_t numVerts,uint64_t numTris,Flags flags)
	: m_numVerts{numVerts},
	m_numTris{numTris},m_flags{flags}
{
	if(HasAlphas() || HasWrinkles())
	{
		// Note: There's no option to supply user-data for vertices in Cycles, so we're (ab)using ATTR_STD_POINTINESS arbitrarily,
		// which is currently only used for Fluid Domain in Cycles, which we don't use anyway (State: 2020-02-25). This may change in the future!
		if(m_alphas)
		{
			// Clear alpha values to 0
			for(auto i=decltype(numVerts){0u};i<numVerts;++i)
				(*m_alphas)[i] = 0.f;
		}
	}
}

util::WeakHandle<unirender::Mesh> unirender::Mesh::GetHandle()
{
	return util::WeakHandle<unirender::Mesh>{shared_from_this()};
}

enum class SerializationFlags : uint8_t
{
	None = 0u,
	UseAlphas = 1u,
	UseSubdivFaces = UseAlphas<<1u
};
REGISTER_BASIC_BITWISE_OPERATORS(SerializationFlags)
void unirender::Mesh::Serialize(DataStream &dsOut,const std::function<std::optional<uint32_t>(const Shader&)> &fGetShaderIndex) const
{
	auto numVerts = umath::min(m_numVerts,m_verts.size());
	auto numTris = umath::min(m_numTris,m_triangles.size() /3);
	dsOut->WriteString(GetName());
	dsOut->Write(m_flags);
	dsOut->Write<decltype(m_numVerts)>(numVerts);
	dsOut->Write<decltype(m_numTris)>(numTris);

	auto flags = SerializationFlags::None;
	if(m_alphas)
		flags |= SerializationFlags::UseAlphas;
	if(m_numSubdFaces > 0)
		flags |= SerializationFlags::UseSubdivFaces;

	dsOut->Write<SerializationFlags>(flags);
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_verts.data()),numVerts *sizeof(m_verts.front()));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexUvs.data()),numVerts *sizeof(m_perVertexUvs.front()));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexTangents.data()),numVerts *sizeof(m_perVertexTangents.front()));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexTangentSigns.data()),numVerts *sizeof(m_perVertexTangentSigns.front()));
	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_perVertexAlphas.data()),numVerts *sizeof(m_perVertexAlphas.front()));

	/*if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	{
		auto numSubdFaces = m_numSubdFaces;
		assert(numSubdFaces == m_numVerts);
		if(numSubdFaces != m_numVerts)
			throw std::logic_error{"Subd face count mismatch!"};
		dsOut->Write<uint32_t>(m_numSubdFaces);
		dsOut->Write<uint32_t>(m_numNGons);
		auto &numCorners = m_mesh.get_subd_num_corners();
		dsOut->Write<uint32_t>(numCorners.size());
		dsOut->Write(reinterpret_cast<uint8_t*>(numCorners.data()),numCorners.size() *sizeof(numCorners[0]));
		std::vector<ccl::Mesh::SubdFace> subdFaces;
		subdFaces.resize(numSubdFaces);
		for(auto i=decltype(numSubdFaces){0u};i<numSubdFaces;++i)
			subdFaces[i] = m_mesh.get_subd_face(i);
		dsOut->Write(reinterpret_cast<const uint8_t*>(subdFaces.data()),numVerts *sizeof(subdFaces[0]));
	}
	*/
	// Validate();

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_triangles.data()),numTris *3 *sizeof(m_triangles[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_shader.data()),numTris *sizeof(m_shader[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_smooth.data()),numTris *sizeof(m_smooth[0]));

	//if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	//	dsOut->Write(reinterpret_cast<const uint8_t*>(m_mesh.get_triangle_patch().data()),numTris *sizeof(m_mesh.get_triangle_patch()[0]));

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_vertexNormals.data()),numVerts *sizeof(m_vertexNormals[0]));

	dsOut->Write(reinterpret_cast<const uint8_t*>(m_uvs.data()),numTris *3 *sizeof(m_uvs[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_uvTangents.data()),numTris *3 *sizeof(m_uvTangents[0]));
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_uvTangentSigns.data()),numTris *3 *sizeof(m_uvTangentSigns[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
		dsOut->Write(reinterpret_cast<const uint8_t*>(m_alphas->data()),numVerts *sizeof((*m_alphas)[0]));

	dsOut->Write<size_t>(m_subMeshShaders.size());
	for(auto &shader : m_subMeshShaders)
	{
		auto idx = fGetShaderIndex(*shader);
		assert(idx.has_value());
		dsOut->Write<uint32_t>(*idx);
	}

	dsOut->Write<size_t>(m_lightmapUvs.size());
	dsOut->Write(reinterpret_cast<const uint8_t*>(m_lightmapUvs.data()),m_lightmapUvs.size() *sizeof(m_lightmapUvs.front()));
}
void unirender::Mesh::Serialize(DataStream &dsOut,const std::unordered_map<const Shader*,size_t> shaderToIndexTable) const
{
	Serialize(dsOut,[&shaderToIndexTable](const Shader &shader) -> std::optional<uint32_t> {
		auto it = shaderToIndexTable.find(&shader);
		return (it != shaderToIndexTable.end()) ? it->second : std::optional<uint32_t>{};
	});
}
void unirender::Mesh::Deserialize(DataStream &dsIn,const std::function<PShader(uint32_t)> &fGetShader)
{
	auto flags = dsIn->Read<SerializationFlags>();

	m_verts.resize(m_numVerts);
	dsIn->Read(m_verts.data(),m_numVerts *sizeof(m_verts[0]));

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

	/*if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	{
		auto numSubdFaces = dsIn->Read<uint32_t>();
		auto numNgons = dsIn->Read<uint32_t>();
		auto numCorners = dsIn->Read<uint32_t>();
		m_mesh.resize_subd_faces(m_numVerts,numNgons,numCorners);
		auto &numSubdCorners = m_mesh.get_subd_num_corners();
		numSubdCorners.resize(numCorners);
		dsIn->Read(numSubdCorners.data(),numSubdCorners.size() *sizeof(numSubdCorners[0]));
		std::vector<ccl::Mesh::SubdFace> subdFaces;
		subdFaces.resize(m_numVerts);
		dsIn->Read(subdFaces.data(),m_numVerts *sizeof(subdFaces[0]));
		ccl::array<int> startCorners;
		ccl::array<int> shaders;
		ccl::array<bool> smooth;
		ccl::array<int> ptexOffsets;
		startCorners.reserve(subdFaces.size());
		startCorners.reserve(shaders.size());
		startCorners.reserve(smooth.size());
		startCorners.reserve(ptexOffsets.size());
		for(auto i=decltype(subdFaces.size()){0u};i<subdFaces.size();++i)
		{
			auto &subdFace = subdFaces.at(i);
			startCorners.push_back_reserved(subdFace.start_corner);
			startCorners.push_back_reserved(subdFace.num_corners);
			startCorners.push_back_reserved(subdFace.shader);
			startCorners.push_back_reserved(subdFace.smooth);
			startCorners.push_back_reserved(subdFace.ptex_offset);
		}
		m_mesh.set_subd_start_corner(startCorners);
		m_mesh.set_subd_shader(shaders);
		m_mesh.set_subd_smooth(smooth);
		m_mesh.set_subd_ptex_offset(ptexOffsets);
	}*/

	m_triangles.resize(m_numTris *3);
	dsIn->Read(m_triangles.data(),m_numTris *3 *sizeof(m_triangles[0]));
	m_shader.resize(m_numTris);
	dsIn->Read(m_shader.data(),m_numTris *sizeof(m_shader[0]));
	m_smooth.resize(m_numTris);
	dsIn->Read(m_smooth.data(),m_numTris *sizeof(m_smooth[0]));

	// Validate();

	/*if(umath::is_flag_set(flags,SerializationFlags::UseSubdivFaces))
	{
		m_mesh.get_triangle_patch().resize(m_numTris);
		dsIn->Read(m_mesh.get_triangle_patch().data(),m_numTris *sizeof(m_mesh.get_triangle_patch()[0]));
	}*/

	m_vertexNormals.resize(m_numVerts);
	dsIn->Read(m_vertexNormals.data(),m_numVerts *sizeof(m_vertexNormals[0]));

	m_uvs.resize(m_numTris *3);
	dsIn->Read(m_uvs.data(),m_numTris *3 *sizeof(m_uvs[0]));

	m_uvTangents.resize(m_numTris *3);
	dsIn->Read(m_uvTangents.data(),m_numTris *3 *sizeof(m_uvTangents[0]));

	m_uvTangentSigns.resize(m_numTris *3);
	dsIn->Read(m_uvTangentSigns.data(),m_numTris *3 *sizeof(m_uvTangentSigns[0]));

	if(umath::is_flag_set(flags,SerializationFlags::UseAlphas))
	{
		m_alphas = std::vector<float>{};
		m_alphas->resize(m_numVerts);
		dsIn->Read(m_alphas->data(),m_numVerts *sizeof((*m_alphas)[0]));
	}

	auto numMeshShaders = dsIn->Read<size_t>();
	m_subMeshShaders.resize(numMeshShaders);
	for(auto i=decltype(m_subMeshShaders.size()){0u};i<m_subMeshShaders.size();++i)
	{
		auto shaderIdx = dsIn->Read<uint32_t>();
		auto shader = fGetShader(shaderIdx);
		assert(shader);
		m_subMeshShaders.at(i) = shader;
	}

	auto numLightmapUvs = dsIn->Read<size_t>();
	m_lightmapUvs.resize(numLightmapUvs);
	dsIn->Read(m_lightmapUvs.data(),m_lightmapUvs.size() *sizeof(m_lightmapUvs.front()));
}

template<typename T>
	static void merge_containers(std::vector<T> &a,const std::vector<T> &b)
{
	a.reserve(a.size() +b.size());
	for(auto &v : b)
		a.push_back(v);
}
void unirender::Mesh::Merge(const Mesh &other)
{
	auto vertexOffset = m_numVerts;
	auto triOffset = m_numTris;
	auto indexOffset = triOffset *3;
	auto subMeshShaderOffset = m_subMeshShaders.size();
	auto smoothOffset = m_smooth.size();
	auto shaderOffset =m_shader.size();

	m_numVerts += other.m_numVerts;
	m_numTris += other.m_numTris;

	auto *attrN = m_vertexNormals.data();
	auto *attrUvTangent = m_uvTangents.data();
	auto *attrTs = m_uvTangentSigns.data();
	auto *attrUv = m_uvs.data();
	if(attrN)
		m_vertexNormals.resize(m_numVerts);
	if(attrUvTangent)
		m_uvTangents.resize(m_numTris *3);
	if(attrTs)
		m_uvTangentSigns.resize(m_numTris *3);
	if(attrUv)
		m_uvs.resize(m_numTris *3);
	m_triangles.resize(m_numTris *3);
	m_smooth.resize(m_smooth.size() +other.m_smooth.size());
	m_shader.resize(m_shader.size() +other.m_shader.size());

	if(attrN)
	{
		for(auto i=vertexOffset;i<m_numVerts;++i)
			m_vertexNormals[i] = other.m_vertexNormals[i -vertexOffset];
	}
	
	if(attrUvTangent)
	{
		for(auto i=indexOffset;i<m_numTris *3;++i)
			m_uvTangents[i] = other.m_uvTangents[i -indexOffset];
	}
	
	if(attrTs)
	{
		for(auto i=indexOffset;i<m_numTris *3;++i)
			m_uvTangentSigns[i] = other.m_uvTangentSigns[i -indexOffset];
	}
	
	if(attrUv)
	{
		for(auto i=indexOffset;i<m_numTris *3;++i)
			m_uvs[i] = other.m_uvs[i -indexOffset];
	}
	
	for(auto i=indexOffset;i<m_numTris *3;++i)
		m_triangles[i] = other.m_triangles[i -indexOffset];

	for(auto i=indexOffset;i<m_triangles.size();++i)
		m_triangles[i] += vertexOffset;

	// TODO: m_alphas

	merge_containers(m_perVertexUvs,other.m_perVertexUvs);
	merge_containers(m_perVertexTangents,other.m_perVertexTangents);
	merge_containers(m_perVertexTangentSigns,other.m_perVertexTangentSigns);
	merge_containers(m_perVertexAlphas,other.m_perVertexAlphas);
	merge_containers(m_lightmapUvs,other.m_lightmapUvs);
	merge_containers(m_subMeshShaders,other.m_subMeshShaders);

	for(auto i=smoothOffset;i<m_smooth.size();++i)
		m_smooth[i] = other.m_smooth[i -smoothOffset];

	for(auto i=shaderOffset;i<m_shader.size();++i)
		m_shader[i] = other.m_shader[i -shaderOffset] +subMeshShaderOffset;
}

/*const ccl::float4 *unirender::Mesh::GetNormals() const {return m_vertexNormals.data();}
const ccl::float3 *unirender::Mesh::GetTangents() const {return m_uvTangents.data();}
const float *unirender::Mesh::GetTangentSigns() const {return m_uvTangentSigns.data();}
const float *unirender::Mesh::GetAlphas() const {return m_alphas.has_value() ? m_alphas->data() : nullptr;}
const float *unirender::Mesh::GetWrinkleFactors() const {return GetAlphas();}
const ccl::float2 *unirender::Mesh::GetUVs() const {return m_uvs.data();}
const ccl::float2 *unirender::Mesh::GetLightmapUVs() const {return m_lightmapUvs.data();}*/
void unirender::Mesh::SetLightmapUVs(std::vector<Vector2> &&lightmapUvs) {m_lightmapUvs = std::move(lightmapUvs);}
const std::vector<unirender::PShader> &unirender::Mesh::GetSubMeshShaders() const {return const_cast<Mesh*>(this)->GetSubMeshShaders();}
std::vector<unirender::PShader> &unirender::Mesh::GetSubMeshShaders() {return m_subMeshShaders;}
uint64_t unirender::Mesh::GetVertexCount() const {return m_numVerts;}
uint64_t unirender::Mesh::GetTriangleCount() const {return m_numTris;}
uint32_t unirender::Mesh::GetVertexOffset() const {return m_verts.size();}
bool unirender::Mesh::HasAlphas() const {return umath::is_flag_set(m_flags,Flags::HasAlphas);}
bool unirender::Mesh::HasWrinkles() const {return umath::is_flag_set(m_flags,Flags::HasWrinkles);}

bool unirender::Mesh::AddVertex(const Vector3 &pos,const Vector3 &n,const Vector4 &t,const Vector2 &uv)
{
	auto idx = m_verts.size();
	if(idx >= m_numVerts)
		return false;
	m_vertexNormals[idx] = n;
	m_verts.push_back(pos);

	m_perVertexUvs.push_back(uv);
	m_perVertexTangents.push_back(t);
	return true;
}

bool unirender::Mesh::AddAlpha(float alpha)
{
	if(HasAlphas() == false)
		return false;
	(*m_alphas)[m_perVertexAlphas.size()] = alpha;
	m_perVertexAlphas.push_back(alpha);
	return true;
}

bool unirender::Mesh::AddWrinkleFactor(float factor)
{
	if(HasWrinkles() == false)
		return false;
	(*m_alphas)[m_perVertexAlphas.size()] = factor;
	m_perVertexAlphas.push_back(factor);
	return true;
}

bool unirender::Mesh::AddTriangle(uint32_t idx0,uint32_t idx1,uint32_t idx2,uint32_t shaderIndex)
{
	// Winding order has to be inverted for cycles
#ifndef ENABLE_TEST_AMBIENT_OCCLUSION
	umath::swap(idx1,idx2);
#endif

	auto numCurMeshTriIndices = m_triangles.size();
	auto idx = numCurMeshTriIndices /3;
	if(idx >= m_numTris)
		return false;
	constexpr auto smooth = true;
	m_triangles.push_back(idx0);
	m_triangles.push_back(idx1);
	m_triangles.push_back(idx2);
	m_shader.push_back(shaderIndex);
	m_smooth.push_back(smooth);

	if(idx0 >= m_perVertexUvs.size() || idx1 >= m_perVertexUvs.size() || idx2 >= m_perVertexUvs.size())
		return false;
	auto &uv0 = m_perVertexUvs.at(idx0);
	auto &uv1 = m_perVertexUvs.at(idx1);
	auto &uv2 = m_perVertexUvs.at(idx2);
	auto offset = numCurMeshTriIndices;
	m_uvs[offset] = uv0;
	m_uvs[offset +1] = uv1;
	m_uvs[offset +2] = uv2;
	
	auto &t0 = m_perVertexTangents.at(idx0);
	auto &t1 = m_perVertexTangents.at(idx1);
	auto &t2 = m_perVertexTangents.at(idx2);
	m_uvTangents[offset] = t0;
	m_uvTangents[offset +1] = t1;
	m_uvTangents[offset +2] = t2;

	m_uvTangentSigns[offset] = t0.w;
	m_uvTangentSigns[offset +1] = t1.w;
	m_uvTangentSigns[offset +2] = t2.w;
	/*if((HasAlphas() || HasWrinkles()) && idx0 < m_perVertexAlphas.size() && idx1 < m_perVertexAlphas.size() && idx2 < m_perVertexAlphas.size())
	{
	m_alphas[offset] = m_perVertexAlphas.at(idx0);
	m_alphas[offset +1] = m_perVertexAlphas.at(idx1);
	m_alphas[offset +2] = m_perVertexAlphas.at(idx2);
	}*/
	return true;
}

uint32_t unirender::Mesh::AddSubMeshShader(Shader &shader)
{
	m_subMeshShaders.push_back(std::static_pointer_cast<Shader>(shader.shared_from_this()));
	return m_subMeshShaders.size() -1;
}

void unirender::Mesh::Validate() const
{
	for(auto i=decltype(m_numTris){0u};i<m_numTris;++i)
	{
		auto idx = m_triangles[i];
		if(idx < 0 || idx >= m_verts.size())
			throw std::range_error{"Triangle index " +std::to_string(idx) +" is out of range of number of vertices (" +std::to_string(m_verts.size()) +")"};
	}
}
#pragma optimize("",on)