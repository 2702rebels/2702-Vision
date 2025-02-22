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
#define SETTINGS_FILE_PATH "./processorSettings.txt"
#include <nlohmann/json.hpp>
 
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
	}

	/*!  
	* \brief Initialize an instance of the class from the settings
	* \param  hueMin - integer representing the hueMin parameter
	* \param  hueMax - integer representing the hueMax parameter
	* \param  saturationMin - integer representing the saturationMin parameter
	* \param  saturationMax - integer representing the satuartionMax parameter
	* \param  valueMin - integer representing the valueMin parameter
	* \param  valueMax - integer representing the valueMax parameter
	*/
	imageProcessingFilterSettings(int hueMin, int hueMax, int saturationMin, int saturationMax, int valueMin, int valueMax)
	{
		this->hueMin = hueMin;
		this->hueMax = hueMax;
		this->saturationMin = saturationMin;
		this->saturationMax = saturationMax;
		this->valueMin = valueMin;
		this->valueMax = valueMax;
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
			m_npreviousHueMin = config.hueMin;
			m_npreviousHueMax = config.hueMax;
			m_npreviousSaturationMin = config.saturationMin;
			m_npreviousSaturationMax = config.saturationMax;
			m_npreviousValueMin = config.valueMin;
			m_npreviousValueMax = config.valueMax;
		}
	}
	/*!  
	* \brief Writes a config save file at \ref SETTINGS_FILE_PATH with the current settings from the image processor.
	* \param  imageProcessor - the image processor with the settings to save to the config file
	*/
	static void triggerConfigSave(imgProc &imageProcessor)
	{
		imageProcessingFilterSettings config = imageProcessingFilterSettings(imageProcessor.getPVHueMin(), imageProcessor.getPVHueMax(),
																			 imageProcessor.getSaturationMin(), imageProcessor.getSaturationMax(), imageProcessor.getValueMin(), imageProcessor.getValueMax());
		config.writeSettings();
		m_npreviousHueMin = config.hueMin;
		m_npreviousHueMax = config.hueMax;
		m_npreviousSaturationMin = config.saturationMin;
		m_npreviousSaturationMax = config.saturationMax;
		m_npreviousValueMin = config.valueMin;
		m_npreviousValueMax = config.valueMax;
	}
};
// Instantiate the static values
int ConfigUtils::m_npreviousHueMin;
int ConfigUtils::m_npreviousHueMax;
int ConfigUtils::m_npreviousSaturationMin;
int ConfigUtils::m_npreviousSaturationMax;
int ConfigUtils::m_npreviousValueMin;
int ConfigUtils::m_npreviousValueMax;