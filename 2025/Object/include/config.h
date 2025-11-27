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
#include <ObjectFrameProcessor.h>

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
	imageProcessingFilterSettings(std::string jsonString);

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
		double camOffsetX, double camOffsetY, double camHeight, double camAngle, int filterWindow);

	/*!  
	* \brief Write the settings stored in the class to the config file at location \ref SETTINGS_FILE_PATH 
	*/
	void writeSettings();
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
	static bool configHasChanged(ObjectFrameProcessor& imageProcessor);
	/*!  
	* \brief Loads the config from the file and effects the settings in the image processor. 
	* \brief Looks for config file at \ref SETTINGS_FILE_PATH
	* \param  imageProcessor - The image processor to set the config on
	*/
	
}
	/*!  
	* \brief Writes a config save file at \ref SETTINGS_FILE_PATH with the current settings from the image processor.
	* \param  imageProcessor - the image processor with the settings to save to the config file
	*/
static void triggerConfigSave(ObjectFrameProcessor& imageProcessor);
};