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
ObjectDetectionConfig<T>::ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string ntConfigSubpath)
	: m_configPath(configFilePath), reflection(currentConfigStructure.GetReflection())
{
	const pbuf::Descriptor* messageClass = defaultConfigSettings.GetDescriptor();
	m_configValues.reserve(messageClass->field_count());
	for (int i = 0; i < messageClass->field_count(); i++) {
		const pbuf::FieldDescriptor* field = messageClass->field(i);
		nt::Topic ntTopic = nt::NetworkTableInstance::GetDefault().GetTopic("ObjectDetectionConfig/" + ntConfigSubpath + field->name());
		switch (field->cpp_type()) {
		case pbuf::FieldDescriptor::CPPTYPE_INT32: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<int>>,
				field,
				defaultConfigSettings,
				nullptr,
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<double>>,
				field,
				defaultConfigSettings,
				nullptr,
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<bool>>,
				field,
				defaultConfigSettings,
				nullptr,
				ntTopic
			);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
			m_configValues.emplace_back(
				std::in_place_type<ObjectDetectionConfigValue<float>>,
				field,
				defaultConfigSettings,
				nullptr,
				ntTopic
			);
			break;
		}
		default: {
			spdlog::error("Unsupported field type of field #{} {}", field->index(), field->name());
		}
		}
	}
	RefreshConfigStructure();
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
const T& ObjectDetectionConfig<T>::GetConfigStructure() {
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
		}, configValue);
	}
}

template<typename T>
void ObjectDetectionConfig<T>::UpdateFromNetworkTables() {
	for (auto& configValue : m_configValues) {
		std::visit([&](auto& value) {
			value.UpdateFromNetworkTables();
		}, configValue);
	}
}