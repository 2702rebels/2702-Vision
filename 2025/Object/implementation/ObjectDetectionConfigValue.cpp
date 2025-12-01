#include <ObjectDetectionConfigValue.h>
#include <string>
#include <type_traits>
#include <ObjectDetectionSettings.pb.h>


template class ObjectDetectionConfigValue<int>;
template class ObjectDetectionConfigValue<double>;
template class ObjectDetectionConfigValue<float>;
template class ObjectDetectionConfigValue<bool>;

template<typename T>
ObjectDetectionConfigValue<T>::ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, pbuf::Message& defaultStruct, std::function<void(T&)>* setter, nt::Topic ntTopic)
	: field(field),
	setterFunction(setter),
	ntValueSubscriber(ntTopic, defaultValue) 
{
	UpdateFromConfigMessage(defaultStruct);
}

template<typename T>
void ObjectDetectionConfigValue<T>::UpdateFromConfigMessage(const pbuf::Message& settings) {
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
	UpdateValue(lastValue);
}

template<typename T>
void ObjectDetectionConfigValue<T>::UpdateValue(const T& newValue) {
	lastValue = newValue;
	if (setterFunction == nullptr) return;
	(*setterFunction)(lastValue);
}

template<typename T>
T ObjectDetectionConfigValue<T>::GetValue() const {
	return lastValue;
}