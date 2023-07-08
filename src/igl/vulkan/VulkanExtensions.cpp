/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <igl/vulkan/VulkanExtensions.h>

#include <algorithm>
#include <iterator>

namespace igl {
namespace vulkan {

VulkanExtensions::VulkanExtensions() {
  extensions_.resize(kNumberOfExtensionTypes);
  enabledExtensions_.resize(kNumberOfExtensionTypes);
}

void VulkanExtensions::enumerate() {
  // On Android, vkEnumerateInstanceExtensionProperties crashes when validation layers are enabled
  // (validation layers are only enabled for DEBUG builds)
  // https://issuetracker.google.com/issues/209835779?pli=1
#if !IGL_PLATFORM_ANDROID || !IGL_DEBUG
  uint32_t count;
  VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));

  std::vector<VkExtensionProperties> allExtensions(count);

  VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, allExtensions.data()));

  constexpr size_t vectorIndex = (size_t)ExtensionType::Instance;
  std::transform(allExtensions.cbegin(),
                 allExtensions.cend(),
                 std::back_inserter(extensions_[vectorIndex]),
                 [](const VkExtensionProperties& extensionProperties) {
                   return extensionProperties.extensionName;
                 });
#endif // !IGL_PLATFORM_ANDROID || !IGL_DEBUG
}

void VulkanExtensions::enumerate(VkPhysicalDevice device) {
  // On Android, vkEnumerateInstanceExtensionProperties crashes when validation layers are enabled
  // (validation layers are only enabled for DEBUG builds)
  // https://issuetracker.google.com/issues/209835779?pli=1
#if !IGL_PLATFORM_ANDROID || !IGL_DEBUG
  uint32_t count;
  VK_ASSERT(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));

  std::vector<VkExtensionProperties> allExtensions(count);

  VK_ASSERT(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, allExtensions.data()));

  constexpr size_t vectorIndex = (size_t)ExtensionType::Device;
  std::transform(allExtensions.cbegin(),
                 allExtensions.cend(),
                 std::back_inserter(extensions_[vectorIndex]),
                 [](const VkExtensionProperties& extensionProperties) {
                   return extensionProperties.extensionName;
                 });
#endif // !IGL_PLATFORM_ANDROID || !IGL_DEBUG
}

const std::vector<std::string>& VulkanExtensions::allAvailableExtensions(
    ExtensionType extensionType) const {
  const size_t vectorIndex = (size_t)extensionType;
  return extensions_[vectorIndex];
}

bool VulkanExtensions::available(const char* extensionName, ExtensionType extensionType) const {
  const size_t vectorIndex = (size_t)extensionType;
  const std::string extensionNameStr(extensionName);
  auto result = std::find_if(
      extensions_[vectorIndex].begin(),
      extensions_[vectorIndex].end(),
      [&extensionNameStr](const std::string& extension) { return extension == extensionNameStr; });

  return result != extensions_[vectorIndex].end();
}

bool VulkanExtensions::enable(const char* extensionName, ExtensionType extensionType) {
  const size_t vectorIndex = (size_t)extensionType;
  // Since VK on Android currently crashes with Validation layers,
  // enable the extension by default.
#if IGL_DEBUG && IGL_PLATFORM_ANDROID
  enabledExtensions_[vectorIndex].insert(extensionName);
  return true;
#else
  if (available(extensionName, extensionType)) {
    enabledExtensions_[vectorIndex].insert(extensionName);
    return true;
  }
  return false;
#endif
}

void VulkanExtensions::enableCommonExtensions(ExtensionType extensionType, bool validationEnabled) {
  if (extensionType == ExtensionType::Instance) {
    enable(VK_KHR_SURFACE_EXTENSION_NAME, ExtensionType::Instance);
    enable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, ExtensionType::Instance);
#if IGL_PLATFORM_WIN
    enable(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, ExtensionType::Instance);
    enable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ExtensionType::Instance);
#elif IGL_PLATFORM_ANDROID
    enable("VK_KHR_android_surface", ExtensionType::Instance);
    enable(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, ExtensionType::Instance);
#elif IGL_PLATFORM_LINUX
    // @fb-only
    // @fb-only
    // @fb-only
    enable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ExtensionType::Instance);
    enable("VK_KHR_xlib_surface", ExtensionType::Instance);
#endif

#if !IGL_PLATFORM_ANDROID
    if (validationEnabled) {
      enable(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, ExtensionType::Instance);
    }
#endif

  } else if (extensionType == ExtensionType::Device) {
    enable(VK_KHR_SWAPCHAIN_EXTENSION_NAME, ExtensionType::Device);
#if defined(IGL_WITH_TRACY) && defined(VK_EXT_calibrated_timestamps)
    enable(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, ExtensionType::Device);
#endif
    enable(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, ExtensionType::Device);
  } else {
    IGL_ASSERT_MSG(false, "Unrecognized extension type when enabling commong extensions.");
  }
}

bool VulkanExtensions::enabled(const char* extensionName) const {
  return (enabledExtensions_[(size_t)ExtensionType::Instance].count(extensionName) > 0) ||
         (enabledExtensions_[(size_t)ExtensionType::Device].count(extensionName) > 0);
}

const std::vector<const char*>& VulkanExtensions::allEnabled(ExtensionType extensionType) const {
  const size_t vectorIndex = (size_t)extensionType;
  returnList_.clear();
  for (const auto& extension : enabledExtensions_[vectorIndex]) {
    returnList_.emplace_back(extension.c_str());
  }
  return returnList_;
}

} // namespace vulkan
} // namespace igl
