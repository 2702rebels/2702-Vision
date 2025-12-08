#include <ObjectDetectionConfigValue.h>
#include <string>
#include <type_traits>

template class ObjectDetectionConfigValue<int32_t>;
template class ObjectDetectionConfigValue<double>;
template class ObjectDetectionConfigValue<float>;
template class ObjectDetectionConfigValue<bool>;
template class ObjectDetectionConfigValue<std::string>;
template class ObjectDetectionConfigValue<int64_t>;

template<typename T>
ObjectDetectionConfigValue<T>::ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field, pbuf::Message& defaultStruct, std::function<void(const T&)> setter, nt::Topic ntTopic)
	: field(field),
	lastValue(GetConfigValueFromMessage(defaultStruct)),
	setterFunction(setter),
	ntValueSyncer(std::make_unique<NetworkTopicGeneric<T>>(ntTopic, lastValue))
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
		return settingStruct.GetReflection()->GetInt32(settingStruct, field);
	}
	else if constexpr (std::is_same_v<T, double>) {
		return settingStruct.GetReflection()->GetDouble(settingStruct, field);
	}
	else if constexpr (std::is_same_v<T, float>) {
		return settingStruct.GetReflection()->GetFloat(settingStruct, field);
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return settingStruct.GetReflection()->GetBool(settingStruct, field);
	}
	else if constexpr (std::is_same_v<T, std::string>) {
		return settingStruct.GetReflection()->GetString(settingStruct, field);
	}
	else if constexpr (std::is_same_v<T, int64_t>) {
		return settingStruct.GetReflection()->GetInt64(settingStruct, field);
	}
	else {
		static_assert(false, "Type not supported by ObjectDetectionConfigValue::UpdateFromConfigMessage");
	}
}

template<typename T>
void ObjectDetectionConfigValue<T>::UpdateValue(const T& newValue) {
	lastValue = newValue;
	ntValueSyncer->PushValue(newValue);
	if (setterFunction) {
		setterFunction(lastValue);
	}
}

template<typename T>
T ObjectDetectionConfigValue<T>::GetValue() const {
	return lastValue;
}

template<typename T>
const pbuf::FieldDescriptor* ObjectDetectionConfigValue<T>::GetFieldDescriptor() const {
	return field;
}

template<typename T>
void ObjectDetectionConfigValue<T>::UpdateFromNetworkTables() {
	T ntValue = ntValueSyncer->GetValue();
	if (ntValue != lastValue) {
		UpdateValue(ntValue);
	}
}