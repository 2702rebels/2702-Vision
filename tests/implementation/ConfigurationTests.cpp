#include <gtest/gtest.h>
#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <sstream>
#include <regex>

TEST(ConfigurationSyncer, LoadFromConfigurationStruct) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./kakasas.json", ObjectDetectionSettings(), "ObjConfig");
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
	newConfig.set_huemin(250);
	newConfig.set_cameraoffsetx(25.2);
	newConfig.set_logdirectory("testing");
	config.LoadConfigFromStructure(newConfig);
	config.WriteConfigToFile();
	std::ifstream newConfigFile("./testloadfile.json", std::ios::in);
	std::ostringstream ss;
	ss << newConfigFile.rdbuf();
	std::string newConfigStr = ss.str();
	newConfigFile.close();
	std::regex hueminrx(".*hueMin\":250.*");
	std::regex cameraoffsetrx(".*cameraOffsetX\":25.2.*");
	std::regex logdirectoryrx(".*logDirectory\":\"testing\"");
	ASSERT_TRUE(std::regex_match(newConfigStr, hueminrx));
	ASSERT_TRUE(std::regex_match(newConfigStr, cameraoffsetrx));
	ASSERT_TRUE(std::regex_match(newConfigStr, logdirectoryrx));
}