#include <gtest/gtest.h>
#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <sstream>
#include <regex>

TEST(ConfigurationSyncer, LoadFromConfigurationStruct) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./LoadFromConfigurationStruct.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(250);
	newConfig.set_cameraoffsetx(25.2);
	newConfig.set_logdirectory("testing");
	config.LoadConfigFromStructure(newConfig);
	ASSERT_EQ(config.Get().huemin(), 250);
	ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 25.2);
	ASSERT_STREQ(config.Get().logdirectory().c_str(), "testing");
}

TEST(ConfigurationSyncer, LoadFromConfigurationFile) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./testloadfile.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(250);
	newConfig.set_cameraoffsetx(25.2);
	newConfig.set_logdirectory("testing");
	std::string newConfigJson;
	auto status = google::protobuf::util::MessageToJsonString(newConfig, &newConfigJson);
	ASSERT_TRUE(status.ok());
	std::ofstream newConfigFile("./testloadfile.json", std::ios::out | std::ios::trunc);
	newConfigFile << newConfigJson;
	newConfigFile.close();
	config.LoadConfigFromFile();
	ASSERT_EQ(config.Get().huemin(), 250);
	ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 25.2);
	ASSERT_STREQ(config.Get().logdirectory().c_str(), "testing");
}

TEST(ConfigurationSyncer, SaveToConfigurationFile) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./testwritefile.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(252);
	newConfig.set_cameraoffsetx(23.2);
	newConfig.set_logdirectory("test2");
	config.LoadConfigFromStructure(newConfig);
	config.WriteConfigToFile();
	std::ifstream newConfigFile("./testwritefile.json", std::ios::in);
	std::ostringstream ss;
	ss << newConfigFile.rdbuf();
	std::string newConfigStr = ss.str();
	newConfigFile.close();
	std::regex hueminrx(".*hueMin\":252.*");
	std::regex cameraoffsetrx(".*cameraOffsetX\":23.2.*");
	std::regex logdirectoryrx(".*logDirectory\":\"test2\".*");
	ASSERT_TRUE(std::regex_match(newConfigStr, hueminrx));
	ASSERT_TRUE(std::regex_match(newConfigStr, cameraoffsetrx));
	ASSERT_TRUE(std::regex_match(newConfigStr, logdirectoryrx));
}

TEST(ConfigurationSyncer, NetworkTableSyncPublish) {

}

TEST(ConfigurationSyncer, NetworkTableSyncFetch) {

}

TEST(ConfigurationSyncer, AttachSetterFunctionFieldName) {
	
}

TEST(ConfigurationSyncer, GetConfigValueStructFieldName) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./GetConfigValueStructFieldName.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(150);
	newConfig.set_cameraoffsetx(3.4);
	newConfig.set_logdirectory("test3");
	config.LoadConfigFromStructure(newConfig);
	ASSERT_ANY_THROW(config.GetConfigValueStruct<int>(0));
	ASSERT_ANY_THROW(config.GetConfigValueStruct(0));
	ASSERT_EQ(config.GetConfigValueStruct<int>(1).GetValue(), 150);
	ASSERT_DOUBLE_EQ(config.GetConfigValueStruct<double>(14).GetValue(), 3.4);
	ASSERT_STREQ(config.GetConfigValueStruct<std::string>(20).GetValue().c_str(), "test3");
}

TEST(ConfigurationSyncer, GetConfigValueStructFieldIndex) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./GetConfigValueStructFieldIndex.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(20);
	newConfig.set_cameraoffsetx(3.1);
	newConfig.set_logdirectory("test4");
	config.LoadConfigFromStructure(newConfig);
	ASSERT_ANY_THROW(config.GetConfigValueStruct("SomeNonExistantField"));
	ASSERT_ANY_THROW(config.GetConfigValueStruct<int>("SomeNonExistantField"));
	ASSERT_EQ(config.GetConfigValueStruct<int>("hueMin").GetValue(), 20);
	ASSERT_DOUBLE_EQ(config.GetConfigValueStruct<double>("cameraOffsetX"), 3.1);
	ASSERT_STREQ(config.GetConfigValueStruct<std::string>("logDirectory").GetValue().c_str(), "test4");
}