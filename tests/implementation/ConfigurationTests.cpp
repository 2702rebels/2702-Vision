#include <gtest/gtest.h>
#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <networktables/IntegerTopic.h>
#include <networktables/DoubleTopic.h>
#include <networktables/StringTopic.h>
#include <networktables/Topic.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/GenericEntry.h>

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
	nt::NetworkTableInstance ntinst = nt::NetworkTableInstance::GetDefault();
	nt::IntegerTopic hueMinTopic = ntinst.GetIntegerTopic("ObjectDetectionConfig/TestConfigName/hueMin");
	nt::DoubleTopic cameraOffsetXTopic = ntinst.GetDoubleTopic("ObjectDetectionConfig/TestConfigName/cameraOffsetX");
	nt::Topic logDirectoryTopic = ntinst.GetTopic("ObjectDetectionConfig/TestConfigName/logDirectory");
	nt::IntegerSubscriber hueMinSub = hueMinTopic.Subscribe(-1);
	nt::DoubleSubscriber cameraOffsetXSub = cameraOffsetXTopic.Subscribe(-1.0);
	nt::GenericSubscriber logDirectorySub = logDirectoryTopic.GenericSubscribe();
	ObjectDetectionSettings defaults;
	defaults.set_huemin(25);
	defaults.set_cameraoffsetx(5.5);
	defaults.set_logdirectory("ntpubdefault");
	ObjectDetectionConfig<ObjectDetectionSettings> config("./NetworkTableSyncPublish.json", defaults, "TestConfigName");
	ASSERT_EQ(hueMinSub.Get(), 25);
	ASSERT_DOUBLE_EQ(cameraOffsetXSub.Get(), 5.5);
	ASSERT_STREQ(logDirectorySub.Get().GetString().data(), "ntpubdefault");
	ObjectDetectionSettings newSettings;
	newSettings.set_huemin(75);
	newSettings.set_cameraoffsetx(9.0);
	newSettings.set_logdirectory("ntpub");
	config.LoadConfigFromStructure(newSettings);
	ASSERT_EQ(hueMinSub.Get(), 75);
	ASSERT_DOUBLE_EQ(cameraOffsetXSub.Get(), 9.0);
	ASSERT_STREQ(logDirectorySub.Get().GetString().data(), "ntpub");
}

TEST(ConfigurationSyncer, NetworkTableSyncFetch) {

}

static int setterIntValueReceived = -1;
static double setterDoubleValueReceived = -1;
static std::string setterStringValueReceived = "";

static void SetterFunction1(const int& newValue) {
	setterIntValueReceived = newValue;
}

static void SetterFunction2(const double& newValue) {
	setterDoubleValueReceived = newValue;
}

static void SetterFunction3(const std::string& newValue) {
	setterStringValueReceived = newValue;
}

TEST(ConfigurationSyncer, AttachSetterFunctionFieldName) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./AttachSetterFunctionFieldName.json", ObjectDetectionSettings(), "ObjConfig");
	config.AttachSetterFunction<int>("hueMin", SetterFunction1);
	config.AttachSetterFunction<double>("cameraOffsetX", SetterFunction2);
	config.AttachSetterFunction<std::string>("logDirectory", SetterFunction3);
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(2);
	newConfig.set_cameraoffsetx(5.0);
	newConfig.set_logdirectory("AttachFieldName");
	config.LoadConfigFromStructure(newConfig);
	ASSERT_EQ(setterIntValueReceived, 2);
	ASSERT_DOUBLE_EQ(setterDoubleValueReceived, 5.0);
	ASSERT_STREQ(setterStringValueReceived.c_str(), "AttachFieldName");
}

TEST(ConfigurationSyncer, AttachSetterFunctionFieldIndex) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./AttachSetterFunctionFieldIndex.json", ObjectDetectionSettings(), "ObjConfig");
	config.AttachSetterFunction<int>(1, SetterFunction1);
	config.AttachSetterFunction<double>(14, SetterFunction2);
	config.AttachSetterFunction<std::string>(20, SetterFunction3);
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(3);
	newConfig.set_cameraoffsetx(6.0);
	newConfig.set_logdirectory("AttachFieldIndex");
	config.LoadConfigFromStructure(newConfig);
	ASSERT_EQ(setterIntValueReceived, 3);
	ASSERT_DOUBLE_EQ(setterDoubleValueReceived, 6.0);
	ASSERT_STREQ(setterStringValueReceived.c_str(), "AttachFieldIndex");
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
	ASSERT_DOUBLE_EQ(config.GetConfigValueStruct<double>("cameraOffsetX").GetValue(), 3.1);
	ASSERT_STREQ(config.GetConfigValueStruct<std::string>("logDirectory").GetValue().c_str(), "test4");
}