#pragma once
#include <networktables/Topic.h>
#include <networktables/GenericEntry.h>

template<typename T>
class NetworkTopicGeneric {
private:
	nt::GenericSubscriber m_subscriber;
	nt::GenericPublisher m_publisher;
	T m_defaultValue;
public:
	NetworkTopicGeneric(nt::Topic& topic, T defaultValue);
	T GetValue() const;
	void PushValue(const T& value);
	~NetworkTopicGeneric() = default;
};