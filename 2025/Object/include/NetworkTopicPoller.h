#include <networktables/Topic.h>
#include <networktables/GenericEntry.h>

template<typename T>
class NetworkTopicPoller {
private:
	nt::GenericSubscriber m_subscriber;
	T m_defaultValue;
public:
	NetworkTopicPoller(const nt::Topic& topic);
	virtual T GetValue() const;
	virtual ~NetworkTopicPoller() = default;
};