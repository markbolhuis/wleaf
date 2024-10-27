#pragma once

VkBool32
wlfGetPhysicalDevicePresentationSupport(
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR pfn,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    struct wlf_context *context);

VkResult
wlfCreateVulkanSurface(
    PFN_vkCreateWaylandSurfaceKHR pfn,
    VkInstance instance,
    struct wlf_surface *surface,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface);
