#include <NetworkTopicGeneric.h>
#include <type_traits>
#include <string>
#include <cstdint>
#include <vector>
#include <span>

template class NetworkTopicGeneric<int32_t>;
template class NetworkTopicGeneric<double>;
template class NetworkTopicGeneric<bool>;
template class NetworkTopicGeneric<float>;
template class NetworkTopicGeneric<std::string>;
template class NetworkTopicGeneric<int64_t>;
template class NetworkTopicGeneric<std::vector<int32_t>>;

template <typename T>
static std::string type_name() {
	if constexpr (std::is_same_v<T, int32_t>) {
		return "int";
	}
	else if constexpr (std::is_same_v<T, double>) {
		return "double";
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return "boolean";
	}
	else if constexpr (std::is_same_v<T, float>) {
		return "double";
	}
	else if constexpr (std::is_same_v<T, std::string>) {
		return "string";
	}
	else if constexpr (std::is_same_v<T, int64_t>) {
		return "int";
	}
	else if constexpr (std::is_same_v<T, std::vector<int32_t>>) {
		return "int[]";
	}
	else if constexpr (std::is_same_v<T, std::vector<double>>) {
		return "double[]";
	}
	else if constexpr (std::is_same_v<T, std::vector<bool>>) {
		return "boolean[]";
	}
	else if constexpr (std::is_same_v<T, std::vector<float>>) {
		return "float[]";
	}
	else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
		return "string[]";
	}
	else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
		return "int[]";
	}
	else {
		static_assert(false, "Type not supported by NetworkTopicGeneric::type_name()");
	}
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
	if constexpr (std::is_same_v<T, int32_t>) {
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
	else if constexpr (std::is_same_v<T, int64_t>) {
		return m_subscriber.GetInteger(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, std::vector<int32_t>>) {
		std::vector<long int> longDefaultValue(m_defaultValue.begin(), m_defaultValue.end());
		std::vector<long int> longArr = m_subscriber.GetIntegerArray(longDefaultValue);
		return std::vector<int32_t>(longArr.begin(), longArr.end());
	}
	else if constexpr (std::is_same_v<T, std::vector<double>>) {
		return m_subscriber.GetDoubleArray(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, std::vector<float>>) {
		return m_subscriber.GetFloatArray(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, std::vector<bool>>) {
		std::vector<int> boolArrayAsInt;
		boolArrayAsInt.resize(m_defaultValue.size());
		for (int i = 0; i < m_defaultValue.size(); i++) {
			boolArrayAsInt[i] = m_defaultValue[i];
		}
		std::vector<int> retBoolArrInt = m_subscriber.GetBooleanArray(std::span<const int>(boolArrayAsInt));
		std::vector<bool> retBoolArr;
		retBoolArr.resize(retBoolArrInt.size());
		for (int i = 0; i < retBoolArrInt.size(); i++) {
			retBoolArr[i] = retBoolArrInt[i] != 0;
		}
		return retBoolArr;
	}
	else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
		return m_subscriber.GetStringArray(m_defaultValue);
	}
	else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
		return m_subscriber.GetIntegerArray(m_defaultValue);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::UpdateFromConfigMessage");
	}
}

template<typename T>
void NetworkTopicGeneric<T>::PushValue(const T& value) {
	if constexpr (std::is_same_v<T, int32_t>) {
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
	else if constexpr (std::is_same_v<T, int64_t>) {
		m_publisher.SetInteger(value);
	}
	else if constexpr (std::is_same_v<T, std::vector<int32_t>>) {
		std::vector<long int> longVec(value.begin(), value.end());
		m_publisher.SetIntegerArray(longVec);
	}
	else if constexpr (std::is_same_v<T, std::vector<double>>) {
		m_publisher.SetDoubleArray(value);
	}
	else if constexpr (std::is_same_v<T, std::vector<float>>) {
		m_publisher.SetFloatArray(value);
	}
	else if constexpr (std::is_same_v<T, std::vector<bool>>) {
		std::vector<int> boolAsInt;
		boolAsInt.resize(value.size());
		for (int i = 0; i < value.size(); i++) {
			boolAsInt[i] = value[i] ? 1 : 0;
		}
		m_publisher.SetBooleanArray(std::span<const int>(boolAsInt));
	}
	else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
		m_publisher.SetStringArray(value);
	}
	else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
		m_publisher.SetIntegerArray(value);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::PushValue");
	}
}