/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#include "util_raytracing/scene.hpp"
#include <util_image_buffer.hpp>
#include <OpenImageDenoise/oidn.hpp>
#include <util/util_half.h>
#include <iostream>

#pragma optimize("",off)
bool raytracing::Scene::Denoise(
	const DenoiseInfo &denoise,
	float *inOutData,float *optAlbedoData,float *optInNormalData,
	const std::function<bool(float)> &fProgressCallback
)
{
	auto device = oidn::newDevice();

	const char *errMsg;
	if(device.getError(errMsg) != oidn::Error::None)
		return false;
	device.setErrorFunction([](void *userPtr,oidn::Error code,const char *message) {
		std::cout<<"Error: "<<message<<std::endl;
		});
	device.set("verbose",true);
	device.commit();

	oidn::FilterRef filter = device.newFilter(denoise.lightmap ? "RTLightmap" : "RT");

	//std::vector<float> albedo {};
	//albedo.resize(denoise.width *denoise.height *3,1.f);

	//std::vector<float> normal {};
	//normal.resize(denoise.width *denoise.height *3,0.f);
	//for(auto i=0;i<denoise.width *denoise.height;i+=3)
	//	normal.at(i +1) = 1.f;

	filter.setImage("color",inOutData,oidn::Format::Float3,denoise.width,denoise.height);
	if(denoise.lightmap == false)
	{
		if(optAlbedoData)
			filter.setImage("albedo",optAlbedoData,oidn::Format::Float3,denoise.width,denoise.height);
		if(optInNormalData)
			filter.setImage("normal",optInNormalData,oidn::Format::Float3,denoise.width,denoise.height);

		filter.set("hdr",denoise.hdr);
	}
	filter.setImage("output",inOutData,oidn::Format::Float3,denoise.width,denoise.height);

	std::unique_ptr<std::function<bool(float)>> ptrProgressCallback = nullptr;
	if(fProgressCallback)
		ptrProgressCallback = std::make_unique<std::function<bool(float)>>(fProgressCallback);
	filter.setProgressMonitorFunction([](void *userPtr,double n) -> bool {
		auto *ptrProgressCallback = static_cast<std::function<bool(float)>*>(userPtr);
		if(ptrProgressCallback)
			return (*ptrProgressCallback)(n);
		return true;
		},ptrProgressCallback.get());

	filter.commit();

	filter.execute();
	return true;
}

bool raytracing::Scene::Denoise(
	const DenoiseInfo &denoiseInfo,uimg::ImageBuffer &imgBuffer,
	uimg::ImageBuffer *optImgBufferAlbedo,uimg::ImageBuffer *optImgBufferNormal,
	const std::function<bool(float)> &fProgressCallback
) const
{
	if(imgBuffer.GetFormat() == uimg::ImageBuffer::Format::RGB_FLOAT)
		return Denoise(denoiseInfo,imgBuffer,optImgBufferAlbedo,optImgBufferNormal,fProgressCallback); // Image is already in the right format, we can just denoise and be done with it

																									   // Image is in the wrong format, we'll need a temporary copy
	auto pImgDenoise = imgBuffer.Copy(uimg::ImageBuffer::Format::RGB_FLOAT);
	auto pImgAlbedo = optImgBufferAlbedo ? optImgBufferAlbedo->Copy(uimg::ImageBuffer::Format::RGB_FLOAT) : nullptr;
	auto pImgNormals = optImgBufferNormal ? optImgBufferNormal->Copy(uimg::ImageBuffer::Format::RGB_FLOAT) : nullptr;
	if(Denoise(
		denoiseInfo,static_cast<float*>(pImgDenoise->GetData()),
		pImgAlbedo ? static_cast<float*>(pImgAlbedo->GetData()) : nullptr,pImgNormals ? static_cast<float*>(pImgNormals->GetData()) : nullptr,
		fProgressCallback
	) == false)
		return false;

	// Copy denoised data back to result buffer
	auto itSrc = pImgDenoise->begin();
	auto itDst = imgBuffer.begin();
	auto numChannels = umath::to_integral(uimg::ImageBuffer::Channel::Count) -1; // -1, because we don't want to overwrite the old alpha channel values
	for(;itSrc != pImgDenoise->end();++itSrc,++itDst)
	{
		auto &pxSrc = *itSrc;
		auto &pxDst = *itDst;
		for(auto i=decltype(numChannels){0u};i<numChannels;++i)
			pxDst.CopyValue(static_cast<uimg::ImageBuffer::Channel>(i),pxSrc);
	}
	return true;
}
#pragma optimize("",on)