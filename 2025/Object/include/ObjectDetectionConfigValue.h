#pragma once
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <NetworkTopicPoller.h>
#include <string>

namespace pbuf = google::protobuf;

template<typename T>
class ObjectDetectionConfigValue {
private:
	const pbuf::FieldDescriptor* field;
	T lastValue;
	std::function<void(T&)>* setterFunction;
	NetworkTopicPoller<T> ntValueSubscriber;
public:
	ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, pbuf::Message& defaultStruct, std::function<void(T&)>* setter, nt::Topic ntTopic);
	void UpdateFromConfigMessage(const pbuf::Message& settings);
	void UpdateValue(const T& newValue);
	T GetValue() const;
};