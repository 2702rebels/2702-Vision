#pragma once
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <NetworkTopicPoller.h>
#include <string>
#include <memory>

namespace pbuf = google::protobuf;

template<typename T>
class ObjectDetectionConfigValue {
private:
	const pbuf::FieldDescriptor* field;
	T lastValue;
	std::function<void(T&)>* setterFunction;
	std::unique_ptr<NetworkTopicPoller<T>> ntValueSubscriber;
	T GetConfigValueFromMessage(const pbuf::Message& topic);
public:
	// Explicit move constructors
	ObjectDetectionConfigValue(ObjectDetectionConfigValue&&) noexcept = default;
	ObjectDetectionConfigValue& operator=(ObjectDetectionConfigValue&&) noexcept = default;
	// No copying allowed
	ObjectDetectionConfigValue(const ObjectDetectionConfigValue&) = delete;
	ObjectDetectionConfigValue& operator=(const ObjectDetectionConfigValue&) = delete;
	ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, pbuf::Message& defaultStruct, std::function<void(T&)>* setter, nt::Topic ntTopic);
	void UpdateFromConfigMessage(const pbuf::Message& settings);
	void UpdateValue(const T& newValue);
	const pbuf::FieldDescriptor* GetFieldDescriptor() const;
	T GetValue() const;
};