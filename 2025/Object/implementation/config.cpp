#include "config.h"

imageProcessingFilterSettings::imageProcessingFilterSettings(std::string jsonString) {
	json filterSettings = json::parse(jsonString);
	this->hueMin = filterSettings[SETTING_HUE_MIN];
	this->hueMax = filterSettings[SETTING_HUE_MAX];
	this->saturationMin = filterSettings[SETTING_SATURATION_MIN];
	this->saturationMax = filterSettings[SETTING_SATURATION_MAX];
	this->valueMin = filterSettings[SETTING_VALUE_MIN];
	this->valueMax = filterSettings[SETTING_VALUE_MAX];
	this->minPixelWidth = filterSettings[SETTING_MIN_PIXEL_WIDTH];
	this->maxPixelWidth = filterSettings[SETTING_MAX_PIXEL_WIDTH];
	this->minPixelHeight = filterSettings[SETTING_MIN_PIXEL_HEIGHT];
	this->maxPixelHeight = filterSettings[SETTING_MAX_PIXEL_HEIGHT];
	this->minPixelArea = filterSettings[SETTING_MIN_PIXEL_AREA];
	this->maxPixelArea = filterSettings[SETTING_MAX_PIXEL_AREA];
	this->minPercentFill = filterSettings[SETTING_MIN_PERCENT_FILL];
	this->camOffsetX = filterSettings[SETTING_CAM_OFFSET_X];
	this->camOffsetY = filterSettings[SETTING_CAM_OFFSET_Y];
	this->camHeight = filterSettings[SETTING_CAM_HEIGHT];
	this->camAngle = filterSettings[SETTING_CAM_ANGLE];
	this->filterWindow = filterSettings[SETTING_FILTER_WINDOW];
}

imageProcessingFilterSettings::imageProcessingFilterSettings(int hueMin, int hueMax, int saturationMin, int saturationMax, int valueMin, int valueMax,
	int minPixelWidth, int maxPixelWidth, int minPixelHeight, int maxPixelHeight, int minPixelArea, int maxPixelArea, double minPercentFill,
	double camOffsetX, double camOffsetY, double camHeight, double camAngle, int filterWindow)
{
	this->hueMin = hueMin;
	this->hueMax = hueMax;
	this->saturationMin = saturationMin;
	this->saturationMax = saturationMax;
	this->valueMin = valueMin;
	this->valueMax = valueMax;
	this->minPixelWidth = minPixelWidth;
	this->maxPixelWidth = maxPixelWidth;
	this->minPixelHeight = minPixelHeight;
	this->maxPixelHeight = maxPixelHeight;
	this->minPixelArea = minPixelArea;
	this->maxPixelArea = maxPixelArea;
	this->minPercentFill = minPercentFill;
	this->camOffsetX = camOffsetX;
	this->camOffsetY = camOffsetY;
	this->camHeight = camHeight;
	this->camAngle = camAngle;
	this->filterWindow = filterWindow;
}

void imageProcessingFilterSettings::writeSettings() {
	std::ofstream settingsfile;
	settingsfile.open(SETTINGS_FILE_PATH, std::ios::trunc);
	json settingsdata;
	settingsdata[SETTING_HUE_MIN] = hueMin;
	settingsdata[SETTING_HUE_MAX] = hueMax;
	settingsdata[SETTING_SATURATION_MIN] = saturationMin;
	settingsdata[SETTING_SATURATION_MAX] = saturationMax;
	settingsdata[SETTING_VALUE_MIN] = valueMin;
	settingsdata[SETTING_VALUE_MAX] = valueMax;
	settingsdata[SETTING_MIN_PIXEL_WIDTH] = minPixelWidth;
	settingsdata[SETTING_MIN_PIXEL_HEIGHT] = minPixelHeight;
	settingsdata[SETTING_MAX_PIXEL_WIDTH] = maxPixelWidth;
	settingsdata[SETTING_MAX_PIXEL_HEIGHT] = maxPixelHeight;
	settingsdata[SETTING_MIN_PIXEL_AREA] = minPixelArea;
	settingsdata[SETTING_MAX_PIXEL_AREA] = maxPixelArea;
	settingsdata[SETTING_MIN_PERCENT_FILL] = minPercentFill;
	settingsdata[SETTING_CAM_OFFSET_X] = camOffsetX;
	settingsdata[SETTING_CAM_OFFSET_Y] = camOffsetY;
	settingsdata[SETTING_CAM_HEIGHT] = camHeight;
	settingsdata[SETTING_CAM_ANGLE] = camAngle;
	settingsdata[SETTING_FILTER_WINDOW] = filterWindow;
	const std::string jsonStr = settingsdata.dump().c_str();
	settingsfile.write(jsonStr.c_str(), jsonStr.length());
	settingsfile.close();
}

bool ConfigUtils::configHasChanged(ObjectFrameProcessor& imageProcessor) {
	if (m_npreviousHueMin != imageProcessor.getPVHueMin())
	{
		return true;
	}
	if (m_npreviousHueMax != imageProcessor.getPVHueMax())
	{
		return true;
	}
	if (m_npreviousSaturationMin != imageProcessor.getSaturationMin())
	{
		return true;
	}
	if (m_npreviousSaturationMax != imageProcessor.getSaturationMax())
	{
		return true;
	}
	if (m_npreviousValueMin != imageProcessor.getValueMin())
	{
		return true;
	}
	if (m_npreviousValueMax != imageProcessor.getValueMax())
	{
		return true;
	}
	if (m_previousMinPixelWidth != imageProcessor.getMinPixelWidth())
	{
		return true;
	}
	if (m_previousMinPixelHeight != imageProcessor.getMinPixelHeight())
	{
		return true;
	}
	if (m_previousMinPixelArea != imageProcessor.getMinPixelArea())
	{
		return true;
	}
	if (m_previousMaxPixelWidth != imageProcessor.getMaxPixelWidth())
	{
		return true;
	}
	if (m_previousMaxPixelHeight != imageProcessor.getMaxPixelHeight())
	{
		return true;
	}
	if (m_previousMaxPixelArea != imageProcessor.getMaxPixelArea())
	{
		return true;
	}
	if (m_previousMinPercentFill != imageProcessor.getMinPercentFill())
	{
		return true;
	}
	if (m_previousCamOffsetX != imageProcessor.getCamOffsetX())
	{
		return true;
	}
	if (m_previousCamOffsetY != imageProcessor.getCamOffsetY())
	{
		return true;
	}
	if (m_previousCamHeight != imageProcessor.getCamHeight())
	{
		return true;
	}
	if (m_previousCamAngle != imageProcessor.getCamAngle())
	{
		return true;
	}
	if (m_previousFilterWindow != imageProcessor.getFilterWindow())
	{
		return true;
	}
	return false;
}

static void imageProcessingFilterSettings::triggerConfigLoad(ObjectFrameProcessor& imageProcessor)
{
	std::ifstream file(SETTINGS_FILE_PATH);
	if (file.is_open())
	{
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string jsonConfig = buffer.str();
		file.close();
		imageProcessingFilterSettings config = imageProcessingFilterSettings(jsonConfig);
		imageProcessor.setPVHueMin(config.hueMin);
		imageProcessor.setPVHueMax(config.hueMax);
		imageProcessor.setSaturationMin(config.saturationMin);
		imageProcessor.setSaturationMax(config.saturationMax);
		imageProcessor.setValueMin(config.valueMin);
		imageProcessor.setValueMax(config.valueMax);
		imageProcessor.setMinPixelWidth(config.minPixelWidth);
		imageProcessor.setMinPixelHeight(config.minPixelHeight);
		imageProcessor.setMinPixelArea(config.minPixelArea);
		imageProcessor.setMaxPixelWidth(config.maxPixelWidth);
		imageProcessor.setMaxPixelHeight(config.maxPixelHeight);
		imageProcessor.setMaxPixelArea(config.maxPixelArea);
		imageProcessor.setMinPercentFill(config.minPercentFill);
		imageProcessor.setCamOffsetX(config.camOffsetX);
		imageProcessor.setCamOffsetY(config.camOffsetY);
		imageProcessor.setCamHeight(config.camHeight);
		imageProcessor.setCamAngle(config.camAngle);
		imageProcessor.setFilterWindow(config.filterWindow);
		m_npreviousHueMin = config.hueMin;
		m_npreviousHueMax = config.hueMax;
		m_npreviousSaturationMin = config.saturationMin;
		m_npreviousSaturationMax = config.saturationMax;
		m_npreviousValueMin = config.valueMin;
		m_npreviousValueMax = config.valueMax;
		m_previousMinPixelWidth = config.minPixelWidth;
		m_previousMinPixelHeight = config.minPixelHeight;
		m_previousMinPixelArea = config.minPixelArea;
		m_previousMaxPixelWidth = config.maxPixelWidth;
		m_previousMaxPixelHeight = config.maxPixelHeight;
		m_previousMaxPixelArea = config.maxPixelArea;
		m_previousMinPercentFill = config.minPercentFill;
		m_previousCamOffsetX = config.camOffsetX;
		m_previousCamOffsetY = config.camOffsetY;
		m_previousCamHeight = config.camHeight;
		m_previousCamAngle = config.camAngle;
		m_previousFilterWindow = config.filterWindow;
		info("Loaded the following config from disk:");
		info("hue min: {}", config.hueMin);
		info("hue max: {}", config.hueMax);
		info("sat min: {}", config.saturationMin);
		info("sat max: {}", config.saturationMax);
		info("value min: {}", config.valueMin);
		info("value max: {}", config.valueMax);
		info("min pixel width: {}", config.minPixelWidth);
		info("min pixel height: {}", config.minPixelHeight);
		info("min pixel area: {}", config.minPixelArea);
		info("max pixel width: {}", config.maxPixelWidth);
		info("max pixel height: {}", config.maxPixelHeight);
		info("max pixel area: {}", config.maxPixelArea);
		info("min pixel width: {}", config.minPercentFill);
		info("cam offset x: {}", config.camOffsetX);
		info("cam offset y: {}", config.camOffsetY);
		info("cam height: {}", config.camHeight);
		info("cam angle: {}", config.camAngle);
		info("filter window: {}", config.filterWindow);
	}

	// Instantiate the static values
	int ConfigUtils::m_npreviousHueMin;
	int ConfigUtils::m_npreviousHueMax;
	int ConfigUtils::m_npreviousSaturationMin;
	int ConfigUtils::m_npreviousSaturationMax;
	int ConfigUtils::m_npreviousValueMin;
	int ConfigUtils::m_npreviousValueMax;
	int ConfigUtils::m_previousMinPixelWidth;
	int ConfigUtils::m_previousMaxPixelWidth;
	int ConfigUtils::m_previousMinPixelHeight;
	int ConfigUtils::m_previousMaxPixelHeight;
	int ConfigUtils::m_previousMinPixelArea;
	int ConfigUtils::m_previousMaxPixelArea;
	double ConfigUtils::m_previousMinPercentFill;
	double ConfigUtils::m_previousCamOffsetX;
	double ConfigUtils::m_previousCamOffsetY;
	double ConfigUtils::m_previousCamHeight;
	double ConfigUtils::m_previousCamAngle;
	int ConfigUtils::m_previousFilterWindow;