#pragma once
#include <networktables/Topic.h>
#include <networktables/GenericEntry.h>

template<typename T>
class NetworkTopicPoller {
private:
	nt::GenericSubscriber m_subscriber;
	T m_defaultValue;
public:
	NetworkTopicPoller(nt::Topic& topic, T defaultValue);
	T GetValue() const;
	~NetworkTopicPoller() = default;
};