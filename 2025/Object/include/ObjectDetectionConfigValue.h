#pragma once
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <NetworkTopicPoller.h>
#include <string>

template<typename T, M>
class ObjectDetectionConfigValue {
private:
	const pbuf::FieldDescriptor* field;
	T lastValue;
	std::function<void(T&)> setterFunction;
	NetworkTopicPoller<T> ntValueSubscriber;
public:
	ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, T defaultValue, std::function<void(T&)>& setter, nt::Topic ntTopic);
	void UpdateFromConfigMessage(const M& settings);
	void UpdateValue(const T& newValue);
};