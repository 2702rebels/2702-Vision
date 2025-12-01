#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/descriptor.h>
#include <fstream>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <google/protobuf/util/json_util.h>

template class ObjectDetectionConfig<ObjectDetectionSettings>;

template<typename T>
ObjectDetectionConfig<T>::ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string ntConfigSubpath)
	: m_configPath(configFilePath) 
{
	const pbuf::Descriptor* messageClass = defaultConfigSettings.GetDescriptor();
	m_configValues = std::vector<ObjectDetectionConfigValue<T>>(messageClass->field_count());
	for (int i = 0; i < messageClass->field_count(); i++) {
		const pbuf::FieldDescriptor* field = messageClass->field(i);
		nt::Topic ntTopic = nt::NetworkTableInstance::GetDefault().GetTopic("ObjectDetectionConfig/" + ntConfigSubpath + field->name());
		m_configValues[i] = ObjectDetectionConfigValue(field, defaultConfigSettings, nullptr, topic);
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
	const pbuf::Descriptor* messageClass = configMessage.GetDescriptor();
	for (int i = 0; i < messageClass->field_count(); i++) {
		m_configValues[i].UpdateFromConfigMessage(configMessage);
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