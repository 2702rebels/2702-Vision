#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/descriptor.h>
#include <fstream>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <google/protobuf/util/json_util.h>
#include <networktables/NetworkTableInstance.h>

template class ObjectDetectionConfig<ObjectDetectionSettings>;

template<typename T>
ObjectDetectionConfig<T>::ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string ntConfigSubpath)
	: m_configPath(configFilePath)
{
	const pbuf::Descriptor* messageClass = defaultConfigSettings.GetDescriptor();
	m_configValues = std::vector<ObjectDetectionConfigValueGen>(messageClass->field_count());
	for (int i = 0; i < messageClass->field_count(); i++) {
		const pbuf::FieldDescriptor* field = messageClass->field(i);
		nt::Topic ntTopic = nt::NetworkTableInstance::GetDefault().GetTopic("ObjectDetectionConfig/" + ntConfigSubpath + field->name());
		switch (field->cpp_type()) {
		case pbuf::FieldDescriptor::CPPTYPE_INT32: {
			m_configValues[i] = ObjectDetectionConfigValue<int>(field, defaultConfigSettings, nullptr, ntTopic);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
			m_configValues[i] = ObjectDetectionConfigValue<double>(field, defaultConfigSettings, nullptr, ntTopic);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_STRING: {
			m_configValues[i] = ObjectDetectionConfigValue<bool>(field, defaultConfigSettings, nullptr, ntTopic);
			break;
		}
		case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
			m_configValues[i] = ObjectDetectionConfigValue<float>(field, defaultConfigSettings, nullptr, ntTopic);
			break;
		}
		default: {
			spdlog::error("Unsupported field type of field #{} {}", field->index(), field->name());
		}
		}
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
	T configMessage;
	auto status = pbuf::util::JsonStringToMessage(configContents, &configMessage);
	if (!status.ok()) {
		spdlog::error("Failed to parse config file at path: {}\nParse error: {}", m_configPath, status.message());
		throw;
	}
	for (auto& configValue : m_configValues) {
		// Runs value.UpdateFromConfigMessage if value is any one of the variant types
		std::visit([&](auto& value) {
			value.UpdateFromConfigMessage(configMessage);
		}, configValue);
	}
}

template<typename T>
void ObjectDetectionConfig<T>::WriteConfigToFile() {
	T configMessage;
	const pbuf::Descriptor* messageClass = configMessage.GetDescriptor();
	const pbuf::Reflection* messageReflection = configMessage.GetReflection();
	for (int i = 0; i < messageClass->field_count(); i++) {

	}
}