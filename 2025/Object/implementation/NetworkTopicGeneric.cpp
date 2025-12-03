#include <NetworkTopicGeneric.h>
#include <type_traits>
#include <string>
#include <typeinfo>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

template class NetworkTopicGeneric<int>;
template class NetworkTopicGeneric<double>;
template class NetworkTopicGeneric<bool>;
template class NetworkTopicGeneric<float>;
template class NetworkTopicGeneric<std::string>;

template <typename T>
static std::string type_name() {
	const char* name = typeid(T).name();
#ifdef __GNUG__
	int status = 0;
	char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	std::string result = (status == 0) ? demangled : name;
	std::free(demangled);
	return result;
#else
	return name; // MSVC already gives readable names
#endif
}

template<typename T>
NetworkTopicGeneric<T>::NetworkTopicGeneric(nt::Topic& topic, T defaultValue) :
	m_defaultValue(defaultValue) 
{
	m_subscriber = topic.GenericSubscribe();
	m_publisher = topic.GenericPublish(type_name<T>());
}

template<typename T>
T NetworkTopicGeneric<T>::GetValue() const {
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

template<typename T>
void NetworkTopicGeneric<T>::PushValue(const T& value) {
	if constexpr (std::is_same_v<T, int>) {
		m_publisher.SetInteger(value);
	}
	else if constexpr (std::is_same_v<T, double>) {
		m_publisher.SetDouble(value);
	}
	else if constexpr (std::is_same_v<T, float>) {
		m_publisher.SetFloat(value);
	}
	else if constexpr (std::is_same_v<T, bool>) {
		m_publisher.SetBoolean(value);
	}
	else if constexpr (std::is_same_v<T, std::string>) {
		m_publisher.SetString(value);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::PushValue");
	}
}