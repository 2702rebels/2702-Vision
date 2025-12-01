#pragma once
#include <ObjectDetectionSettings.pb.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <networktables/ProtobufTopic.h>
#include <NetworkTopicPoller.h>

namespace pbuf = google::protobuf;

template<typename T>
class ObjectDetectionConfigValue {
private:
	const pbuf::FieldDescriptor* field;
	T lastValue;
	std::function<void(T&)> setterFunction;
	NetworkTopicPoller<T> ntValueSubscriber;
public:
	ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, T defaultValue, std::function<void(T&)>& setter, nt::Topic ntTopic);
	void UpdateFromConfigMessage(const ObjectDetectionSettings& settings);
	void UpdateValue(const T& newValue);
};