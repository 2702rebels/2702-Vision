#include <networktables/Topic.h>

template<typename T>
class NetworkTopicPoller {
private:
	nt::GenericSubscriber m_subscriber;
	T m_defaultValue;
public:
	virtual NetworkTopicPoller(const nt::Topic& topic) = 0;
	virtual T GetValue() const = 0;
	virtual ~NetworkTopicPoller() = default;
};