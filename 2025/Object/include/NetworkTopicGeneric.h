#pragma once
#include <networktables/GenericEntry.h>
#include <networktables/Topic.h>

/**
 * @class NetworkTopicGeneric
 * @brief Abstraction for getting and pushing values to network tables topics
 * using a template class
 * @details Used for keeping @ref ObjectDetectionConfigValue in sync with
 * networktables
 * @tparam T the type of the value to sync. Can be int32_t, int64_t, float,
 * double, bool, std::string or a vector of any of those
 */
template <typename T> class NetworkTopicGeneric {
private:
  /** @brief The subscriber to read the config value from network tables */
  nt::GenericSubscriber m_subscriber;
  /** @brief The publisher to push the config value to networktables when it is
   * changed locally */
  nt::GenericPublisher m_publisher;
  T m_defaultValue;

public:
  /** @brief Begin the sync by pushing a default value and subscribing to the
   * topic
   * @param topic The topic to sync the value with
   * @defaultValue the value to initially publish to the topic
   */
  NetworkTopicGeneric(nt::Topic &topic, T defaultValue);
  /** @brief Get the current value from @ref m_subscriber */
  T GetValue() const;
  /** @brief Push the value to @ref m_publisher
   * @param value The value to push to the nt topic
   */
  void PushValue(const T &value);
  ~NetworkTopicGeneric() = default;
};