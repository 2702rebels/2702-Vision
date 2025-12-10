#pragma once
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <ObjectDetectionConfigValue.h>
#include <type_traits>
#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

namespace pbuf = google::protobuf;

using ObjectDetectionConfigValueGen = std::variant<
	ObjectDetectionConfigValue<int32_t>,
	ObjectDetectionConfigValue<double>,
	ObjectDetectionConfigValue<float>,
	ObjectDetectionConfigValue<bool>,
	ObjectDetectionConfigValue<std::string>,
	ObjectDetectionConfigValue<int64_t>,
	ObjectDetectionConfigValue<std::vector<int32_t>>,
	ObjectDetectionConfigValue<std::vector<double>>,
	ObjectDetectionConfigValue<std::vector<float>>,
	ObjectDetectionConfigValue<std::vector<bool>>,
	ObjectDetectionConfigValue<std::vector<std::string>>,
	ObjectDetectionConfigValue<std::vector<int64_t>>
>;

template<typename T>
class ObjectDetectionConfig {
	static_assert(std::is_base_of<pbuf::Message, T>::value, "ObjectDetectionConfig can only be instantiated with protobuf message types");
private:
	const pbuf::Reflection* reflection;
	std::string m_configPath;
	std::vector<ObjectDetectionConfigValueGen> m_configValues;
	std::unordered_map<std::string, int> fieldNamesToIndex;
	T currentConfigStructure;
public:
	ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings, std::string configName);
	void LoadConfigFromFile();
	void WriteConfigToFile();
	void UpdateFromNetworkTables();
	void RefreshConfigStructure();
	void LoadConfigFromStructure(const T& newConfigSettings);
	const T& Get();
	ObjectDetectionConfigValueGen& GetConfigValueStruct(std::string fieldName);
	ObjectDetectionConfigValueGen& GetConfigValueStruct(int valueIndex);
	template<typename O>
	ObjectDetectionConfigValue<O>& GetConfigValueStruct(std::string fieldName);
	template<typename O>
	ObjectDetectionConfigValue<O>& GetConfigValueStruct(int valueIndex);
	template<typename O>
	void AttachSetterFunction(std::string fieldName, std::function<void(const O&)> setterFunction);
	template<typename O>
	void AttachSetterFunction(int valueIndex, std::function<void(const O&)> setterFunction);
};