#pragma once
#include <ObjectDetectionSettings.pb.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>

using namespace pbuf = google::protobuf;

template<typename T>
class ObjectDetectionConfigValue {
private:
	const pbuf::FieldDescriptor* field;
	T lastValue;
	std::function<void, (T&)> setterFunction;
	nt::ProtobufSubscriber<T> ntValueSubscriber;
public:
	ObjectDetectionConfigValue(const pbuf::Descriptor* field, T defaultValue, std::function<void, (T&)>& setter, nt::Topic ntTopic);
	void UpdateFromConfigMessage(const ObjectDetectionSettings& settings);
	void UpdateValue(const T& newValue);
};