#include <NetworkTopicPoller.h>
#include <type_traits>
#include <string>

template class NetworkTopicPoller<int>;
template class NetworkTopicPoller<double>;
template class NetworkTopicPoller<bool>;
template class NetworkTopicPoller<float>;
template class NetworkTopicPoller<std::string>;

template<typename T>
NetworkTopicPoller<T>::NetworkTopicPoller(nt::Topic& topic, T defaultValue) :
	m_defaultValue(defaultValue) 
{
	m_subscriber = topic.GenericSubscribe();
}

template<typename T>
T NetworkTopicPoller<T>::GetValue() const {
	if constexpr (std::is_same_v<T, int>) {
		return m_subscriber.GetInteger(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, double>) {
		return m_subscriber.GetDouble(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, float>) {
		return m_subscriber.GetFloat(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return m_subscriber.GetBoolean(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, std::string>) {
		return m_subscriber.GetString(m_defaultValue);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::UpdateFromConfigMessage");
	}
}