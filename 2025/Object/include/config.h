#pragma once
#include "LoggingUtils.h"
/*! \brief Class for loading / saving configs from disk. 
Depends on: 
- nlohmannjson */

// JSON field names
#define SETTING_HUE_MIN "hueMin"
#define SETTING_HUE_MAX "hueMax"
#define SETTING_SATURATION_MIN "satMin"
#define SETTING_SATURATION_MAX "satMax"
#define SETTING_VALUE_MIN "valMin"
#define SETTING_VALUE_MAX "valMax"
#define SETTING_MIN_PIXEL_WIDTH "minPixelWidth"
#define SETTING_MAX_PIXEL_WIDTH "maxPixelWidth"
#define SETTING_MIN_PIXEL_HEIGHT "minPixelHeight"
#define SETTING_MAX_PIXEL_HEIGHT "maxPixelHeight"
#define SETTING_MIN_PIXEL_AREA "minPixelArea"
#define SETTING_MAX_PIXEL_AREA "maxPixelArea"
#define SETTING_MIN_PERCENT_FILL "minPercentFill"
#define SETTING_CAM_OFFSET_X "camOffsetX"
#define SETTING_CAM_OFFSET_Y "camOffsetY"
#define SETTING_CAM_HEIGHT "camHeight"
#define SETTING_CAM_ANGLE "camAngle"
#define SETTING_FILTER_WINDOW "filterWindow"
#define SETTINGS_FILE_PATH "./processorSettings.txt"
#include <nlohmann/json.hpp>
#include <fstream>
#include "object.h"
 
using json = nlohmann::json;

/*! \brief Class representing the parameters used for object detection */
class imageProcessingFilterSettings
{
public:
	int hueMin;
	int hueMax;
	int saturationMin;
	int saturationMax;
	int valueMin;
	int valueMax;
	int minPixelWidth;
	int maxPixelWidth;
	int minPixelHeight;
	int maxPixelHeight;
	int minPixelArea;
	int maxPixelArea;
	double minPercentFill;
	double camOffsetX;
	double camOffsetY;
	double camHeight;
	double camAngle;
	int filterWindow;
 
	/*!  
	* \brief Fetches the object detection settings from json 
	* \param  jsonString - a JSON string that contains the config data, must be checked before passing into this function
	*/
	imageProcessingFilterSettings(std::string jsonString)
	{
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

	/*!  
	* \brief Initialize an instance of the class from the settings
	* \param  hueMin - integer representing the hueMin parameter
	* \param  hueMax - integer representing the hueMax parameter
	* \param  saturationMin - integer representing the saturationMin parameter
	* \param  saturationMax - integer representing the satuartionMax parameter
	* \param  valueMin - integer representing the valueMin parameter
	* \param  valueMax - integer representing the valueMax parameter
	* \param  minPixelWidth - integer representing the minimum pixel width of a detection for it to be valid
	* \param  maxPixelWidth - integer representing the maximum pixel width of a detection for it to be valid
	* \param  minPixelHeight - integer representing the minimum pixel height of a detection for it to be valid
	* \param  maxPixelHeight - integer representing the maximum pixel height of a detection for it to be valid
	* \param  minPixelArea - integer representing the minimum pixel area of a detection for it to be valid
	* \param  maxPixelArea - integer representing the maximum pixel area of a detection for it to be valid
	* \param  minPercentFill - percent(eg 0.8) representing the minimum % of pixels in a detection that have to be valid for the detection to be valid
	* \param  camOffsetX - X Distance between camera center and center of robot in meters
	* \param  camOffsetY - Y Distance between camera center and center of robot in meters
	* \param  camHeight - vertical distance between camera center and center of robot in meters
	* \param  camAngle - Angle below the horizontal of camera in radians
	* \param filterWindow - Number of packets over which the median is taken for angle/distance values
	*/
	imageProcessingFilterSettings(int hueMin, int hueMax, int saturationMin, int saturationMax, int valueMin, int valueMax,
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

	/*!  
	* \brief Write the settings stored in the class to the config file at location \ref SETTINGS_FILE_PATH 
	*/
	void writeSettings()
	{
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
};

/*!  
* \brief Class containing the static functions for interacting with imageProcessingFilterSettings and the config file. 
* \brief Cannot be instantiated
*/
class ConfigUtils
{
public:
	static int m_npreviousHueMin;
	static int m_npreviousHueMax;
	static int m_npreviousSaturationMin;
	static int m_npreviousSaturationMax;
	static int m_npreviousValueMin;
	static int m_npreviousValueMax;
	static int m_previousMinPixelWidth;
	static int m_previousMaxPixelWidth;
	static int m_previousMinPixelHeight;
	static int m_previousMaxPixelHeight;
	static int m_previousMinPixelArea;
	static int m_previousMaxPixelArea;
	static double m_previousMinPercentFill;
	static double m_previousCamOffsetX;
	static double m_previousCamOffsetY;
	static double m_previousCamHeight;
	static double m_previousCamAngle;
	static int m_previousFilterWindow;
	/*!  
	* \brief Returns true if image processor's configuration is different from what is currently written to the config file. 
	* \brief Otherwise returns false.
	* \param  imageProcessor - the \ref imgProc with the current settings being used for object detection
	*/
	static bool configHasChanged(imgProc &imageProcessor)
	{
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
		if(m_previousMaxPixelHeight != imageProcessor.getMaxPixelHeight())
		{
			return true;
		}
		if(m_previousMaxPixelArea != imageProcessor.getMaxPixelArea())
		{
			return true;
		}
		if(m_previousMinPercentFill != imageProcessor.getMinPercentFill())
		{
			return true;
		}
		if(m_previousCamOffsetX != imageProcessor.getCamOffsetX())
		{
			return true;
		}
		if(m_previousCamOffsetY != imageProcessor.getCamOffsetY())
		{
			return true;
		}
		if(m_previousCamHeight != imageProcessor.getCamHeight())
		{
			return true;
		}
		if(m_previousCamAngle != imageProcessor.getCamAngle())
		{
			return true;
		}
		if(m_previousFilterWindow != imageProcessor.getFilterWindow())
		{
			return true;
		}
		return false;
	}
	/*!  
	* \brief Loads the config from the file and effects the settings in the image processor. 
	* \brief Looks for config file at \ref SETTINGS_FILE_PATH
	* \param  imageProcessor - The image processor to set the config on
	*/
	static void triggerConfigLoad(imgProc &imageProcessor)
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
	}
	/*!  
	* \brief Writes a config save file at \ref SETTINGS_FILE_PATH with the current settings from the image processor.
	* \param  imageProcessor - the image processor with the settings to save to the config file
	*/
	static void triggerConfigSave(imgProc &imageProcessor)
	{
		imageProcessingFilterSettings config = imageProcessingFilterSettings(imageProcessor.getPVHueMin(), imageProcessor.getPVHueMax(),
																			 imageProcessor.getSaturationMin(), imageProcessor.getSaturationMax(), imageProcessor.getValueMin(), imageProcessor.getValueMax(),
																			imageProcessor.getMinPixelWidth(), imageProcessor.getMaxPixelWidth(), imageProcessor.getMinPixelHeight(), imageProcessor.getMaxPixelHeight(),
																		imageProcessor.getMinPixelArea(), imageProcessor.getMaxPixelArea(), imageProcessor.getMinPercentFill(),
																	imageProcessor.getCamOffsetX(), imageProcessor.getCamOffsetY(), imageProcessor.getCamHeight(), imageProcessor.getCamAngle(),
																imageProcessor.getFilterWindow());
		config.writeSettings();
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
	}
};
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
