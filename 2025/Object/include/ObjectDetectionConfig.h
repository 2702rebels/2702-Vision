#pragma once
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <ObjectDetectionConfigValue.h>
#include <type_traits>
#include <vector>
#include <string>
#include <variant>

namespace pbuf = google::protobuf;

using ObjectDetectionConfigValueGen = std::variant<
	ObjectDetectionConfigValue<int>,
	ObjectDetectionConfigValue<double>,
	ObjectDetectionConfigValue<float>,
	ObjectDetectionConfigValue<bool>

template<typename T>
class ObjectDetectionConfig {
	static_assert(std::is_base_of<pbuf::Message, T>::value, "ObjectDetectionConfig can only be instantiated with protobuf message types");
private:
	std::vector<ObjectDetectionConfigValue> m_configValues;
	std::string m_configPath;
public:
	ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string ntConfigSubpath);
	void LoadConfigFromFile();
	void WriteConfigToFile();
	T GetConfigStructure();
};