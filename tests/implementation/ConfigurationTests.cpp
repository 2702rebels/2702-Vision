#include <gtest/gtest.h>
#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>

TEST(ConfigurationSyncer, LoadFromConfigurationStruct) {
	ObjectDetectionConfig<ObjectDetectionSettings> config("./kakasas.json", ObjectDetectionSettings(), "ObjConfig");
	ObjectDetectionSettings newConfig;
	newConfig.set_huemin(250);
	newConfig.set_cameraoffsetx(25.2);
	newConfig.set_logdirectory("testing");
	config.LoadConfigFromStructure(newConfig);
}