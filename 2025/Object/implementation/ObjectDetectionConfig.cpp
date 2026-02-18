#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <TestConfigurationSettings.pb.h>
#include <filesystem>
#include <fstream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <networktables/NetworkTableInstance.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <type_traits>

template class ObjectDetectionConfig<ObjectDetectionSettings>;
template class ObjectDetectionConfig<TestConfigurationSettings>;

// gcc magic trick, see else branches
template <typename> inline constexpr bool always_false_v = false;

template <typename T>
ObjectDetectionConfig<T>::ObjectDetectionConfig(std::string configFilePath,
                                                T defaultConfigSettings,
                                                std::string configName)
    : m_configPath(configFilePath),
      reflection(currentConfigStructure.GetReflection()) {
  const pbuf::Descriptor *messageClass = defaultConfigSettings.GetDescriptor();
  m_configValues.reserve(messageClass->field_count());
  for (int i = 0; i < messageClass->field_count(); i++) {
    const pbuf::FieldDescriptor *field = messageClass->field(i);
    fieldNamesToIndex[field->name()] = i;
    nt::Topic ntTopic = nt::NetworkTableInstance::GetDefault().GetTopic(
        "ObjectDetectionConfig/" + configName + "/" + field->name());
    if (!field->is_repeated()) {
      switch (field->cpp_type()) {
      case pbuf::FieldDescriptor::CPPTYPE_INT32: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<int32_t>>, field,
            defaultConfigSettings, std::function<void(const int32_t &)>{},
            ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<double>>, field,
            defaultConfigSettings, std::function<void(const double &)>{},
            ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<bool>>, field,
            defaultConfigSettings, std::function<void(const bool &)>{},
            ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<float>>, field,
            defaultConfigSettings, std::function<void(const float &)>{},
            ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_STRING: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<std::string>>, field,
            defaultConfigSettings, std::function<void(const std::string &)>{},
            ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_INT64: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<int64_t>>, field,
            defaultConfigSettings, std::function<void(const int64_t &)>{},
            ntTopic);
        break;
      }
      default: {
        spdlog::error(
            "Unsupported field type of field #{} {} in ObjectDetectionConfig",
            field->index(), field->name());
        throw std::invalid_argument("Unsupported field type");
      }
      }
    } else {
      switch (field->cpp_type()) {
      case pbuf::FieldDescriptor::CPPTYPE_INT32: {
        m_configValues.emplace_back(
            std::in_place_type<
                ObjectDetectionConfigValue<std::vector<int32_t>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<int32_t> &)>{}, ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<std::vector<double>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<double> &)>{}, ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<std::vector<bool>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<bool> &)>{}, ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
        m_configValues.emplace_back(
            std::in_place_type<ObjectDetectionConfigValue<std::vector<float>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<float> &)>{}, ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_STRING: {
        m_configValues.emplace_back(
            std::in_place_type<
                ObjectDetectionConfigValue<std::vector<std::string>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<std::string> &)>{}, ntTopic);
        break;
      }
      case pbuf::FieldDescriptor::CPPTYPE_INT64: {
        m_configValues.emplace_back(
            std::in_place_type<
                ObjectDetectionConfigValue<std::vector<int64_t>>>,
            field, defaultConfigSettings,
            std::function<void(const std::vector<int64_t> &)>{}, ntTopic);
        break;
      }
      default: {
        spdlog::error(
            "Unsupported field type of field #{} {} in ObjectDetectionConfig",
            field->index(), field->name());
        throw std::invalid_argument("Unsupported field type");
      }
      }
    }
  }
  if (std::filesystem::exists(configFilePath)) {
    LoadConfigFromFile();
  } else {
    RefreshConfigStructure();
  }
}

template <typename T>
void ObjectDetectionConfig<T>::LoadConfigFromStructure(
    const T &newConfigSettings) {
  currentConfigStructure.CopyFrom(newConfigSettings);
  for (auto &configValue : m_configValues) {
    std::visit(
        [&](auto &value) {
          value.UpdateFromConfigMessage(currentConfigStructure);
        },
        configValue);
  }
  WriteConfigToFile();
}

template <typename T> void ObjectDetectionConfig<T>::LoadConfigFromFile() {
  std::ifstream configFile(m_configPath);
  if (!configFile.is_open()) {
    spdlog::error("Failed to open config file at path: {}", m_configPath);
    throw std::runtime_error("Failed to open config file");
  }
  std::ostringstream buffer;
  buffer << configFile.rdbuf();
  std::string configContents = buffer.str();
  auto status =
      pbuf::util::JsonStringToMessage(configContents, &currentConfigStructure);
  if (!status.ok()) {
    spdlog::error("Failed to parse config file at path: {}\nParse error: {}",
                  m_configPath, status.message());
    throw std::runtime_error("Failed to parse config file");
  }
  for (auto &configValue : m_configValues) {
    // Runs value.UpdateFromConfigMessage if value is any one of the variant
    // types
    std::visit(
        [&](auto &value) {
          value.UpdateFromConfigMessage(currentConfigStructure);
        },
        configValue);
  }
}

template <typename T> void ObjectDetectionConfig<T>::WriteConfigToFile() {
  std::ofstream configFile(m_configPath, std::ios::out | std::ios::trunc);
  if (!configFile.is_open()) {
    spdlog::error("Failed to open config file for writing at path: {}",
                  m_configPath);
    throw std::runtime_error("Failed to open config file for writing");
  }
  std::string configJson;
  auto status =
      pbuf::util::MessageToJsonString(currentConfigStructure, &configJson);
  if (!status.ok()) {
    spdlog::error("Failed to serialize config to JSON: {}", status.message());
    throw std::runtime_error("Failed to serialize config to JSON");
  }
  configFile << configJson;
}

template <typename T> const T &ObjectDetectionConfig<T>::Get() {
  return currentConfigStructure;
}

template <typename T> void ObjectDetectionConfig<T>::RefreshConfigStructure() {
  for (auto &configValue : m_configValues) {
    std::visit(
        [&](auto &value) {
          const pbuf::FieldDescriptor *field = value.GetFieldDescriptor();
          using ValueType = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<ValueType,
                                       ObjectDetectionConfigValue<int32_t>>) {
            reflection->SetInt32(&currentConfigStructure, field,
                                 value.GetValue());
          } else if constexpr (std::is_same_v<
                                   ValueType,
                                   ObjectDetectionConfigValue<double>>) {
            reflection->SetDouble(&currentConfigStructure, field,
                                  value.GetValue());
          } else if constexpr (std::is_same_v<
                                   ValueType,
                                   ObjectDetectionConfigValue<float>>) {
            reflection->SetFloat(&currentConfigStructure, field,
                                 value.GetValue());
          } else if constexpr (std::is_same_v<
                                   ValueType,
                                   ObjectDetectionConfigValue<bool>>) {
            reflection->SetBool(&currentConfigStructure, field,
                                value.GetValue());
          } else if constexpr (std::is_same_v<
                                   ValueType,
                                   ObjectDetectionConfigValue<std::string>>) {
            reflection->SetString(&currentConfigStructure, field,
                                  value.GetValue());
          } else if constexpr (std::is_same_v<
                                   ValueType,
                                   ObjectDetectionConfigValue<int64_t>>) {
            reflection->SetInt64(&currentConfigStructure, field,
                                 value.GetValue());
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<int32_t>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (int32_t val : value.GetValue()) {
              reflection->AddInt32(&currentConfigStructure, field, val);
            }
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<double>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (double val : value.GetValue()) {
              reflection->AddDouble(&currentConfigStructure, field, val);
            }
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<float>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (float val : value.GetValue()) {
              reflection->AddFloat(&currentConfigStructure, field, val);
            }
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<bool>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (bool val : value.GetValue()) {
              reflection->AddBool(&currentConfigStructure, field, val);
            }
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<std::string>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (std::string val : value.GetValue()) {
              reflection->AddString(&currentConfigStructure, field, val);
            }
          } else if constexpr (std::is_same_v<ValueType,
                                              ObjectDetectionConfigValue<
                                                  std::vector<int64_t>>>) {
            reflection->ClearField(&currentConfigStructure, field);
            for (int64_t val : value.GetValue()) {
              reflection->AddInt64(&currentConfigStructure, field, val);
            }
          } else {
            static_assert(always_false_v<T>,
                          "Unsupported ValueType provided to "
                          "ObjectDetectionConfig::RefreshConfigStructure");
          }
        },
        configValue);
  }
  WriteConfigToFile();
}

template <typename T> void ObjectDetectionConfig<T>::UpdateFromNetworkTables() {
  for (ObjectDetectionConfigValueGen &configValue : m_configValues) {
    std::visit([&](auto &value) { value.UpdateFromNetworkTables(); },
               configValue);
  }
  RefreshConfigStructure();
}

template <typename T>
ObjectDetectionConfigValueGen &
ObjectDetectionConfig<T>::GetConfigValueStruct(std::string fieldName) {
  auto it = fieldNamesToIndex.find(fieldName);
  if (it == fieldNamesToIndex.end()) {
    spdlog::error("Failed to find field of name {} in GetConfigValueStruct",
                  fieldName);
    throw std::invalid_argument("Failed to find field of name");
  }
  return m_configValues[fieldNamesToIndex[fieldName]];
}

template <typename T>
ObjectDetectionConfigValueGen &
ObjectDetectionConfig<T>::GetConfigValueStruct(int fieldIndex) {
  int arrIndex = fieldIndex - 1;
  if (arrIndex < 0 || arrIndex >= m_configValues.size()) {
    spdlog::error("Invalid fieldIndex provided to GetConfigValueStruct: {}",
                  fieldIndex);
    throw std::invalid_argument("Invalid fieldIndex");
  }
  return m_configValues[arrIndex];
}

template <typename T>
template <typename O>
ObjectDetectionConfigValue<O> &
ObjectDetectionConfig<T>::GetConfigValueStruct(std::string fieldName) {
  return std::get<ObjectDetectionConfigValue<O>>(
      GetConfigValueStruct(fieldName));
}

template <typename T>
template <typename O>
ObjectDetectionConfigValue<O> &
ObjectDetectionConfig<T>::GetConfigValueStruct(int fieldIndex) {
  return std::get<ObjectDetectionConfigValue<O>>(
      GetConfigValueStruct(fieldIndex));
}

template <typename T>
template <typename O>
void ObjectDetectionConfig<T>::AttachSetterFunction(
    std::string fieldName, std::function<void(const O &)> setterFunction) {
  auto it = fieldNamesToIndex.find(fieldName);
  if (it == fieldNamesToIndex.end()) {
    spdlog::error("Failed to find field with name {} in AttachSetterFunction",
                  fieldName);
    throw std::invalid_argument("Failed to find field with name");
  }
  std::get<ObjectDetectionConfigValue<O>>(m_configValues[it->second])
      .setterFunction = std::move(setterFunction);
}

template <typename T>
template <typename O>
void ObjectDetectionConfig<T>::AttachSetterFunction(
    int fieldIndex, std::function<void(const O &)> setterFunction) {
  int arrIndex = fieldIndex - 1;
  if (arrIndex < 0 || arrIndex >= m_configValues.size()) {
    spdlog::error("Invalid field index in AttachSetterFunction: {}",
                  fieldIndex);
    throw std::invalid_argument("Invalid fieldIndex");
  }
  std::get<ObjectDetectionConfigValue<O>>(m_configValues[arrIndex])
      .setterFunction = std::move(setterFunction);
}

/** Wall of template instantiations */

template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<int32_t>(
    int, std::function<void(const int32_t &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<double>(
    int, std::function<void(const double &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<float>(
    int, std::function<void(const float &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<bool>(
    int, std::function<void(const bool &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::string>(int, std::function<void(const std::string &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<int64_t>(
    int, std::function<void(const int64_t &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<int32_t>>(int,
                          std::function<void(const std::vector<int32_t> &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<double>>(int, std::function<void(const std::vector<double> &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<float>>(int, std::function<void(const std::vector<float> &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<bool>>(int, std::function<void(const std::vector<bool> &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<std::string>>(
    int, std::function<void(const std::vector<std::string> &)>);
template void
ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
    std::vector<int64_t>>(int,
                          std::function<void(const std::vector<int64_t> &)>);

template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        int32_t>(std::string, std::function<void(const int32_t &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        double>(std::string, std::function<void(const double &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<float>(
        std::string, std::function<void(const float &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<bool>(
        std::string, std::function<void(const bool &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::string>(std::string, std::function<void(const std::string &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        int64_t>(std::string, std::function<void(const int64_t &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<int32_t>>(
        std::string, std::function<void(const std::vector<int32_t> &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<double>>(std::string,
                             std::function<void(const std::vector<double> &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<float>>(std::string,
                            std::function<void(const std::vector<float> &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<bool>>(std::string,
                           std::function<void(const std::vector<bool> &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<std::string>>(
        std::string, std::function<void(const std::vector<std::string> &)>);
template void
    ObjectDetectionConfig<ObjectDetectionSettings>::AttachSetterFunction<
        std::vector<int64_t>>(
        std::string, std::function<void(const std::vector<int64_t> &)>);

template ObjectDetectionConfigValue<int32_t> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<int32_t>(std::string);
template ObjectDetectionConfigValue<double> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<double>(std::string);
template ObjectDetectionConfigValue<float> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<float>(
        std::string);
template ObjectDetectionConfigValue<bool> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<bool>(
        std::string);
template ObjectDetectionConfigValue<std::string> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<std::string>(std::string);
template ObjectDetectionConfigValue<int64_t> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<int64_t>(std::string);
template ObjectDetectionConfigValue<std::vector<int32_t>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<int32_t>>(std::string);
template ObjectDetectionConfigValue<std::vector<double>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<double>>(std::string);
template ObjectDetectionConfigValue<std::vector<float>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<float>>(std::string);
template ObjectDetectionConfigValue<std::vector<bool>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<bool>>(std::string);
template ObjectDetectionConfigValue<std::vector<std::string>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<std::string>>(std::string);
template ObjectDetectionConfigValue<std::vector<int64_t>> &
    ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
        std::vector<int64_t>>(std::string);

template ObjectDetectionConfigValue<int32_t> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<int32_t>(
    int);
template ObjectDetectionConfigValue<double> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<double>(
    int);
template ObjectDetectionConfigValue<float> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<float>(
    int);
template ObjectDetectionConfigValue<bool> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<bool>(int);
template ObjectDetectionConfigValue<std::string> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<std::string>(int);
template ObjectDetectionConfigValue<int64_t> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<int64_t>(
    int);
template ObjectDetectionConfigValue<std::vector<int32_t>> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
    std::vector<int32_t>>(int);
template ObjectDetectionConfigValue<std::vector<double>> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<std::vector<double>>(int);
template ObjectDetectionConfigValue<std::vector<float>> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<std::vector<float>>(int);
template ObjectDetectionConfigValue<std::vector<bool>> &ObjectDetectionConfig<
    ObjectDetectionSettings>::GetConfigValueStruct<std::vector<bool>>(int);
template ObjectDetectionConfigValue<std::vector<std::string>> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
    std::vector<std::string>>(int);
template ObjectDetectionConfigValue<std::vector<int64_t>> &
ObjectDetectionConfig<ObjectDetectionSettings>::GetConfigValueStruct<
    std::vector<int64_t>>(int);

template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<int32_t>(
    int, std::function<void(const int32_t &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<double>(
    int, std::function<void(const double &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<float>(
    int, std::function<void(const float &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<bool>(
    int, std::function<void(const bool &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::string>(int, std::function<void(const std::string &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<int64_t>(
    int, std::function<void(const int64_t &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<int32_t>>(int,
                          std::function<void(const std::vector<int32_t> &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<double>>(int, std::function<void(const std::vector<double> &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<float>>(int, std::function<void(const std::vector<float> &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<bool>>(int, std::function<void(const std::vector<bool> &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<std::string>>(
    int, std::function<void(const std::vector<std::string> &)>);
template void
ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
    std::vector<int64_t>>(int,
                          std::function<void(const std::vector<int64_t> &)>);

template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        int32_t>(std::string, std::function<void(const int32_t &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        double>(std::string, std::function<void(const double &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        float>(std::string, std::function<void(const float &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        bool>(std::string, std::function<void(const bool &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::string>(std::string, std::function<void(const std::string &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        int64_t>(std::string, std::function<void(const int64_t &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<int32_t>>(
        std::string, std::function<void(const std::vector<int32_t> &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<double>>(std::string,
                             std::function<void(const std::vector<double> &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<float>>(std::string,
                            std::function<void(const std::vector<float> &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<bool>>(std::string,
                           std::function<void(const std::vector<bool> &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<std::string>>(
        std::string, std::function<void(const std::vector<std::string> &)>);
template void
    ObjectDetectionConfig<TestConfigurationSettings>::AttachSetterFunction<
        std::vector<int64_t>>(
        std::string, std::function<void(const std::vector<int64_t> &)>);

template ObjectDetectionConfigValue<int32_t> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<int32_t>(std::string);
template ObjectDetectionConfigValue<double> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<double>(std::string);
template ObjectDetectionConfigValue<float> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<float>(std::string);
template ObjectDetectionConfigValue<bool> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<bool>(std::string);
template ObjectDetectionConfigValue<std::string> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<std::string>(std::string);
template ObjectDetectionConfigValue<int64_t> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<int64_t>(std::string);
template ObjectDetectionConfigValue<std::vector<int32_t>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<int32_t>>(std::string);
template ObjectDetectionConfigValue<std::vector<double>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<double>>(std::string);
template ObjectDetectionConfigValue<std::vector<float>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<float>>(std::string);
template ObjectDetectionConfigValue<std::vector<bool>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<bool>>(std::string);
template ObjectDetectionConfigValue<std::vector<std::string>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<std::string>>(std::string);
template ObjectDetectionConfigValue<std::vector<int64_t>> &
    ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
        std::vector<int64_t>>(std::string);

template ObjectDetectionConfigValue<int32_t> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<int32_t>(
    int);
template ObjectDetectionConfigValue<double> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<double>(
    int);
template ObjectDetectionConfigValue<float> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<float>(
    int);
template ObjectDetectionConfigValue<bool> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<bool>(
    int);
template ObjectDetectionConfigValue<std::string> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<std::string>(int);
template ObjectDetectionConfigValue<int64_t> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<int64_t>(
    int);
template ObjectDetectionConfigValue<std::vector<int32_t>> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
    std::vector<int32_t>>(int);
template ObjectDetectionConfigValue<std::vector<double>> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<std::vector<double>>(int);
template ObjectDetectionConfigValue<std::vector<float>> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<std::vector<float>>(int);
template ObjectDetectionConfigValue<std::vector<bool>> &ObjectDetectionConfig<
    TestConfigurationSettings>::GetConfigValueStruct<std::vector<bool>>(int);
template ObjectDetectionConfigValue<std::vector<std::string>> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
    std::vector<std::string>>(int);
template ObjectDetectionConfigValue<std::vector<int64_t>> &
ObjectDetectionConfig<TestConfigurationSettings>::GetConfigValueStruct<
    std::vector<int64_t>>(int);