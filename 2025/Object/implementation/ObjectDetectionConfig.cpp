#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/descriptor.h>
#include <fstream>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <google/protobuf/util/json_util.h>
#include <networktables/NetworkTableInstance.h>
#include <type_traits>
#include <google/protobuf/message.h>

template class ObjectDetectionConfig<ObjectDetectionSettings>;

template<typename T>
ObjectDetectionConfig<T>::ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string configName)
	: m_configPath(configFilePath), reflection(currentConfigStructure.GetReflection())
{
	const pbuf::Descriptor* messageClass = defaultConfigSettings.GetDescriptor();
	m_configValues.reserve(messageClass->field_count());
	for (int i = 0; i < messageClass->field_count(); i++) {
		const pbuf::FieldDescriptor* field = messageClass->field(i);
		fieldNamesToIndex[field->name()] = i;
		nt::Topic ntTopic = nt::NetworkTableInstance::GetDefault().GetTopic("ObjectDetectionConfig/" + configName  + "/" + field->name());
		switch (field->cpp_type()) {
		case pbuf::FieldDescriptor::CPPTYPE_INT32: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<int>>,
				field,
				defaultConfigSettings,
				std::function<void(const int&)>{},
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<double>>,
				field,
				defaultConfigSettings,
				std::function<void(const double&)>{},
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<bool>>,
				field,
				defaultConfigSettings,
				std::function<void(const bool&)>{},
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<float>>,
				field,
				defaultConfigSettings,
				std::function<void(const float&)>{},
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_STRING: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<std::string>>,
				field,
				defaultConfigSettings,
				std::function<void(const std::string&)>{},
				ntTopic
			);
			break;
		}
		default: {
			spdlog::error("Unsupported field type of field #{} {}", field->index(), field->name());
			throw;
		}
		}
	}
	std::ifstream checkFile(configFilePath);
	if (checkFile.is_open()) {
		checkFile.close();
		LoadConfigFromFile();
	}
	else {
		RefreshConfigStructure();
	}
}

template<typename T>
void ObjectDetectionConfig<T>::LoadConfigFromStructure(const T& newConfigSettings) {
	currentConfigStructure.CopyFrom(newConfigSettings);
	for (auto& configValue : m_configValues) {
		std::visit([&](auto& value) {
			value.UpdateFromConfigStructure(currentConfigStructure);
		}, configValue);
	}
}

template<typename T>
void ObjectDetectionConfig<T>::LoadConfigFromFile() {
	std::ifstream configFile(m_configPath);
	if (!configFile.is_open()) {
		spdlog::error("Failed to open config file at path: {}", m_configPath);
		throw;
	}
	std::ostringstream buffer;
	buffer << configFile.rdbuf();
	std::string configContents = buffer.str();
	auto status = pbuf::util::JsonStringToMessage(configContents, &currentConfigStructure);
	if (!status.ok()) {
		spdlog::error("Failed to parse config file at path: {}\nParse error: {}", m_configPath, status.message());
		throw;
	}
	for (auto& configValue : m_configValues) {
		// Runs value.UpdateFromConfigMessage if value is any one of the variant types
		std::visit([&](auto& value) {
			value.UpdateFromConfigMessage(currentConfigStructure);
		}, configValue);
	}
}

template<typename T>
void ObjectDetectionConfig<T>::WriteConfigToFile() {
	std::ofstream configFile(m_configPath, std::ios::out | std::ios::trunc);
	if (!configFile.is_open()) {
		spdlog::error("Failed to open config file for writing at path: {}", m_configPath);
		throw;
	}
	std::string configJson;
	auto status = pbuf::util::MessageToJsonString(currentConfigStructure, &configJson);
	if (!status.ok()) {
		spdlog::error("Failed to serialize config to JSON: {}", status.message());
		throw;
	}
	configFile << configJson;
}

template<typename T>
const T& ObjectDetectionConfig<T>::Get() {
	return currentConfigStructure;
}

template<typename T>
void ObjectDetectionConfig<T>::RefreshConfigStructure() {
	for (auto& configValue : m_configValues) {
		std::visit([&](auto& value) {
			const pbuf::FieldDescriptor* field = value.GetFieldDescriptor();
			using ValueType = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<ValueType, ObjectDetectionConfigValue<int>>) {
				reflection->SetInt32(&currentConfigStructure, field, value.GetValue());
			}
			else if constexpr (std::is_same_v<ValueType, ObjectDetectionConfigValue<double>>) {
				reflection->SetDouble(&currentConfigStructure, field, value.GetValue());
			}
			else if constexpr (std::is_same_v<ValueType, ObjectDetectionConfigValue<float>>) {
				reflection->SetFloat(&currentConfigStructure, field, value.GetValue());
			}
			else if constexpr (std::is_same_v<ValueType, ObjectDetectionConfigValue<bool>>) {
				reflection->SetBool(&currentConfigStructure, field, value.GetValue());
			}
			else if constexpr (std::is_same_v<ValueType, ObjectDetectionConfigValue<std::string>>) {
				reflection->SetString(&currentConfigStructure, field, value.GetValue());
			}
		}, configValue);
	}
	WriteConfigToFile();
}

template<typename T>
void ObjectDetectionConfig<T>::UpdateFromNetworkTables() {
	for (auto& configValue : m_configValues) {
		std::visit([&](auto& value) {
			value.UpdateFromNetworkTables();
		}, configValue);
	}
	RefreshConfigStructure();
}

template<typename T>
ObjectDetectionConfigValueGen& ObjectDetectionConfig<T>::GetConfigValueStruct(std::string fieldName) {
	auto it = fieldNamesToIndex.find(fieldName);
	if (it == fieldNamesToIndex.end()) {
		spdlog::error("Failed to find field of name {} in GetConfigValueStruct", fieldName);
		throw;
	}
	return m_configValues[fieldNamesToIndex[fieldName]];
}

template<typename T>
ObjectDetectionConfigValueGen& ObjectDetectionConfig<T>::GetConfigValueStruct(int fieldIndex) {
	if (fieldIndex < 0 || fieldIndex >= m_configValues.size()) {
		spdlog::error("Invalid fieldIndex provided to GetConfigValueStruct: {}", fieldIndex);
		throw;
	}
	return m_configValues[fieldIndex];
}

template<typename T>
template<typename O>
ObjectDetectionConfigValue<O>& ObjectDetectionConfig<T>::GetConfigValueStruct(std::string fieldName) {
	return std::get<O>(GetConfigValueStruct(fieldName));
}

template<typename T>
template<typename O>
ObjectDetectionConfigValue<O>& ObjectDetectionConfig<T>::GetConfigValueStruct(int fieldIndex) {
	return std::get<O>(GetConfigValueStruct(fieldIndex));
}

template<typename T>
template<typename O>
void ObjectDetectionConfig<T>::AttachSetterFunction(std::string fieldName, std::function<void(const O&)> setterFunction) {
	auto it = fieldNamesToIndex.find(fieldName);
	if (it == fieldNamesToIndex.end()) {
		spdlog::error("Failed to find field with name {} in AttachSetterFunction", fieldName);
		throw;
	}
	std::get<ObjectDetectionConfigValue<O>>(m_configValues[it->second]).setterFunction = std::move(setterFunction);
}

template<typename T>
template<typename O>
void ObjectDetectionConfig<T>::AttachSetterFunction(int fieldIndex, std::function<void(const O&)> setterFunction) {
	if (fieldIndex < 0 || fieldIndex >= m_configValues.size()) {
		spdlog::error("Invalid field index in AttachSetterFunction: {}", fieldIndex);
		throw;
	}
	std::get<ObjectDetectionConfigValue<O>>(m_configValues[fieldIndex]).setterFunction = std::move(setterFunction);
}

template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<int>(int, std::function<void(const int&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<double>(int, std::function<void(const double&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<float>(int, std::function<void(const float&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<bool>(int, std::function<void(const bool&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<std::string>(int, std::function<void(const std::string&)>);

template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<int>(std::string, std::function<void(const int&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<double>(std::string, std::function<void(const double&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<float>(std::string, std::function<void(const float&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<bool>(std::string, std::function<void(const bool&)>);
template void ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<std::string>(std::string, std::function<void(const std::string&)>);