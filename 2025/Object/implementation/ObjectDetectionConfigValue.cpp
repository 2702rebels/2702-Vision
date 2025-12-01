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
	lastValue(GetConfigValueFromMessage(defaultStruct)),
	setterFunction(setter),
	ntValueSubscriber(ntTopic, lastValue)
{
	UpdateFromConfigMessage(defaultStruct);
}

template<typename T>
void ObjectDetectionConfigValue<T>::UpdateFromConfigMessage(const pbuf::Message& settings) {
	lastValue = GetConfigValueFromMessage(settings);
	UpdateValue(lastValue);
}

template<typename T>
T ObjectDetectionConfigValue<T>::GetConfigValueFromMessage(const pbuf::Message& settingStruct) {
	if constexpr (std::is_same_v<T, int>) {
		return settingStruct.GetReflection()->GetInt32(settings, field);
	}
	else if constexpr (std::is_same_v<T, double>) {
		return settingStruct.GetReflection()->GetDouble(settings, field);
	}
	else if constexpr (std::is_same_v<T, float>) {
		return settingStruct.GetReflection()->GetFloat(settings, field);
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return settingStruct.GetReflection()->GetBool(settings, field);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::UpdateFromConfigMessage");
	}
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