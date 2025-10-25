// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>

#include <cinttypes>
#include <string>

#include <optional>

module pragma.scenekit;

import :data_value;

void pragma::scenekit::DataValue::Serialize(util::DataStream &dsOut) const
{
	dsOut->Write(type);
	dsOut->Write<bool>(value != nullptr);
	if(value == nullptr)
		return;
	switch(type) {
	case SocketType::Bool:
		dsOut->Write<STBool>(*static_cast<STBool *>(value.get()));
		break;
	case SocketType::Float:
		dsOut->Write<STFloat>(*static_cast<STFloat *>(value.get()));
		break;
	case SocketType::Int:
		dsOut->Write<STInt>(*static_cast<STInt *>(value.get()));
		break;
	case SocketType::UInt:
		dsOut->Write<STUInt>(*static_cast<STUInt *>(value.get()));
		break;
	case SocketType::Color:
		dsOut->Write<STColor>(*static_cast<STColor *>(value.get()));
		break;
	case SocketType::Vector:
		dsOut->Write<STVector>(*static_cast<STVector *>(value.get()));
		break;
	case SocketType::Point:
		dsOut->Write<STPoint>(*static_cast<STPoint *>(value.get()));
		break;
	case SocketType::Normal:
		dsOut->Write<STNormal>(*static_cast<STNormal *>(value.get()));
		break;
	case SocketType::Point2:
		dsOut->Write<STPoint2>(*static_cast<STPoint2 *>(value.get()));
		break;
	case SocketType::Enum:
		dsOut->Write<STEnum>(*static_cast<STEnum *>(value.get()));
		break;
	case SocketType::Transform:
		dsOut->Write<STTransform>(*static_cast<STTransform *>(value.get()));
		break;
	case SocketType::String:
		dsOut->WriteString(*static_cast<STString *>(value.get()));
		break;
	case SocketType::FloatArray:
		{
			auto &v = *static_cast<STFloatArray *>(value.get());
			dsOut->Write<uint32_t>(v.size());
			dsOut->Write(reinterpret_cast<uint8_t *>(v.data()), v.size() * sizeof(v.front()));
			break;
		}
	case SocketType::ColorArray:
		{
			auto &v = *static_cast<STColorArray *>(value.get());
			dsOut->Write<uint32_t>(v.size());
			dsOut->Write(reinterpret_cast<uint8_t *>(v.data()), v.size() * sizeof(v.front()));
			break;
		}
	case SocketType::Closure:
	case SocketType::Node:
		break;
	}
	static_assert(umath::to_integral(SocketType::Count) == 16);
}
pragma::scenekit::DataValue pragma::scenekit::DataValue::Deserialize(util::DataStream &dsIn)
{
	auto type = dsIn->Read<SocketType>();
	auto hasValue = dsIn->Read<bool>();
	if(hasValue == false)
		return DataValue {type, nullptr};
	switch(type) {
	case SocketType::Bool:
		return DataValue::Create<STBool, SocketType::Bool>(dsIn->Read<STBool>());
	case SocketType::Float:
		return DataValue::Create<STFloat, SocketType::Float>(dsIn->Read<STFloat>());
	case SocketType::Int:
		return DataValue::Create<STInt, SocketType::Int>(dsIn->Read<STInt>());
	case SocketType::UInt:
		return DataValue::Create<STUInt, SocketType::UInt>(dsIn->Read<STUInt>());
	case SocketType::Color:
		return DataValue::Create<STColor, SocketType::Color>(dsIn->Read<STColor>());
	case SocketType::Vector:
		return DataValue::Create<STVector, SocketType::Vector>(dsIn->Read<STVector>());
	case SocketType::Point:
		return DataValue::Create<STPoint, SocketType::Point>(dsIn->Read<STPoint>());
	case SocketType::Normal:
		return DataValue::Create<STNormal, SocketType::Normal>(dsIn->Read<STNormal>());
	case SocketType::Point2:
		return DataValue::Create<STPoint2, SocketType::Point2>(dsIn->Read<STPoint2>());
	case SocketType::Enum:
		return DataValue::Create<STEnum, SocketType::Enum>(dsIn->Read<STEnum>());
	case SocketType::Transform:
		return DataValue::Create<STTransform, SocketType::Transform>(dsIn->Read<STTransform>());
	case SocketType::String:
		return DataValue::Create<STString, SocketType::String>(dsIn->ReadString());
	case SocketType::FloatArray:
		{
			auto n = dsIn->Read<uint32_t>();
			STFloatArray values {};
			values.resize(n);
			dsIn->Read(values.data(), values.size() * sizeof(values.front()));
			return DataValue::Create<STFloatArray, SocketType::Transform>(std::move(values));
		}
	case SocketType::ColorArray:
		{
			auto n = dsIn->Read<uint32_t>();
			STColorArray values {};
			values.resize(n);
			dsIn->Read(values.data(), values.size() * sizeof(values.front()));
			return DataValue::Create<STColorArray, SocketType::Transform>(std::move(values));
		}
	case SocketType::Closure:
	case SocketType::Node:
		return DataValue {type, nullptr};
	}
	//unreachable, unless it went seriously wrong.
	return DataValue {type, nullptr};
}
void pragma::scenekit::DataValue::Serialize(udm::LinkedPropertyWrapper &data) const
{
	data["type"] << type;
	if(value == nullptr) {
		data["value"] << udm::Nil {};
		return;
	}
	switch(type) {
	case SocketType::Bool:
		data["value"] << *static_cast<STBool *>(value.get());
		break;
	case SocketType::Float:
		data["value"] << *static_cast<STFloat *>(value.get());
		break;
	case SocketType::Int:
		data["value"] << *static_cast<STInt *>(value.get());
		break;
	case SocketType::UInt:
		data["value"] << *static_cast<STUInt *>(value.get());
		break;
	case SocketType::Color:
		data["value"] << *static_cast<STColor *>(value.get());
		break;
	case SocketType::Vector:
		data["value"] << *static_cast<STVector *>(value.get());
		break;
	case SocketType::Point:
		data["value"] << *static_cast<STPoint *>(value.get());
		break;
	case SocketType::Normal:
		data["value"] << *static_cast<STNormal *>(value.get());
		break;
	case SocketType::Point2:
		data["value"] << *static_cast<STPoint2 *>(value.get());
		break;
	case SocketType::Enum:
		data["value"] << *static_cast<STEnum *>(value.get());
		break;
	case SocketType::Transform:
		data["value"] << glm::transpose(*static_cast<STTransform *>(value.get()));
		break;
	case SocketType::String:
		data["value"] << *static_cast<STString *>(value.get());
		break;
	case SocketType::FloatArray:
		{
			auto &v = *static_cast<STFloatArray *>(value.get());
			data.AddArray<float>("value", v);
			break;
		}
	case SocketType::ColorArray:
		{
			auto &v = *static_cast<STColorArray *>(value.get());
			data.AddArray<Vector3>("value", v);
			break;
		}
	case SocketType::Closure:
	case SocketType::Node:
		break;
	}
	static_assert(umath::to_integral(SocketType::Count) == 16);
}
pragma::scenekit::DataValue pragma::scenekit::DataValue::Deserialize(udm::LinkedPropertyWrapper &data)
{
	auto type = SocketType::Invalid;
	data["type"] >> type;
	if(!data["value"])
		return DataValue {type, nullptr};
	auto loadVal = [&data, type]<typename T, SocketType SOCKET_TYPE>() {
		if constexpr(std::is_same_v<T, glm::mat4x3>) {
			udm::Mat3x4 m;
			if(data["value"] >> m)
				return DataValue::Create<T, SOCKET_TYPE>(glm::transpose(m));
		}
		else {
			T val;
			if(data["value"] >> val)
				return DataValue::Create<T, SOCKET_TYPE>(val);
		}
		return DataValue {type, nullptr};
	};
	switch(type) {
	case SocketType::Bool:
		return loadVal.template operator()<STBool, SocketType::Bool>();
	case SocketType::Float:
		return loadVal.template operator()<STFloat, SocketType::Float>();
	case SocketType::Int:
		return loadVal.template operator()<STInt, SocketType::Int>();
	case SocketType::UInt:
		return loadVal.template operator()<STUInt, SocketType::UInt>();
	case SocketType::Color:
		return loadVal.template operator()<STColor, SocketType::Color>();
	case SocketType::Vector:
		return loadVal.template operator()<STVector, SocketType::Vector>();
	case SocketType::Point:
		return loadVal.template operator()<STPoint, SocketType::Point>();
	case SocketType::Normal:
		return loadVal.template operator()<STNormal, SocketType::Normal>();
	case SocketType::Point2:
		return loadVal.template operator()<STPoint2, SocketType::Point2>();
	case SocketType::Enum:
		return loadVal.template operator()<STEnum, SocketType::Enum>();
	case SocketType::Transform:
		return loadVal.template operator()<STTransform, SocketType::Transform>();
	case SocketType::String:
		return loadVal.template operator()<STString, SocketType::String>();
	case SocketType::FloatArray:
		{
			auto udmVal = data["value"];
			STFloatArray values {};
			udmVal >> values;
			return DataValue::Create<STFloatArray, SocketType::Transform>(std::move(values));
		}
	case SocketType::ColorArray:
		{
			auto udmVal = data["value"];
			STColorArray values {};
			udmVal >> values;
			return DataValue::Create<STColorArray, SocketType::Transform>(std::move(values));
		}
	case SocketType::Closure:
	case SocketType::Node:
		return DataValue {type, nullptr};
	}
	//unreachable, unless it went seriously wrong.
	return DataValue {type, nullptr};
}

std::string pragma::scenekit::to_string(SocketType type)
{
	switch(type) {
	case SocketType::Bool:
		return "Bool";
	case SocketType::Float:
		return "Float";
	case SocketType::Int:
		return "Int";
	case SocketType::UInt:
		return "UInt";
	case SocketType::Color:
		return "Color";
	case SocketType::Vector:
		return "Vector";
	case SocketType::Point:
		return "Point";
	case SocketType::Normal:
		return "Normal";
	case SocketType::Point2:
		return "Point2";
	case SocketType::Closure:
		return "Closure";
	case SocketType::String:
		return "String";
	case SocketType::Enum:
		return "Enum";
	case SocketType::Transform:
		return "Transform";
	case SocketType::Node:
		return "Node";
	case SocketType::FloatArray:
		return "FloatArray";
	case SocketType::ColorArray:
		return "ColorArray";
	}
	return "Invalid";
}
std::optional<pragma::scenekit::DataValue> pragma::scenekit::convert(const void *value, SocketType srcType, SocketType dstType)
{
	if(is_convertible_to(srcType, dstType) == false)
		return {};
	std::optional<DataValue> dstDataValue {};
	switch(dstType) {
	case SocketType::Bool:
		{
			auto dstValue = convert<STBool>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STBool>(*dstValue)};
			break;
		}
	case SocketType::Float:
		{
			auto dstValue = convert<STFloat>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STFloat>(*dstValue)};
			break;
		}
	case SocketType::Int:
		{
			auto dstValue = convert<STInt>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STInt>(*dstValue)};
			break;
		}
	case SocketType::Enum:
		{
			auto dstValue = convert<STEnum>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STEnum>(*dstValue)};
			break;
		}
	case SocketType::UInt:
		{
			auto dstValue = convert<STUInt>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STUInt>(*dstValue)};
			break;
		}
	case SocketType::Color:
		{
			auto dstValue = convert<STColor>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STColor>(*dstValue)};
			break;
		}
	case SocketType::Vector:
		{
			auto dstValue = convert<STVector>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STVector>(*dstValue)};
			break;
		}
	case SocketType::Point:
		{
			auto dstValue = convert<STPoint>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STPoint>(*dstValue)};
			break;
		}
	case SocketType::Normal:
		{
			auto dstValue = convert<STNormal>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STNormal>(*dstValue)};
			break;
		}
	case SocketType::Point2:
		{
			auto dstValue = convert<STPoint2>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STPoint2>(*dstValue)};
			break;
		}
	case SocketType::String:
		{
			auto dstValue = convert<STString>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STString>(*dstValue)};
			break;
		}
	case SocketType::Transform:
		{
			auto dstValue = convert<STTransform>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STTransform>(*dstValue)};
			break;
		}
	case SocketType::FloatArray:
		{
			auto dstValue = convert<STFloatArray>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STFloatArray>(*dstValue)};
			break;
		}
	case SocketType::ColorArray:
		{
			auto dstValue = convert<STColorArray>(value, srcType);
			if(dstValue.has_value())
				dstDataValue = DataValue {dstType, std::make_shared<STColorArray>(*dstValue)};
			break;
		}
	}
	return dstDataValue;
}
