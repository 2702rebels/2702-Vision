#pragma once
#include <NetworkTopicGeneric.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <memory>
#include <networktables/Topic.h>
#include <string>

namespace pbuf = google::protobuf;

/**
 * @class ObjectDetectionConfigValue
 * @brief A configuration field which syncs the 3 config sources: program
 * memory, disk, and networktables
 * @details Also offers support for adding a callback to be executed when the
 * value is changed
 * @tparam T the type of the value to sync. Can be int32_t, int64_t, float,
 * double, bool, std::string or a vector of any of those
 */
template <typename T>
class ObjectDetectionConfigValue {
    template <typename>
    friend class ObjectDetectionConfig;

  private:
    /** @brief The protobuf field this instance corresponds to */
    const pbuf::FieldDescriptor* field;
    /** @brief The current value of the configuration setting */
    T lastValue;
    /** @brief A function to call when the value of this setting is changed */
    std::function<void(const T&)> setterFunction;
    /** @brief The @ref NetworkTopicGeneric responsible for syncing this value
     * with networktables */
    std::unique_ptr<NetworkTopicGeneric<T>> ntValueSyncer;
    /** @brief Retrieves a new config value from a protobuf config structure
     * @warning Assumes the field exists within the message
     * @returns The config value from the message
     */
    T GetConfigValueFromMessage(const pbuf::Message& topic);
    /** @brief Update this configuration value from a protobuf config structure
     * @warning Assumes the field exists within the message
     */
    void UpdateFromConfigMessage(const pbuf::Message& settings);
    /** @brief Update this configuration value from networktables if a new value
     * is available
     * @details If the value is updated, it will by synced to disk and @ref
     * setterFunction will be called
     */
    void UpdateFromNetworkTables();
    /** @brief Update this configuration value from an explicit value
     * @details setterFunction will be called with the new value, it will be
     * written to disk and pushed to networktables
     */
    void UpdateValue(const T& newValue);

  public:
    // Explicit move constructors
    ObjectDetectionConfigValue(ObjectDetectionConfigValue&&) noexcept = default;
    ObjectDetectionConfigValue&
    operator=(ObjectDetectionConfigValue&&) noexcept = default;
    // No copying allowed
    ObjectDetectionConfigValue(const ObjectDetectionConfigValue&) = delete;
    ObjectDetectionConfigValue&
    operator=(const ObjectDetectionConfigValue&) = delete;
    /** @brief Create a new @ref ObjectDetectionConfigValue
     * @param field The protobuf field corresponding to this config value
     * @param defaultStruct The protobuf message to extract the default value of
     * this configuration value from
     * @param setter An optional setter function to execute when the value of this
     * config value is updated
     * @param ntTopic The networktables topic to be associated with this
     * configuration value
     */
    ObjectDetectionConfigValue(const pbuf::FieldDescriptor* field,
                               pbuf::Message& defaultStruct,
                               std::function<void(const T&)> setter,
                               nt::Topic ntTopic);
    /** @brief Getter for @ref field */
    const pbuf::FieldDescriptor* GetFieldDescriptor() const;
    /** @brief Getter for @ref lastValue */
    T GetValue() const;
};