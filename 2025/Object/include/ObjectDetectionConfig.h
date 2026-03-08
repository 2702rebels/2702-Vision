#pragma once
#include <ObjectDetectionConfigValue.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <networktables/Topic.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pbuf = google::protobuf;

/** @brief All the possible ObjectDetectionConfigValues */
using ObjectDetectionConfigValueGen = std::variant<
    ObjectDetectionConfigValue<int32_t>, ObjectDetectionConfigValue<double>,
    ObjectDetectionConfigValue<float>, ObjectDetectionConfigValue<bool>,
    ObjectDetectionConfigValue<std::string>,
    ObjectDetectionConfigValue<int64_t>,
    ObjectDetectionConfigValue<std::vector<int32_t>>,
    ObjectDetectionConfigValue<std::vector<double>>,
    ObjectDetectionConfigValue<std::vector<float>>,
    ObjectDetectionConfigValue<std::vector<bool>>,
    ObjectDetectionConfigValue<std::vector<std::string>>,
    ObjectDetectionConfigValue<std::vector<int64_t>>>;

/**
 * @class ObjectDetectionConfig
 * @brief A class to encapsulate a protobuf message representing a configuration
 * for the purposes of syncing its values in memory, on disk and in
 * networktables
 * @warning Your protobuf message can only contain certain allowed types (see
 * @ref ObjectDetectionConfigValue tparam docs)
 * @warning You must add template definitions for your config message to
 * ObjectDetectionConfig.cpp or you will get linker errors
 * @tparam T the type of the protobuf message configuration that this class is
 * encapsulating
 */
template <typename T>
class ObjectDetectionConfig {
    static_assert(std::is_base_of<pbuf::Message, T>::value,
                  "ObjectDetectionConfig can only be instantiated with protobuf "
                  "message types");

  private:
    /** @brief The reflection instance corresponding to @ref
     * currentConfigStructure */
    const pbuf::Reflection* reflection;
    /** @brief The path to where the configuration should be saved and loaded from
     * on disk */
    std::string m_configPath;
    /** @brief A list of all the fields in the configuration wrapped in @ref
     * ObjectDetectionConfigValue classes */
    std::vector<ObjectDetectionConfigValueGen> m_configValues;
    /** @brief A map of field names to their field numbers in the protobuf message
     * T */
    std::unordered_map<std::string, int> fieldNamesToIndex;
    /** @brief The current configuration in protobuf message format which is
     * synced across the 3 sources */
    T currentConfigStructure;

  public:
    /** @brief Create a new @ref ObjectDetectionConfig
     * @details Will load defaults from the file at configFilePath if it exists,
     * otherwise loads defaults from defaultConfigSettings param
     * @param configFilePath the path to where to load and save the configuration
     * file to
     * @param defaultConfigSettings The configuration settings to use if there is
     * no file available at the configFilePath
     * @param configName The name for the configuration which is used as a prefix
     * to the network table topic paths
     * @warning Assumes that the file at configFilePath is a valid configuration
     * file if it exists
     */
    ObjectDetectionConfig(std::string configFilePath, T defaultConfigSettings,
                          std::string configName);
    /** @brief Load the configuration written in a file at @ref m_configPath into
     * memory and sync it with networktables */
    void LoadConfigFromFile();
    /** @brief Write the current configuration in memory to a file at @ref
     * m_configPath */
    void WriteConfigToFile();
    /** @brief Load the configuration from networktables if it has changed and
     * sync to disk if anything has changed */
    void UpdateFromNetworkTables();
    /** @brief Recreate @ref currentConfigStructure from @ref m_configValues */
    void RefreshConfigStructure();
    /** @brief Load a new configuration from a protobuf message configuration and
     * sync it to disk and networktables */
    void LoadConfigFromStructure(const T& newConfigSettings);
    /** @brief Gets the current configuration from @ref currentConfigStructure */
    const T& Get();
    /** @brief Get the @ref ObjectDetectionConfigValue associated with a certain
     * field name
     * @warning Will throw if there is no field with the name provided
     */
    ObjectDetectionConfigValueGen& GetConfigValueStruct(std::string fieldName);
    /** @brief Get the @ref ObjectDetectionConfigValue associated with a field
     * index
     * @warning Will throw if there is no field at the index
     */
    ObjectDetectionConfigValueGen& GetConfigValueStruct(int fieldIndex);
    /** @brief Get the @ref ObjectDetectionConfigValue associated with a field
     * name casted to a certain config value type
     * @tparam O The raw value type of the config value, eg int32_t which the
     * field is of
     * @warning Will throw if the type O is inconsistent with the actual type of
     * the field
     * @warning Will throw if there is no field with the name specified
     */
    template <typename O>
    ObjectDetectionConfigValue<O>& GetConfigValueStruct(std::string fieldName);
    /** @brief Get the @ref ObjectDetectionConfigValue associated with a field
     * index casted to a certain config value type
     * @tparam O The raw value type of the config value, eg int32_t which the
     * field is of
     * @warning Will throw if the type O is inconsistent with the actual type of
     * the field
     * @warning Will throw if there is no field at the index specified
     */
    template <typename O>
    ObjectDetectionConfigValue<O>& GetConfigValueStruct(int fieldIndex);
    /** @brief Set the setter function of a @ref ObjectDetectionConfigValue from a
     * field name and setter function
     * @tparam O The raw value type of the config value, eg int32_t which the
     * field is of
     * @warning Will throw if the type O is inconsistent with the actual type of
     * the field
     * @warning Will overwrite any previous setter function associated with the
     * value
     * @warning Assumes that there actually is a field with the specified name
     */
    template <typename O>
    void AttachSetterFunction(std::string fieldName,
                              std::function<void(const O&)> setterFunction);
    /** @brief Set the setter function of a @ref ObjectDetectionConfigValue from a
     * field index and setter function
     * @tparam O The raw value type of the config value, eg int32_t which the
     * field is of
     * @warning Will throw if the type O is inconsistent with the actual type of
     * the field
     * @warning Will overwrite any previous setter function associated with the
     * value
     * @warning Assumes that there is actually a field at the specified index
     */
    template <typename O>
    void AttachSetterFunction(int valueIndex,
                              std::function<void(const O&)> setterFunction);
};