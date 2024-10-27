#include <wayland-util.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

#include "context_priv.h"
#include "toplevel_priv.h"

#include "wlf/vulkan.h"

VkBool32
wlfGetPhysicalDevicePresentationSupport(
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR pfn,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    struct wlf_context *context)
{
    return pfn(physicalDevice, queueFamilyIndex, context->wl_display);
}

VkResult
wlfCreateVulkanSurface(
    PFN_vkCreateWaylandSurfaceKHR pfn,
    VkInstance instance,
    struct wlf_surface *surface,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    VkWaylandSurfaceCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .display = surface->context->wl_display,
        .surface = surface->wl_surface,
    };

    return pfn(instance, &info, pAllocator, pSurface);
}
