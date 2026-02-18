#include <ObjectDetectionConfig.h>
#include <TestConfigurationSettings.pb.h>
#include <array>
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <networktables/BooleanArrayTopic.h>
#include <networktables/DoubleArrayTopic.h>
#include <networktables/DoubleTopic.h>
#include <networktables/GenericEntry.h>
#include <networktables/IntegerArrayTopic.h>
#include <networktables/IntegerTopic.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/StringArrayTopic.h>
#include <networktables/StringTopic.h>
#include <networktables/Topic.h>
#include <regex>
#include <sstream>

TEST(ConfigurationSyncer, LoadFromDefaults) {
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(250);
  newConfig.set_cameraoffsetx(25.2);
  newConfig.set_logdirectory("testing");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(false);
  newConfig.add_repeatedint(12);
  newConfig.add_repeatedint(56);
  newConfig.add_repeatedint2(82);
  newConfig.add_repeatedint2(99);
  newConfig.add_repeatedstr("hi");
  newConfig.add_repeatedstr("hello");
  newConfig.add_repeateddouble(5.6);
  newConfig.add_repeateddouble(4.6);
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./LoadFromDefaults.json", newConfig, "ObjConfig");
  ASSERT_EQ(config.Get().huemin(), 250);
  ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 25.2);
  ASSERT_STREQ(config.Get().logdirectory().c_str(), "testing");
  ASSERT_EQ(config.Get().repeatedbool(0), true);
  ASSERT_EQ(config.Get().repeatedbool(1), false);
  ASSERT_EQ(config.Get().repeatedint(0), 12);
  ASSERT_EQ(config.Get().repeatedint(1), 56);
  ASSERT_EQ(config.Get().repeatedint2(0), 82);
  ASSERT_EQ(config.Get().repeatedint2(1), 99);
  ASSERT_STREQ(config.Get().repeatedstr(0).c_str(), "hi");
  ASSERT_STREQ(config.Get().repeatedstr(1).c_str(), "hello");
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(0), 5.6);
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(1), 4.6);
}

TEST(ConfigurationSyncer, LoadFromConfigurationStruct) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./LoadFromConfigurationStruct.json", TestConfigurationSettings(),
      "ObjConfig");
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(250);
  newConfig.set_cameraoffsetx(25.2);
  newConfig.set_logdirectory("testing");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(false);
  newConfig.add_repeatedint(12);
  newConfig.add_repeatedint(56);
  newConfig.add_repeatedint2(82);
  newConfig.add_repeatedint2(99);
  newConfig.add_repeatedstr("hi");
  newConfig.add_repeatedstr("hello");
  newConfig.add_repeateddouble(5.6);
  newConfig.add_repeateddouble(4.6);
  config.LoadConfigFromStructure(newConfig);
  ASSERT_EQ(config.Get().huemin(), 250);
  ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 25.2);
  ASSERT_STREQ(config.Get().logdirectory().c_str(), "testing");
  ASSERT_EQ(config.Get().repeatedbool(0), true);
  ASSERT_EQ(config.Get().repeatedbool(1), false);
  ASSERT_EQ(config.Get().repeatedint(0), 12);
  ASSERT_EQ(config.Get().repeatedint(1), 56);
  ASSERT_EQ(config.Get().repeatedint2(0), 82);
  ASSERT_EQ(config.Get().repeatedint2(1), 99);
  ASSERT_STREQ(config.Get().repeatedstr(0).c_str(), "hi");
  ASSERT_STREQ(config.Get().repeatedstr(1).c_str(), "hello");
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(0), 5.6);
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(1), 4.6);
}

TEST(ConfigurationSyncer, LoadFromConfigurationFile) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./testloadfile.json", TestConfigurationSettings(), "ObjConfig");
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(250);
  newConfig.set_cameraoffsetx(25.2);
  newConfig.set_logdirectory("testing");
  newConfig.add_repeatedbool(false);
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedint(5);
  newConfig.add_repeatedint(2);
  newConfig.add_repeatedint2(15);
  newConfig.add_repeatedint2(14);
  newConfig.add_repeatedstr("testA");
  newConfig.add_repeatedstr("testB");
  newConfig.add_repeateddouble(3.2);
  newConfig.add_repeateddouble(12.4);
  std::string newConfigJson;
  auto status =
      google::protobuf::util::MessageToJsonString(newConfig, &newConfigJson);
  ASSERT_TRUE(status.ok());
  std::ofstream newConfigFile("./testloadfile.json",
                              std::ios::out | std::ios::trunc);
  newConfigFile << newConfigJson;
  newConfigFile.close();
  config.LoadConfigFromFile();
  ASSERT_EQ(config.Get().huemin(), 250);
  ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 25.2);
  ASSERT_STREQ(config.Get().logdirectory().c_str(), "testing");
  ASSERT_EQ(config.Get().repeatedbool(0), false);
  ASSERT_EQ(config.Get().repeatedbool(1), true);
  ASSERT_EQ(config.Get().repeatedint(0), 5);
  ASSERT_EQ(config.Get().repeatedint(1), 2);
  ASSERT_EQ(config.Get().repeatedint2(0), 15);
  ASSERT_EQ(config.Get().repeatedint2(1), 14);
  ASSERT_STREQ(config.Get().repeatedstr(0).c_str(), "testA");
  ASSERT_STREQ(config.Get().repeatedstr(1).c_str(), "testB");
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(0), 3.2);
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(1), 12.4);
}

TEST(ConfigurationSyncer, SaveToConfigurationFile) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./testwritefile.json", TestConfigurationSettings(), "ObjConfig");
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(252);
  newConfig.set_cameraoffsetx(23.2);
  newConfig.set_logdirectory("test2");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedint(10);
  newConfig.add_repeatedint(6);
  newConfig.add_repeatedint2(8);
  newConfig.add_repeatedint2(9);
  newConfig.add_repeatedstr("test1");
  newConfig.add_repeatedstr("test2");
  newConfig.add_repeateddouble(5.5);
  newConfig.add_repeateddouble(1.1);
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
  std::regex boolArrRx(".*repeatedBool\":\\[true,true\\].*");
  std::regex repeatedIntRx(".*repeatedInt\":\\[10,6\\].*");
  // int64s are represented as strings in json due to JS's Number class only
  // having 53 bits of precision
  std::regex repeatedInt2Rx(".*repeatedInt2\":\\[\"8\",\"9\"\\].*");
  std::regex repeatedStrRx(".*repeatedStr\":\\[\"test1\",\"test2\"\\].*");
  std::regex repeatedDoubleRx(".*repeatedDouble\":\\[5.5,1.1\\].*");
  ASSERT_TRUE(std::regex_match(newConfigStr, hueminrx));
  ASSERT_TRUE(std::regex_match(newConfigStr, cameraoffsetrx));
  ASSERT_TRUE(std::regex_match(newConfigStr, logdirectoryrx));
  ASSERT_TRUE(std::regex_match(newConfigStr, boolArrRx));
  ASSERT_TRUE(std::regex_match(newConfigStr, repeatedIntRx));
  ASSERT_TRUE(std::regex_match(newConfigStr, repeatedInt2Rx));
  ASSERT_TRUE(std::regex_match(newConfigStr, repeatedStrRx));
  ASSERT_TRUE(std::regex_match(newConfigStr, repeatedDoubleRx));
}

TEST(ConfigurationSyncer, NetworkTableSyncPublish) {
  nt::NetworkTableInstance ntinst = nt::NetworkTableInstance::GetDefault();
  nt::IntegerSubscriber hueMinSub =
      ntinst.GetIntegerTopic("ObjectDetectionConfig/TestConfigName/hueMin")
          .Subscribe(-1);
  nt::DoubleSubscriber cameraOffsetXSub =
      ntinst
          .GetDoubleTopic("ObjectDetectionConfig/TestConfigName/cameraOffsetX")
          .Subscribe(-1.0);
  nt::StringSubscriber logDirectorySub =
      ntinst.GetStringTopic("ObjectDetectionConfig/TestConfigName/logDirectory")
          .Subscribe("");
  nt::BooleanArraySubscriber repeatedBoolSub =
      ntinst
          .GetBooleanArrayTopic(
              "ObjectDetectionConfig/TestConfigName/repeatedBool")
          .Subscribe({});
  nt::IntegerArraySubscriber repeatedIntSub =
      ntinst
          .GetIntegerArrayTopic(
              "ObjectDetectionConfig/TestConfigName/repeatedInt")
          .Subscribe({});
  nt::IntegerArraySubscriber repeatedInt2Sub =
      ntinst
          .GetIntegerArrayTopic(
              "ObjectDetectionConfig/TestConfigName/repeatedInt2")
          .Subscribe({});
  nt::StringArraySubscriber repeatedStrSub =
      ntinst
          .GetStringArrayTopic(
              "ObjectDetectionConfig/TestConfigName/repeatedStr")
          .Subscribe({});
  nt::DoubleArraySubscriber repeatedDoubleSub =
      ntinst
          .GetDoubleArrayTopic(
              "ObjectDetectionConfig/TestConfigName/repeatedDouble")
          .Subscribe({});
  TestConfigurationSettings defaults;
  defaults.set_huemin(25);
  defaults.set_cameraoffsetx(5.5);
  defaults.set_logdirectory("ntpubdefault");
  defaults.add_repeatedbool(false);
  defaults.add_repeatedbool(false);
  defaults.add_repeatedint(1);
  defaults.add_repeatedint(2);
  defaults.add_repeatedint2(3);
  defaults.add_repeatedint2(4);
  defaults.add_repeatedstr("hello");
  defaults.add_repeatedstr("world");
  defaults.add_repeateddouble(7.5);
  defaults.add_repeateddouble(12.5);
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./NetworkTableSyncPublish.json", defaults, "TestConfigName");
  ASSERT_EQ(hueMinSub.Get(), 25);
  ASSERT_DOUBLE_EQ(cameraOffsetXSub.Get(), 5.5);
  ASSERT_STREQ(logDirectorySub.Get().data(), "ntpubdefault");
  ASSERT_EQ(repeatedBoolSub.Get().size(), 2);
  ASSERT_EQ(repeatedBoolSub.Get()[0] != 0, false);
  ASSERT_EQ(repeatedBoolSub.Get()[1] != 0, false);
  ASSERT_EQ(repeatedIntSub.Get().size(), 2);
  ASSERT_EQ(repeatedIntSub.Get()[0], 1);
  ASSERT_EQ(repeatedIntSub.Get()[1], 2);
  ASSERT_EQ(repeatedInt2Sub.Get().size(), 2);
  ASSERT_EQ(repeatedInt2Sub.Get()[0], 3);
  ASSERT_EQ(repeatedInt2Sub.Get()[1], 4);
  ASSERT_EQ(repeatedStrSub.Get().size(), 2);
  ASSERT_STREQ(repeatedStrSub.Get()[0].c_str(), "hello");
  ASSERT_STREQ(repeatedStrSub.Get()[1].c_str(), "world");
  ASSERT_EQ(repeatedDoubleSub.Get().size(), 2);
  ASSERT_DOUBLE_EQ(repeatedDoubleSub.Get()[0], 7.5);
  ASSERT_DOUBLE_EQ(repeatedDoubleSub.Get()[1], 12.5);
  TestConfigurationSettings newSettings;
  newSettings.set_huemin(75);
  newSettings.set_cameraoffsetx(9.0);
  newSettings.set_logdirectory("ntpub");
  newSettings.add_repeatedbool(true);
  newSettings.add_repeatedbool(true);
  newSettings.add_repeatedint(10);
  newSettings.add_repeatedint(20);
  newSettings.add_repeatedint2(30);
  newSettings.add_repeatedint2(40);
  newSettings.add_repeatedstr("abc");
  newSettings.add_repeatedstr("123");
  newSettings.add_repeateddouble(5.0);
  newSettings.add_repeateddouble(6.5);
  config.LoadConfigFromStructure(newSettings);
  ASSERT_EQ(hueMinSub.Get(), 75);
  ASSERT_DOUBLE_EQ(cameraOffsetXSub.Get(), 9.0);
  ASSERT_STREQ(logDirectorySub.Get().data(), "ntpub");
  ASSERT_EQ(repeatedBoolSub.Get().size(), 2);
  ASSERT_EQ(repeatedBoolSub.Get()[0] != 0, true);
  ASSERT_EQ(repeatedBoolSub.Get()[1] != 0, true);
  ASSERT_EQ(repeatedIntSub.Get().size(), 2);
  ASSERT_EQ(repeatedIntSub.Get()[0], 10);
  ASSERT_EQ(repeatedIntSub.Get()[1], 20);
  ASSERT_EQ(repeatedInt2Sub.Get().size(), 2);
  ASSERT_EQ(repeatedInt2Sub.Get()[0], 30);
  ASSERT_EQ(repeatedInt2Sub.Get()[1], 40);
  ASSERT_EQ(repeatedStrSub.Get().size(), 2);
  ASSERT_STREQ(repeatedStrSub.Get()[0].c_str(), "abc");
  ASSERT_STREQ(repeatedStrSub.Get()[1].c_str(), "123");
  ASSERT_EQ(repeatedDoubleSub.Get().size(), 2);
  ASSERT_DOUBLE_EQ(repeatedDoubleSub.Get()[0], 5.0);
  ASSERT_DOUBLE_EQ(repeatedDoubleSub.Get()[1], 6.5);
}

TEST(ConfigurationSyncer, NetworkTableSyncFetch) {
  nt::NetworkTableInstance ntinst = nt::NetworkTableInstance::GetDefault();
  nt::IntegerPublisher hueMinPub =
      ntinst.GetIntegerTopic("ObjectDetectionConfig/TestConfigFetch/hueMin")
          .Publish();
  nt::DoublePublisher cameraOffsetPub =
      ntinst
          .GetDoubleTopic("ObjectDetectionConfig/TestConfigFetch/cameraOffsetX")
          .Publish();
  nt::StringPublisher logDirectoryPub =
      ntinst
          .GetStringTopic("ObjectDetectionConfig/TestConfigFetch/logDirectory")
          .Publish();
  nt::BooleanArrayPublisher repeatedBoolPub =
      ntinst
          .GetBooleanArrayTopic(
              "ObjectDetectionConfig/TestConfigFetch/repeatedBool")
          .Publish();
  nt::IntegerArrayPublisher repeatedIntPub =
      ntinst
          .GetIntegerArrayTopic(
              "ObjectDetectionConfig/TestConfigFetch/repeatedInt")
          .Publish();
  nt::IntegerArrayPublisher repeatedInt2Pub =
      ntinst
          .GetIntegerArrayTopic(
              "ObjectDetectionConfig/TestConfigFetch/repeatedInt2")
          .Publish();
  nt::StringArrayPublisher repeatedStrPub =
      ntinst
          .GetStringArrayTopic(
              "ObjectDetectionConfig/TestConfigFetch/repeatedStr")
          .Publish();
  nt::DoubleArrayPublisher repeatedDoublePub =
      ntinst
          .GetDoubleArrayTopic(
              "ObjectDetectionConfig/TestConfigFetch/repeatedDouble")
          .Publish();
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./NetworkTableSyncFetch.json", TestConfigurationSettings(),
      "TestConfigFetch");
  hueMinPub.Set(84);
  cameraOffsetPub.Set(18.4);
  logDirectoryPub.Set("fetchSync");
  // false, true
  std::array<int, 2> boolArr = {0, 1};
  repeatedBoolPub.Set(boolArr);
  std::array<int64_t, 2> int1Arr = {15, 25};
  repeatedIntPub.Set(int1Arr);
  std::array<int64_t, 2> int2Arr = {35, 45};
  repeatedInt2Pub.Set(int2Arr);
  std::array<std::string, 2> strArr = {"nt", "publisher"};
  repeatedStrPub.Set(strArr);
  std::array<double, 2> doubleArr = {3.2, 2.6};
  repeatedDoublePub.Set(doubleArr);
  config.UpdateFromNetworkTables();
  ASSERT_EQ(config.Get().huemin(), 84);
  ASSERT_DOUBLE_EQ(config.Get().cameraoffsetx(), 18.4);
  ASSERT_STREQ(config.Get().logdirectory().c_str(), "fetchSync");
  ASSERT_EQ(config.Get().repeatedbool_size(), 2);
  ASSERT_EQ(config.Get().repeatedbool(0), false);
  ASSERT_EQ(config.Get().repeatedbool(1), true);
  ASSERT_EQ(config.Get().repeatedint_size(), 2);
  ASSERT_EQ(config.Get().repeatedint(0), 15);
  ASSERT_EQ(config.Get().repeatedint(1), 25);
  ASSERT_EQ(config.Get().repeatedint2_size(), 2);
  ASSERT_EQ(config.Get().repeatedint2(0), 35);
  ASSERT_EQ(config.Get().repeatedint2(1), 45);
  ASSERT_EQ(config.Get().repeatedstr_size(), 2);
  ASSERT_STREQ(config.Get().repeatedstr(0).c_str(), "nt");
  ASSERT_STREQ(config.Get().repeatedstr(1).c_str(), "publisher");
  ASSERT_EQ(config.Get().repeateddouble_size(), 2);
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(0), 3.2);
  ASSERT_DOUBLE_EQ(config.Get().repeateddouble(1), 2.6);
}

static int setterIntValueReceived = -1;
static double setterDoubleValueReceived = -1;
static std::string setterStringValueReceived = "";
static std::vector<int32_t> setterIntsReceived;
static std::vector<int64_t> setterInts2Received;
static std::vector<double> setterDoublesReceived;
static std::vector<bool> setterBoolsReceived;
static std::vector<std::string> setterStringsReceived;

static void SetterFunction1(const int &newValue) {
  setterIntValueReceived = newValue;
}

static void SetterFunction2(const double &newValue) {
  setterDoubleValueReceived = newValue;
}

static void SetterFunction3(const std::string &newValue) {
  setterStringValueReceived = newValue;
}

static void SetterFunction4(const std::vector<int32_t> &newValue) {
  setterIntsReceived = newValue;
}

static void SetterFunction5(const std::vector<int64_t> &newValue) {
  setterInts2Received = newValue;
}

static void SetterFunction6(const std::vector<double> &newValue) {
  setterDoublesReceived = newValue;
}

static void SetterFunction7(const std::vector<bool> &newValue) {
  setterBoolsReceived = newValue;
}

static void SetterFunction8(const std::vector<std::string> &newValue) {
  setterStringsReceived = newValue;
}

TEST(ConfigurationSyncer, AttachSetterFunctionFieldName) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./AttachSetterFunctionFieldName.json", TestConfigurationSettings(),
      "ObjConfig");
  config.AttachSetterFunction<int>("hueMin", SetterFunction1);
  config.AttachSetterFunction<double>("cameraOffsetX", SetterFunction2);
  config.AttachSetterFunction<std::string>("logDirectory", SetterFunction3);
  config.AttachSetterFunction<std::vector<int32_t>>("repeatedInt",
                                                    SetterFunction4);
  config.AttachSetterFunction<std::vector<int64_t>>("repeatedInt2",
                                                    SetterFunction5);
  config.AttachSetterFunction<std::vector<double>>("repeatedDouble",
                                                   SetterFunction6);
  config.AttachSetterFunction<std::vector<bool>>("repeatedBool",
                                                 SetterFunction7);
  config.AttachSetterFunction<std::vector<std::string>>("repeatedStr",
                                                        SetterFunction8);
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(2);
  newConfig.set_cameraoffsetx(5.0);
  newConfig.set_logdirectory("AttachFieldName");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(false);
  newConfig.add_repeatedint(5);
  newConfig.add_repeatedint(6);
  newConfig.add_repeatedint2(7);
  newConfig.add_repeatedint2(8);
  newConfig.add_repeatedstr("field");
  newConfig.add_repeatedstr("name");
  newConfig.add_repeateddouble(10.2);
  newConfig.add_repeateddouble(14.5);
  config.LoadConfigFromStructure(newConfig);
  ASSERT_EQ(setterIntValueReceived, 2);
  ASSERT_DOUBLE_EQ(setterDoubleValueReceived, 5.0);
  ASSERT_STREQ(setterStringValueReceived.c_str(), "AttachFieldName");
  ASSERT_EQ(setterBoolsReceived[0], true);
  ASSERT_EQ(setterBoolsReceived[1], false);
  ASSERT_EQ(setterIntsReceived[0], 5);
  ASSERT_EQ(setterIntsReceived[1], 6);
  ASSERT_EQ(setterInts2Received[0], 7);
  ASSERT_EQ(setterInts2Received[1], 8);
  ASSERT_STREQ(setterStringsReceived[0].c_str(), "field");
  ASSERT_STREQ(setterStringsReceived[1].c_str(), "name");
  ASSERT_DOUBLE_EQ(setterDoublesReceived[0], 10.2);
  ASSERT_DOUBLE_EQ(setterDoublesReceived[1], 14.5);
}

TEST(ConfigurationSyncer, AttachSetterFunctionFieldIndex) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./AttachSetterFunctionFieldIndex.json", TestConfigurationSettings(),
      "ObjConfig");
  config.AttachSetterFunction<int>(1, SetterFunction1);
  config.AttachSetterFunction<double>(14, SetterFunction2);
  config.AttachSetterFunction<std::string>(20, SetterFunction3);
  config.AttachSetterFunction<std::vector<int32_t>>(22, SetterFunction4);
  config.AttachSetterFunction<std::vector<int64_t>>(23, SetterFunction5);
  config.AttachSetterFunction<std::vector<double>>(26, SetterFunction6);
  config.AttachSetterFunction<std::vector<bool>>(25, SetterFunction7);
  config.AttachSetterFunction<std::vector<std::string>>(24, SetterFunction8);
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(4);
  newConfig.set_cameraoffsetx(3.0);
  newConfig.set_logdirectory("AttachFieldIndex");
  newConfig.add_repeatedbool(false);
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedint(9);
  newConfig.add_repeatedint(10);
  newConfig.add_repeatedint2(11);
  newConfig.add_repeatedint2(12);
  newConfig.add_repeatedstr("field");
  newConfig.add_repeatedstr("index");
  newConfig.add_repeateddouble(5.5);
  newConfig.add_repeateddouble(5.8);
  config.LoadConfigFromStructure(newConfig);
  ASSERT_EQ(setterIntValueReceived, 4);
  ASSERT_DOUBLE_EQ(setterDoubleValueReceived, 3.0);
  ASSERT_STREQ(setterStringValueReceived.c_str(), "AttachFieldIndex");
  ASSERT_EQ(setterBoolsReceived[0], false);
  ASSERT_EQ(setterBoolsReceived[1], true);
  ASSERT_EQ(setterIntsReceived[0], 9);
  ASSERT_EQ(setterIntsReceived[1], 10);
  ASSERT_EQ(setterInts2Received[0], 11);
  ASSERT_EQ(setterInts2Received[1], 12);
  ASSERT_STREQ(setterStringsReceived[0].c_str(), "field");
  ASSERT_STREQ(setterStringsReceived[1].c_str(), "index");
  ASSERT_DOUBLE_EQ(setterDoublesReceived[0], 5.5);
  ASSERT_DOUBLE_EQ(setterDoublesReceived[1], 5.8);
}

TEST(ConfigurationSyncer, GetConfigValueStructFieldIndex) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./GetConfigValueStructFieldIndex.json", TestConfigurationSettings(),
      "ObjConfig");
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(150);
  newConfig.set_cameraoffsetx(3.4);
  newConfig.set_logdirectory("test3");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedint(13);
  newConfig.add_repeatedint(14);
  newConfig.add_repeatedint2(15);
  newConfig.add_repeatedint2(16);
  newConfig.add_repeatedstr("struct");
  newConfig.add_repeatedstr("name");
  newConfig.add_repeateddouble(5.5);
  newConfig.add_repeateddouble(5.8);
  config.LoadConfigFromStructure(newConfig);
  ASSERT_ANY_THROW(config.GetConfigValueStruct<int>(0));
  ASSERT_ANY_THROW(config.GetConfigValueStruct(0));
  ASSERT_EQ(config.GetConfigValueStruct<int>(1).GetValue(), 150);
  ASSERT_DOUBLE_EQ(config.GetConfigValueStruct<double>(14).GetValue(), 3.4);
  ASSERT_STREQ(config.GetConfigValueStruct<std::string>(20).GetValue().c_str(),
               "test3");
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int32_t>>(22).GetValue()[0],
            13);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int32_t>>(22).GetValue()[1],
            14);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int64_t>>(23).GetValue()[0],
            15);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int64_t>>(23).GetValue()[1],
            16);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<bool>>(25).GetValue()[0],
            true);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<bool>>(25).GetValue()[1],
            true);
  ASSERT_STREQ(config.GetConfigValueStruct<std::vector<std::string>>(24)
                   .GetValue()[0]
                   .c_str(),
               "struct");
  ASSERT_STREQ(config.GetConfigValueStruct<std::vector<std::string>>(24)
                   .GetValue()[1]
                   .c_str(),
               "name");
  ASSERT_DOUBLE_EQ(
      config.GetConfigValueStruct<std::vector<double>>(26).GetValue()[0], 5.5);
  ASSERT_DOUBLE_EQ(
      config.GetConfigValueStruct<std::vector<double>>(26).GetValue()[1], 5.8);
}

TEST(ConfigurationSyncer, GetConfigValueStructFieldName) {
  ObjectDetectionConfig<TestConfigurationSettings> config(
      "./GetConfigValueStructFieldName.json", TestConfigurationSettings(),
      "ObjConfig");
  TestConfigurationSettings newConfig;
  newConfig.set_huemin(20);
  newConfig.set_cameraoffsetx(3.1);
  newConfig.set_logdirectory("test4");
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedbool(true);
  newConfig.add_repeatedint(13);
  newConfig.add_repeatedint(14);
  newConfig.add_repeatedint2(15);
  newConfig.add_repeatedint2(16);
  newConfig.add_repeatedstr("struct");
  newConfig.add_repeatedstr("name");
  newConfig.add_repeateddouble(5.5);
  newConfig.add_repeateddouble(5.8);
  config.LoadConfigFromStructure(newConfig);
  ASSERT_ANY_THROW(config.GetConfigValueStruct("SomeNonExistantField"));
  ASSERT_ANY_THROW(config.GetConfigValueStruct<int>("SomeNonExistantField"));
  ASSERT_EQ(config.GetConfigValueStruct<int>("hueMin").GetValue(), 20);
  ASSERT_DOUBLE_EQ(
      config.GetConfigValueStruct<double>("cameraOffsetX").GetValue(), 3.1);
  ASSERT_STREQ(config.GetConfigValueStruct<std::string>("logDirectory")
                   .GetValue()
                   .c_str(),
               "test4");
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int32_t>>("repeatedInt")
                .GetValue()[0],
            13);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int32_t>>("repeatedInt")
                .GetValue()[1],
            14);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int64_t>>("repeatedInt2")
                .GetValue()[0],
            15);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<int64_t>>("repeatedInt2")
                .GetValue()[1],
            16);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<bool>>("repeatedBool")
                .GetValue()[0],
            true);
  ASSERT_EQ(config.GetConfigValueStruct<std::vector<bool>>("repeatedBool")
                .GetValue()[1],
            true);
  ASSERT_STREQ(
      config.GetConfigValueStruct<std::vector<std::string>>("repeatedStr")
          .GetValue()[0]
          .c_str(),
      "struct");
  ASSERT_STREQ(
      config.GetConfigValueStruct<std::vector<std::string>>("repeatedStr")
          .GetValue()[1]
          .c_str(),
      "name");
  ASSERT_DOUBLE_EQ(
      config.GetConfigValueStruct<std::vector<double>>("repeatedDouble")
          .GetValue()[0],
      5.5);
  ASSERT_DOUBLE_EQ(
      config.GetConfigValueStruct<std::vector<double>>("repeatedDouble")
          .GetValue()[1],
      5.8);
}