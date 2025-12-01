#include <ObjectDetectionConfigValue.h>
#include <string>
#include <type_traits>


template class ObjectDetectionConfigValue<int>;
template class ObjectDetectionConfigValue<double>;
template class ObjectDetectionConfigValue<float>;
template class ObjectDetectionConfigValue<bool>;

template<typename T,M>
ObjectDetectionConfigValue<T,M>::ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, T defaultValue, std::function<void(T&)>& setter, nt::Topic ntTopic)
	: field(field),
	lastValue(defaultValue),
	setterFunction(setter),
	ntValueSubscriber(ntTopic, defaultValue) 
{
	UpdateValue(defaultValue);
}

template<typename T,M>
void ObjectDetectionConfigValue<T,M>::UpdateFromConfigMessage(const M& settings) {
	if constexpr (std::is_same_v<T, int>) {
		lastValue = settings.GetReflection()->GetInt32(settings, field);
	}
	else if constexpr (std::is_same_v<T, double>) {
		lastValue = settings.GetReflection()->GetDouble(settings, field);
	}
	else if constexpr (std::is_same_v<T, float>) {
		lastValue = settings.GetReflection()->GetFloat(settings, field);
	}
	else if constexpr (std::is_same_v<T, bool>) {
		lastValue = settings.GetReflection()->GetBool(settings, field);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::UpdateFromConfigMessage");
	}
	setterFunction(lastValue);
}

template<typename T,M>
void ObjectDetectionConfigValue<T,M>::UpdateValue(const T& newValue) {
	lastValue = newValue;
	setterFunction(lastValue);
}