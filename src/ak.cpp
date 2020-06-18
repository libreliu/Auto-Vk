#pragma once

#define NOMINMAX
#include <ak/ak_log.hpp>
#include <ak/ak.hpp>

namespace ak
{
#pragma region root definitions
	uint32_t root::find_memory_type_index(uint32_t aMemoryTypeBits, vk::MemoryPropertyFlags aMemoryProperties)
	{
		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. 
		// Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for 
		// when VRAM runs out. The different types of memory exist within these heaps. Right now we'll 
		// only concern ourselves with the type of memory and not the heap it comes from, but you can 
		// imagine that this can affect performance. (Source: https://vulkan-tutorial.com/)
		auto memProperties = physical_device().getMemoryProperties();
		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i) {
			if ((aMemoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & aMemoryProperties) == aMemoryProperties) {
				return i;
			}
		}
		throw ak::runtime_error("failed to find suitable memory type!");
	}

	bool root::is_format_supported(vk::Format pFormat, vk::ImageTiling pTiling, vk::FormatFeatureFlags aFormatFeatures)
	{
		auto formatProps = physical_device().getFormatProperties(pFormat);
		if (pTiling == vk::ImageTiling::eLinear 
			&& (formatProps.linearTilingFeatures & aFormatFeatures) == aFormatFeatures) {
			return true;
		}
		else if (pTiling == vk::ImageTiling::eOptimal 
				 && (formatProps.optimalTilingFeatures & aFormatFeatures) == aFormatFeatures) {
			return true;
		} 
		return false;
	}

	vk::PhysicalDeviceRayTracingPropertiesKHR root::get_ray_tracing_properties()
	{
		vk::PhysicalDeviceRayTracingPropertiesKHR rtProps;
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		physical_device().getProperties2(&props2);
		return rtProps;
	}

	vk::DeviceAddress root::get_buffer_address(vk::Buffer aBufferHandle)
	{
		PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device(), "vkGetBufferDeviceAddressKHR"));

	    VkBufferDeviceAddressInfo bufferAddressInfo;
		bufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferAddressInfo.pNext = nullptr;
		bufferAddressInfo.buffer = aBufferHandle;

	    return vkGetBufferDeviceAddress(device(), &bufferAddressInfo);
	}

	void root::finish_configuration(buffer_view_t& aBufferViewToBeFinished, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		aBufferViewToBeFinished.mInfo = vk::BufferViewCreateInfo{}
			.setBuffer(aBufferViewToBeFinished.buffer_handle())
			.setFormat(aViewFormat)
			.setOffset(0) // TODO: Support offsets
			.setRange(VK_WHOLE_SIZE); // TODO: Support ranges

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aBufferViewToBeFinished);
		}

		aBufferViewToBeFinished.mBufferView = device().createBufferViewUnique(aBufferViewToBeFinished.mInfo);

		// TODO: Descriptors?!
	}
#pragma endregion

#pragma region vk_utils
	bool is_srgb_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> srgbFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32
		};
		auto it = std::find(std::begin(srgbFormats), std::end(srgbFormats), pImageFormat);
		return it != srgbFormats.end();
	}

	bool is_uint8_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		// TODO: sRGB-formats are assumed to be uint8-formats (not signed int8-formats) => is that true?
		static std::set<vk::Format> uint8Formats = {
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SrgbPack32
		};
		auto it = std::find(std::begin(uint8Formats), std::end(uint8Formats), pImageFormat);
		return it != uint8Formats.end();
	}

	bool is_int8_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int8Formats = {
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Sint,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Sint,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8SintPack32,
		};
		auto it = std::find(std::begin(int8Formats), std::end(int8Formats), pImageFormat);
		return it != int8Formats.end();
	}

	bool is_uint16_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint16Formats = {
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Uint
		};
		auto it = std::find(std::begin(uint16Formats), std::end(uint16Formats), pImageFormat);
		return it != uint16Formats.end();
	}

	bool is_int16_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int16Formats = {
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Sint
		};
		auto it = std::find(std::begin(int16Formats), std::end(int16Formats), pImageFormat);
		return it != int16Formats.end();
	}

	bool is_uint32_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> uint32Format = { 
			vk::Format::eR32Uint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32A32Uint
		};
		auto it = std::find(std::begin(uint32Format), std::end(uint32Format), pImageFormat);
		return it != uint32Format.end();
	}

	bool is_int32_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> int32Format = {
			vk::Format::eR32Sint,
			vk::Format::eR32G32Sint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32A32Sint
		};
		auto it = std::find(std::begin(int32Format), std::end(int32Format), pImageFormat);
		return it != int32Format.end();
	}

	bool is_float_format(const vk::Format& pImageFormat)
	{
		return is_float16_format(pImageFormat) || is_float32_format(pImageFormat) || is_float64_format(pImageFormat);
	}

	bool is_float16_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float16Formats = {
			vk::Format::eR16Sfloat,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR16G16B16A16Sfloat
		};
		auto it = std::find(std::begin(float16Formats), std::end(float16Formats), pImageFormat);
		return it != float16Formats.end();
	}

	bool is_float32_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float32Formats = {
			vk::Format::eR32Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR32G32B32A32Sfloat
		};
		auto it = std::find(std::begin(float32Formats), std::end(float32Formats), pImageFormat);
		return it != float32Formats.end();
	}

	bool is_float64_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> float64Formats = {
			vk::Format::eR64Sfloat,
			vk::Format::eR64G64Sfloat,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eR64G64B64A64Sfloat
		};
		auto it = std::find(std::begin(float64Formats), std::end(float64Formats), pImageFormat);
		return it != float64Formats.end();
	}

	bool is_rgb_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbFormats = {
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,

		};
		auto it = std::find(std::begin(rgbFormats), std::end(rgbFormats), pImageFormat);
		return it != rgbFormats.end();
	}

	bool is_rgba_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> rgbaFormats = {
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
		};
		auto it = std::find(std::begin(rgbaFormats), std::end(rgbaFormats), pImageFormat);
		return it != rgbaFormats.end();
	}

	bool is_argb_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> argbFormats = {
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
		};
		auto it = std::find(std::begin(argbFormats), std::end(argbFormats), pImageFormat);
		return it != argbFormats.end();
	}

	bool is_bgr_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgrFormats = {
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
		};
		auto it = std::find(std::begin(bgrFormats), std::end(bgrFormats), pImageFormat);
		return it != bgrFormats.end();
	}

	bool is_bgra_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> bgraFormats = {
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
		};
		auto it = std::find(std::begin(bgraFormats), std::end(bgraFormats), pImageFormat);
		return it != bgraFormats.end();
	}

	bool is_abgr_format(const vk::Format& pImageFormat)
	{
		// Note: Currently, the compressed sRGB-formats are ignored => could/should be added in the future, maybe
		static std::set<vk::Format> abgrFormats = {
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
		};
		auto it = std::find(std::begin(abgrFormats), std::end(abgrFormats), pImageFormat);
		return it != abgrFormats.end();
	}

	bool has_stencil_component(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> stencilFormats = {
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
		};
		auto it = std::find(std::begin(stencilFormats), std::end(stencilFormats), pImageFormat);
		return it != stencilFormats.end();
	}

	bool is_depth_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> depthFormats = {
			vk::Format::eD16Unorm,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
		};
		auto it = std::find(std::begin(depthFormats), std::end(depthFormats), pImageFormat);
		return it != depthFormats.end();
	}

	bool is_1component_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> singleCompFormats = {
			vk::Format::eR8Srgb,
			vk::Format::eR8Unorm,
			vk::Format::eR8Uscaled,
			vk::Format::eR8Uint,
			vk::Format::eR8Srgb,
			vk::Format::eR8Snorm,
			vk::Format::eR8Sscaled,
			vk::Format::eR8Sint,
			vk::Format::eR16Unorm,
			vk::Format::eR16Uscaled,
			vk::Format::eR16Uint,
			vk::Format::eR16Snorm,
			vk::Format::eR16Sscaled,
			vk::Format::eR16Sint,
			vk::Format::eR32Uint,
			vk::Format::eR32Sint,
			vk::Format::eR16Sfloat,
			vk::Format::eR32Sfloat,
			vk::Format::eR64Sfloat,
		};
		auto it = std::find(std::begin(singleCompFormats), std::end(singleCompFormats), pImageFormat);
		return it != singleCompFormats.end();
	}

	bool is_2component_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> twoComponentFormats = {
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8Uscaled,
			vk::Format::eR8G8Uint,
			vk::Format::eR8G8Srgb,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8Sscaled,
			vk::Format::eR8G8Sint,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16Uscaled,
			vk::Format::eR16G16Uint,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16Sscaled,
			vk::Format::eR16G16Sint,
			vk::Format::eR32G32Uint,
			vk::Format::eR32G32Sint,
			vk::Format::eR16G16Sfloat,
			vk::Format::eR32G32Sfloat,
			vk::Format::eR64G64Sfloat,
		};
		auto it = std::find(std::begin(twoComponentFormats), std::end(twoComponentFormats), pImageFormat);
		return it != twoComponentFormats.end();
	}

	bool is_3component_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> threeCompFormat = {
			vk::Format::eR8G8B8Srgb,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eR5G6B5UnormPack16,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eR8G8B8Uscaled,
			vk::Format::eR8G8B8Sscaled,
			vk::Format::eR8G8B8Uint,
			vk::Format::eR8G8B8Sint,
			vk::Format::eR8G8B8Srgb,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16Uscaled,
			vk::Format::eR16G16B16Sscaled,
			vk::Format::eR16G16B16Uint,
			vk::Format::eR16G16B16Sint,
			vk::Format::eR16G16B16Sfloat,
			vk::Format::eR32G32B32Uint,
			vk::Format::eR32G32B32Sint,
			vk::Format::eR32G32B32Sfloat,
			vk::Format::eR64G64B64Uint,
			vk::Format::eR64G64B64Sint,
			vk::Format::eR64G64B64Sfloat,
			vk::Format::eB5G6R5UnormPack16,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eB8G8R8Uscaled,
			vk::Format::eB8G8R8Sscaled,
			vk::Format::eB8G8R8Uint,
			vk::Format::eB8G8R8Sint,
			vk::Format::eB8G8R8Srgb,
			vk::Format::eB10G11R11UfloatPack32,
		};
		auto it = std::find(std::begin(threeCompFormat), std::end(threeCompFormat), pImageFormat);
		return it != threeCompFormat.end();
	}

	bool is_4component_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> fourCompFormats = {
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eR4G4B4A4UnormPack16,
			vk::Format::eR5G5B5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR16G16B16A16Unorm,
			vk::Format::eR16G16B16A16Snorm,
			vk::Format::eR16G16B16A16Uscaled,
			vk::Format::eR16G16B16A16Sscaled,
			vk::Format::eR16G16B16A16Uint,
			vk::Format::eR16G16B16A16Sint,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Format::eR32G32B32A32Uint,
			vk::Format::eR32G32B32A32Sint,
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR64G64B64A64Uint,
			vk::Format::eR64G64B64A64Sint,
			vk::Format::eR64G64B64A64Sfloat,
			vk::Format::eA1R5G5B5UnormPack16,
			vk::Format::eA2R10G10B10UnormPack32,
			vk::Format::eA2R10G10B10SnormPack32,
			vk::Format::eA2R10G10B10UscaledPack32,
			vk::Format::eA2R10G10B10SscaledPack32,
			vk::Format::eA2R10G10B10UintPack32,
			vk::Format::eA2R10G10B10SintPack32,
			vk::Format::eB4G4R4A4UnormPack16,
			vk::Format::eB5G5R5A1UnormPack16,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32,
			vk::Format::eA2B10G10R10UnormPack32,
			vk::Format::eA2B10G10R10SnormPack32,
			vk::Format::eA2B10G10R10UscaledPack32,
			vk::Format::eA2B10G10R10SscaledPack32,
			vk::Format::eA2B10G10R10UintPack32,
			vk::Format::eA2B10G10R10SintPack32,
		};
		auto it = std::find(std::begin(fourCompFormats), std::end(fourCompFormats), pImageFormat);
		return it != fourCompFormats.end();
	}

	bool is_unorm_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> unormFormats = {
			vk::Format::eR8Unorm,
			vk::Format::eR8G8Unorm,
			vk::Format::eR8G8B8Unorm,
			vk::Format::eB8G8R8Unorm,
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eR16Unorm,
			vk::Format::eR16G16Unorm,
			vk::Format::eR16G16B16Unorm,
			vk::Format::eR16G16B16A16Unorm
		};
		auto it = std::find(std::begin(unormFormats), std::end(unormFormats), pImageFormat);
		return it != unormFormats.end();
	}
	
	bool is_snorm_format(const vk::Format& pImageFormat)
	{
		static std::set<vk::Format> snormFormats = {
			vk::Format::eR8Snorm,
			vk::Format::eR8G8Snorm,
			vk::Format::eR8G8B8Snorm,
			vk::Format::eB8G8R8Snorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eR16Snorm,
			vk::Format::eR16G16Snorm,
			vk::Format::eR16G16B16Snorm,
			vk::Format::eR16G16B16A16Snorm
		};
		auto it = std::find(std::begin(snormFormats), std::end(snormFormats), pImageFormat);
		return it != snormFormats.end();
	}
	
	bool is_norm_format(const vk::Format& pImageFormat)
	{
		return is_unorm_format(pImageFormat) || is_snorm_format(pImageFormat) || is_srgb_format(pImageFormat);
	}

	std::tuple<vk::ImageUsageFlags, vk::ImageLayout, vk::ImageTiling, vk::ImageCreateFlags> determine_usage_layout_tiling_flags_based_on_image_usage(ak::image_usage aImageUsageFlags)
	{
		vk::ImageUsageFlags imageUsage{};

		bool isReadOnly = ak::has_flag(aImageUsageFlags, ak::image_usage::read_only);
		ak::image_usage cleanedUpUsageFlagsForReadOnly = exclude(aImageUsageFlags, ak::image_usage::transfer_source | ak::image_usage::transfer_destination | ak::image_usage::sampled | ak::image_usage::read_only | ak::image_usage::presentable | ak::image_usage::shared_presentable | ak::image_usage::tiling_optimal | ak::image_usage::tiling_linear | ak::image_usage::sparse_memory_binding | ak::image_usage::cube_compatible | ak::image_usage::is_protected); // TODO: To be verified, it's just a guess.

		auto targetLayout = isReadOnly ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eGeneral; // General Layout or Shader Read Only Layout is the default
		auto imageTiling = vk::ImageTiling::eOptimal; // Optimal is the default
		vk::ImageCreateFlags imageCreateFlags{};

		if (ak::has_flag(aImageUsageFlags, ak::image_usage::transfer_source)) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
			ak::image_usage cleanedUpUsageFlags = exclude(aImageUsageFlags, ak::image_usage::read_only | ak::image_usage::presentable | ak::image_usage::shared_presentable | ak::image_usage::tiling_optimal | ak::image_usage::tiling_linear | ak::image_usage::sparse_memory_binding | ak::image_usage::cube_compatible | ak::image_usage::is_protected | ak::image_usage::mip_mapped); // TODO: To be verified, it's just a guess.
			if (ak::image_usage::transfer_source == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferSrcOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::transfer_destination)) {
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			ak::image_usage cleanedUpUsageFlags = exclude(aImageUsageFlags, ak::image_usage::read_only | ak::image_usage::presentable | ak::image_usage::shared_presentable | ak::image_usage::tiling_optimal | ak::image_usage::tiling_linear | ak::image_usage::sparse_memory_binding | ak::image_usage::cube_compatible | ak::image_usage::is_protected | ak::image_usage::mip_mapped); // TODO: To be verified, it's just a guess.
			if (ak::image_usage::transfer_destination == cleanedUpUsageFlags) {
				targetLayout = vk::ImageLayout::eTransferDstOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eGeneral;
			}
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::sampled)) {
			imageUsage |= vk::ImageUsageFlagBits::eSampled;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::color_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
			targetLayout = vk::ImageLayout::eColorAttachmentOptimal;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::depth_stencil_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			if (isReadOnly && ak::image_usage::depth_stencil_attachment == cleanedUpUsageFlagsForReadOnly) {
				targetLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
			}
			else {
				targetLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			}
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::input_attachment)) {
			imageUsage |= vk::ImageUsageFlagBits::eInputAttachment;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::shading_rate_image)) {
			imageUsage |= vk::ImageUsageFlagBits::eShadingRateImageNV;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::presentable)) {
			targetLayout = vk::ImageLayout::ePresentSrcKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::shared_presentable)) {
			targetLayout = vk::ImageLayout::eSharedPresentKHR; // TODO: This probably needs some further action(s) => implement that further action(s)
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::tiling_optimal)) {
			imageTiling = vk::ImageTiling::eOptimal;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::tiling_linear)) {
			imageTiling = vk::ImageTiling::eLinear;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::sparse_memory_binding)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eSparseBinding;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::cube_compatible)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eCubeCompatible;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::is_protected)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eProtected;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::mutable_format)) {
			imageCreateFlags |= vk::ImageCreateFlagBits::eMutableFormat;
		}
		if (ak::has_flag(aImageUsageFlags, ak::image_usage::shader_storage)) { 
			imageUsage |= vk::ImageUsageFlagBits::eStorage;	
			// Can not be Shader Read Only Layout
			targetLayout = vk::ImageLayout::eGeneral; // TODO: Verify that this should always be in general layout!
		}

		return std::make_tuple(imageUsage, targetLayout, imageTiling, imageCreateFlags);
	}
#pragma endregion

#pragma region ak_error definitions
	runtime_error::runtime_error (const std::string& what_arg) : std::runtime_error(what_arg)
	{
		AK_LOG_ERROR("!RUNTIME ERROR! " + what_arg);
	}
	
	runtime_error::runtime_error (const char* what_arg) : std::runtime_error(what_arg)
	{
		AK_LOG_ERROR("!RUNTIME ERROR! " + std::string(what_arg));
	}

	logic_error::logic_error (const std::string& what_arg) : std::logic_error(what_arg)
	{
		AK_LOG_ERROR("!LOGIC ERROR! " + what_arg);
	}
	
	logic_error::logic_error (const char* what_arg) : std::logic_error(what_arg)
	{
		AK_LOG_ERROR("!LOGIC ERROR! " + std::string(what_arg));
	}
#pragma endregion

#pragma region attachment definitions
	attachment attachment::declare(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp)
	{
		return attachment{
			std::get<vk::Format>(aFormatAndSamples),
			std::get<vk::SampleCountFlagBits>(aFormatAndSamples),
			aLoadOp, aStoreOp,
			{},      {},
			std::move(aUsageInSubpasses),
			{ 0.0, 0.0, 0.0, 0.0 },
			1.0f, 0u
		};
	}
	
	attachment attachment::declare(vk::Format aFormat, on_load aLoadOp, usage_desc aUsageInSubpasses, on_store aStoreOp)
	{
		return declare({aFormat, vk::SampleCountFlagBits::e1}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}
	
	attachment attachment::declare_for(const image_view_t& aImageView, ak::on_load aLoadOp, ak::usage_desc aUsageInSubpasses, ak::on_store aStoreOp)
	{
		const auto& imageConfig = aImageView.get_image().config();
		const auto format = imageConfig.format;
		const std::optional<image_usage> imageUsage = aImageView.get_image().usage_config();
		auto result = declare({format, imageConfig.samples}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
		if (imageUsage.has_value()) {
			result.set_image_usage_hint(imageUsage.value());
		}
		return result;
	}
#pragma endregion

#pragma region bottom level acceleration structure definitions
	ak::owning_resource<bottom_level_acceleration_structure_t> root::create_bottom_level_acceleration_structure(std::vector<ak::acceleration_structure_size_requirements> aGeometryDescriptions, bool aAllowUpdates, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;
		result.mGeometryInfos.reserve(aGeometryDescriptions.size());
		
		// 1. Gather all geometry descriptions and create vk::AccelerationStructureCreateGeometryTypeInfoKHR entries:
		for (auto& gd : aGeometryDescriptions) {
			auto& back = result.mGeometryInfos.emplace_back()
				.setGeometryType(gd.mGeometryType)
				.setMaxPrimitiveCount(gd.mNumPrimitives)
				.setMaxVertexCount(gd.mNumVertices)
				.setVertexFormat(gd.mVertexFormat)
				.setAllowsTransforms(VK_FALSE); // TODO: Add support for transforms (allowsTransforms indicates whether transform data can be used by this acceleration structure or not, when geometryType is VK_GEOMETRY_TYPE_TRIANGLES_KHR.)
			if (vk::GeometryTypeKHR::eTriangles == gd.mGeometryType) {
				back.setIndexType(ak::to_vk_index_type(gd.mIndexTypeSize));
				// TODO: Support non-indexed geometry
			}
		} // for each geometry description
		
		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mCreateInfo = vk::AccelerationStructureCreateInfoKHR{}
			.setCompactedSize(0) // If compactedSize is 0 then maxGeometryCount must not be 0
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(aAllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
					  : vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace) // TODO: Support flags
			.setMaxGeometryCount(static_cast<uint32_t>(result.mGeometryInfos.size()))
			.setPGeometryInfos(result.mGeometryInfos.data())
			.setDeviceAddress(VK_NULL_HANDLE); // TODO: support this (deviceAddress is the device address requested for the acceleration structure if the rayTracingAccelerationStructureCaptureReplay feature is being used.)
		
		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// 4. Create it
		result.mAccStructure = device().createAccelerationStructureKHRUnique(result.mCreateInfo, nullptr, dynamic_dispatch());

		// Steps 5. to 10. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));
		
		return result;
	}

	//const generic_buffer_t& bottom_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	//{
	//	if (!mScratchBuffer.has_value()) {
	//		mScratchBuffer = ak::create(
	//			ak::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size())),
	//			ak::memory_usage::device,
	//			vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR
	//		);
	//	}
	//	return mScratchBuffer.value();
	//}
	
	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(std::vector<std::tuple<std::reference_wrapper<const ak::vertex_buffer_t>, std::reference_wrapper<const ak::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction)
	{
		// TODO: into ak::commands
		
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (aScratchBuffer.has_value()) {
			scratchBuffer = &aScratchBuffer->get();
		}
		else {
			throw ak::runtime_error("Not implemented!");
			//scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}

		std::vector<vk::AccelerationStructureGeometryKHR> accStructureGeometries;
		accStructureGeometries.reserve(aGeometries.size());
		
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
		buildGeometryInfos.reserve(aGeometries.size()); 
		
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> buildOffsetInfos;
		buildOffsetInfos.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> buildOffsetInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		buildOffsetInfoPtrs.reserve(aGeometries.size());

		for (auto& tpl : aGeometries) {
			auto& vertexBuffer = std::get<std::reference_wrapper<const ak::vertex_buffer_t>>(tpl).get();
			auto& indexBuffer = std::get<std::reference_wrapper<const ak::index_buffer_t>>(tpl).get();
			
			if (vertexBuffer.meta_data().member_descriptions().size() == 0) {
				throw ak::runtime_error("ak::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers must have a member_description for their positions element in their meta data.");
			}
			// Find member representing the positions, and...
			auto posMember = std::find_if(
				std::begin(vertexBuffer.meta_data().member_descriptions()), 
				std::end(vertexBuffer.meta_data().member_descriptions()), 
				[](const buffer_element_member_meta& md) {
					return md.mContent == content_description::position;
				});
			// ... perform 2nd check:
			if (posMember == std::end(vertexBuffer.meta_data().member_descriptions())) {
				throw ak::runtime_error("ak::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers has no member which represents positions.");
			}
	
			accStructureGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeKHR::eTriangles)
				.setGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR{}
					.setVertexFormat(posMember->mFormat)
					.setVertexData(vk::DeviceOrHostAddressConstKHR{ vertexBuffer.device_address() }) // TODO: Support host addresses
					.setVertexStride(static_cast<vk::DeviceSize>(vertexBuffer.meta_data().sizeof_one_element()))
					.setIndexType(ak::to_vk_index_type( indexBuffer.meta_data().sizeof_one_element()))
					.setIndexData(vk::DeviceOrHostAddressConstKHR{ indexBuffer.device_address() }) // TODO: Support host addresses
					.setTransformData(nullptr)
				)
				.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

			auto& boi = buildOffsetInfos.emplace_back()
				.setPrimitiveCount(static_cast<uint32_t>(indexBuffer.meta_data().num_elements()) / 3u)
				.setPrimitiveOffset(0u)
				.setFirstVertex(0u)
				.setTransformOffset(0u); // TODO: Support different values for all these parameters?!

			buildOffsetInfoPtrs.emplace_back(&boi);
		}
		
		const auto* pointerToAnArray = accStructureGeometries.data();
		
		buildGeometryInfos.emplace_back()
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(mCreateInfo.flags) // TODO: support individual flags per geometry?
			.setUpdate(aBuildAction == blas_action::build ? VK_FALSE : VK_TRUE)
			.setSrcAccelerationStructure(nullptr) // TODO: support different src acceleration structure?!
			.setDstAccelerationStructure(mAccStructure.get())
			.setGeometryArrayOfPointers(VK_FALSE)
			.setGeometryCount(static_cast<uint32_t>(accStructureGeometries.size()))
			.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->device_address() });

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});
		
		// Operation:
		commandBuffer.handle().buildAccelerationStructureKHR(
			static_cast<uint32_t>(buildGeometryInfos.size()), 
			buildGeometryInfos.data(),
			buildOffsetInfoPtrs.data(),
			mDynamicDispatch
		);

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});
		
		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	void bottom_level_acceleration_structure_t::build(std::vector<std::tuple<std::reference_wrapper<const ak::vertex_buffer_t>, std::reference_wrapper<const ak::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::build);
	}
	
	void bottom_level_acceleration_structure_t::update(std::vector<std::tuple<std::reference_wrapper<const ak::vertex_buffer_t>, std::reference_wrapper<const ak::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::update);
	}

	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(ak::generic_buffer aBuffer, std::vector<ak::aabb> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction)
	{
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (aScratchBuffer.has_value()) {
			scratchBuffer = &aScratchBuffer->get();
		}
		else {
			throw ak::runtime_error("Not implemented!");
			//scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}

		std::vector<vk::AccelerationStructureGeometryKHR> accStructureGeometries;
		//accStructureGeometries.reserve(aGeometries.size());
		
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
		//buildGeometryInfos.reserve(aGeometries.size()); 
		
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> buildOffsetInfos;
		//buildOffsetInfos.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> buildOffsetInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		//buildOffsetInfoPtrs.reserve(aGeometries.size());

		accStructureGeometries.emplace_back()
			.setGeometryType(vk::GeometryTypeKHR::eAabbs)
			.setGeometry(vk::AccelerationStructureGeometryAabbsDataKHR{}
				.setData(vk::DeviceOrHostAddressConstKHR{ aBuffer->device_address() })
				.setStride(0)
			)
			.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

		auto& boi = buildOffsetInfos.emplace_back()
			.setPrimitiveCount(static_cast<uint32_t>(aGeometries.size()))
			.setPrimitiveOffset(0u)
			.setFirstVertex(0u)
			.setTransformOffset(0u); // TODO: Support different values for all these parameters?!

		buildOffsetInfoPtrs.emplace_back(&boi);
		
		const auto* pointerToAnArray = accStructureGeometries.data();
		
		buildGeometryInfos.emplace_back()
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(mCreateInfo.flags) // TODO: support individual flags per geometry?
			.setUpdate(aBuildAction == blas_action::build ? VK_FALSE : VK_TRUE)
			.setSrcAccelerationStructure(nullptr) // TODO: support different src acceleration structure?!
			.setDstAccelerationStructure(mAccStructure.get())
			.setGeometryArrayOfPointers(VK_FALSE)
			.setGeometryCount(static_cast<uint32_t>(accStructureGeometries.size()))
			.setPpGeometries(&pointerToAnArray)
			.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->device_address() });

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});
		
		// Operation:
		commandBuffer.handle().buildAccelerationStructureKHR(
			static_cast<uint32_t>(buildGeometryInfos.size()), 
			buildGeometryInfos.data(),
			buildOffsetInfoPtrs.data(),
			mDynamicDispatch
		);

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});
		
		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	void bottom_level_acceleration_structure_t::build(ak::generic_buffer aBuffer, std::vector<ak::aabb> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aBuffer), std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::build);
	}
	
	void bottom_level_acceleration_structure_t::update(ak::generic_buffer aBuffer, std::vector<ak::aabb> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aBuffer), std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::update);
	}
#pragma endregion

#pragma region buffer definitions

#pragma endregion

#pragma region buffer view definitions
	owning_resource<buffer_view_t> root::create_buffer_view(ak::uniform_texel_buffer aBufferToOwn, std::optional<vk::Format> aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		vk::Format format;
		if (aViewFormat.has_value()) {
			format = aViewFormat.value();
		}
		else {
			if (aBufferToOwn->meta_data().member_descriptions().size() == 0) {
				throw ak::runtime_error("No _ViewFormat passed and ak::uniform_texel_buffer contains no member descriptions");
			}
			if (aBufferToOwn->meta_data().member_descriptions().size() > 1) {
				AK_LOG_WARNING("No aViewFormat passed and there is more than one member description in ak::uniform_texel_buffer. The view will likely be corrupted.");
			}
			format = aBufferToOwn->meta_data().member_descriptions().front().mFormat;
		}
		// Transfer ownership:
		result.mBuffer = std::move(aBufferToOwn);
		finish_configuration(result, format, std::move(aAlterConfigBeforeCreation));
		return result;
	}
	
	owning_resource<buffer_view_t> root::create_buffer_view(ak::storage_texel_buffer aBufferToOwn, std::optional<vk::Format> aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		vk::Format format;
		if (aViewFormat.has_value()) {
			format = aViewFormat.value();
		}
		else {
			if (aBufferToOwn->meta_data().member_descriptions().size() == 0) {
				throw ak::runtime_error("No aViewFormat passed and ak::storage_texel_buffer contains no member descriptions");
			}
			if (aBufferToOwn->meta_data().member_descriptions().size() > 1) {
				AK_LOG_WARNING("No aViewFormat passed and there is more than one member description in ak::storage_texel_buffer. The view will likely be corrupted.");
			}
			format = aBufferToOwn->meta_data().member_descriptions().front().mFormat;
		}
		// Transfer ownership:
		result.mBuffer = std::move(aBufferToOwn);
		finish_configuration(result, format, std::move(aAlterConfigBeforeCreation));
		return result;
	}
	
	owning_resource<buffer_view_t> root::create_buffer_view(vk::Buffer aBufferToReference, vk::BufferCreateInfo aBufferInfo, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation)
	{
		buffer_view_t result;
		// Store handles:
		result.mBuffer = std::make_tuple(aBufferToReference, aBufferInfo);
		finish_configuration(result, aViewFormat, std::move(aAlterConfigBeforeCreation));
		return result;
	}
#pragma endregion

#pragma region command pool and command buffer definitions
	command_pool root::create_command_pool(vk::CommandPoolCreateFlags aCreateFlags)
	{
		const auto queueFamilyIndex = queue_family_index();
		auto createInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(queueFamilyIndex)
			.setFlags(aCreateFlags);
		command_pool result;
		result.mQueueFamilyIndex = queueFamilyIndex;
		result.mCreateInfo = createInfo;
		result.mCommandPool = device().createCommandPoolUnique(createInfo);
		return result;
	}

	std::vector<owning_resource<command_buffer_t>> root::create_command_buffers(uint32_t aCount, vk::CommandPoolCreateFlags aCommandPoolFlags, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel)
	{
		auto bufferAllocInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(command_pool_for_flags(aCommandPoolFlags))
			.setLevel(aLevel) 
			.setCommandBufferCount(aCount);

		auto tmp = device().allocateCommandBuffersUnique(bufferAllocInfo);

		// Iterate over all the "raw"-Vk objects in `tmp` and...
		std::vector<owning_resource<command_buffer_t>> buffers;
		buffers.reserve(aCount);
		std::transform(std::begin(tmp), std::end(tmp),
			std::back_inserter(buffers),
			// ...transform them into `ak::command_buffer_t` objects:
			[lUsageFlags = aUsageFlags](auto& vkCb) -> owning_resource<command_buffer_t> {
				command_buffer_t result;
				result.mBeginInfo = vk::CommandBufferBeginInfo()
					.setFlags(lUsageFlags)
					.setPInheritanceInfo(nullptr);
				result.mCommandBuffer = std::move(vkCb);
				return result;
			});
		return buffers;
	}

	owning_resource<command_buffer_t> root::create_command_buffer(vk::CommandPoolCreateFlags aCommandPoolFlags, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel)
	{
		auto result = std::move(create_command_buffers(1, aCommandPoolFlags, aUsageFlags, aLevel)[0]);
		return result;
	}

	command_buffer root::create_command_buffer(bool aSimultaneousUseEnabled, bool aPrimary)
	{
		auto usageFlags = vk::CommandBufferUsageFlags();
		if (aSimultaneousUseEnabled) {
			usageFlags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = create_command_buffer(vk::CommandPoolCreateFlags{}, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;
	}

	std::vector<command_buffer> root::create_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		auto usageFlags = vk::CommandBufferUsageFlags();
		if (aSimultaneousUseEnabled) {
			usageFlags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = create_command_buffers(aNumBuffers, vk::CommandPoolCreateFlags{}, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;
	}
	
	command_buffer root::create_single_use_command_buffer(bool aPrimary)
	{
		const vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		auto result = create_command_buffer(vk::CommandPoolCreateFlagBits::eTransient, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;
	}

	std::vector<command_buffer> root::create_single_use_command_buffers(uint32_t aNumBuffers, bool aPrimary)
	{
		const vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		auto result = create_command_buffers(aNumBuffers, vk::CommandPoolCreateFlagBits::eTransient, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;		
	}

	command_buffer root::create_resettable_command_buffer(bool aSimultaneousUseEnabled, bool aPrimary)
	{
		vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		if (aSimultaneousUseEnabled) {
			usageFlags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = create_command_buffer(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;
	}

	std::vector<command_buffer> root::create_resettable_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled, bool aPrimary)
	{
		vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		if (aSimultaneousUseEnabled) {
			usageFlags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		}
		auto result = create_command_buffers(aNumBuffers, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, usageFlags, aPrimary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		return result;
	}
	
	command_buffer_t::~command_buffer_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	void command_buffer_t::invoke_post_execution_handler() const
	{
		if (mPostExecutionHandler.has_value() && *mPostExecutionHandler) {
			(*mPostExecutionHandler)();
		}
	}
	void command_buffer_t::begin_recording()
	{
		mCommandBuffer->begin(mBeginInfo);
		mState = command_buffer_state::recording;
	}

	void command_buffer_t::end_recording()
	{
		mCommandBuffer->end();
		mState = command_buffer_state::finished_recording;
	}

	void command_buffer_t::begin_render_pass_for_framebuffer(const renderpass_t& aRenderpass, framebuffer_t& aFramebuffer, vk::Offset2D aRenderAreaOffset, std::optional<vk::Extent2D> aRenderAreaExtent, bool aSubpassesInline)
	{
		const auto firstAttachmentsSize = aFramebuffer.image_view_at(0)->get_image().config().extent;
		const auto& clearValues = aRenderpass.clear_values();
		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(aRenderpass.handle())
			.setFramebuffer(aFramebuffer.handle())
			.setRenderArea(vk::Rect2D()
				.setOffset(vk::Offset2D{ aRenderAreaOffset.x, aRenderAreaOffset.y })
				.setExtent(aRenderAreaExtent.has_value() 
							? vk::Extent2D{ aRenderAreaExtent.value() } 
							: vk::Extent2D{ firstAttachmentsSize.width,  firstAttachmentsSize.height }
					)
				)
			.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
			.setPClearValues(clearValues.data());

		mSubpassContentsState = aSubpassesInline ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers;
		mCommandBuffer->beginRenderPass(renderPassBeginInfo, mSubpassContentsState);
		// 2nd parameter: how the drawing commands within the render pass will be provided. It can have one of two values [7]:
		//  - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
		//  - VK_SUBPASS_CONTENTS_SECONDARY_command_buffer_tS : The render pass commands will be executed from secondary command buffers.

		// Sorry, but have to do this:
#ifdef _DEBUG
		bool hadToEnable = false;
#endif
		std::vector<ak::image_view> imageViews;
		for (auto& view : aFramebuffer.image_views()) {
			if (!view.is_shared_ownership_enabled()) {
				view.enable_shared_ownership();
#ifdef _DEBUG
				hadToEnable = true;
#endif
			}
			imageViews.push_back(view);
		}
#ifdef _DEBUG
		if (hadToEnable) {
			AK_LOG_DEBUG("Had to enable shared ownership on all the framebuffers' views in command_buffer_t::begin_render_pass_for_framebuffer, fyi.");
		}
#endif
		set_post_execution_handler([lAttachmentDescs = aRenderpass.attachment_descriptions(), lImageViews = std::move(imageViews)] () {
			const auto n = lImageViews.size();
			for (size_t i = 0; i < n; ++i) {
				// I think, the const_cast is justified here:
				const_cast<image_t&>(lImageViews[i]->get_image()).set_current_layout(lAttachmentDescs[i].finalLayout);
			}
		});
	}

	void command_buffer_t::next_subpass()
	{
		mCommandBuffer->nextSubpass(mSubpassContentsState);
	}

	void command_buffer_t::establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage), // Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage), // Which stage has to wait until memory has been made visible
			vk::DependencyFlags{}, // TODO: support dependency flags
			{},	{}, {} // no memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),				// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),				// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},								// TODO: support dependency flags
			{ vk::MemoryBarrier{								// Establish a global memory barrier, ...
				to_vk_access_flags(aSrcAccessToBeMadeAvailable),//  ... making memory from these access types available (after aSrcStage),
				to_vk_access_flags(aDstAccessToBeMadeVisible)	//  ... and for these access types visible (before aDstStage)
			}},
			{}, {} // no buffer/image memory barriers
		);
	}

	void command_buffer_t::establish_global_memory_barrier_rw(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_global_memory_barrier(aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible)
	{
		mCommandBuffer->pipelineBarrier(
			to_vk_pipeline_stage_flags(aSrcStage),						// Up to which stage to execute before making memory available
			to_vk_pipeline_stage_flags(aDstStage),						// Which stage has to wait until memory has been made visible
			vk::DependencyFlags{},										// TODO: support dependency flags
			{}, {},														// no global memory barriers, no buffer memory barriers
			{
				vk::ImageMemoryBarrier{
					to_vk_access_flags(aSrcAccessToBeMadeAvailable),	// After the aSrcStage, make this memory available
					to_vk_access_flags(aDstAccessToBeMadeVisible),		// Before the aDstStage, make this memory visible
					aImage.current_layout(), aImage.target_layout(),	// Transition for the former to the latter
					VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,	// TODO: Support queue family ownership transfer
					aImage.handle(),
					aImage.entire_subresource_range()					// TODO: Support different subresource ranges
				}
			}
		);
		aImage.set_current_layout(aImage.target_layout()); // Just optimistically set it
	}
	
	void command_buffer_t::establish_image_memory_barrier_rw(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible)
	{
		establish_image_memory_barrier(aImage, aSrcStage, aDstStage, to_memory_access(aSrcAccessToBeMadeAvailable), to_memory_access(aDstAccessToBeMadeVisible));
	}

	void command_buffer_t::copy_image(const image_t& aSource, const vk::Image& aDestination)
	{ // TODO: fix this hack after the RTX-VO!
		auto fullImageOffset = vk::Offset3D(0, 0, 0);
		auto fullImageExtent = aSource.config().extent;
		auto halfImageOffset = vk::Offset3D(0, 0, 0); //vk::Offset3D(pSource.mInfo.extent.width / 2, 0, 0);
		auto halfImageExtent = vk::Extent3D(aSource.config().extent.width, aSource.config().extent.height, aSource.config().extent.depth);
		auto offset = halfImageOffset;
		auto extent = halfImageExtent;

		auto copyInfo = vk::ImageCopy()
			.setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setSrcOffset(offset)
			.setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u))
			.setDstOffset(offset)
			.setExtent(extent);
		mCommandBuffer->copyImage(aSource.handle(), vk::ImageLayout::eTransferSrcOptimal, aDestination, vk::ImageLayout::eTransferDstOptimal, { copyInfo });
	}

	void command_buffer_t::end_render_pass()
	{
		mCommandBuffer->endRenderPass();
	}

	void command_buffer_t::bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::vector<descriptor_set> aDescriptorSets)
	{
		if (aDescriptorSets.size() == 0) {
			AK_LOG_WARNING("command_buffer_t::bind_descriptors has been called, but there are no descriptor sets to be bound.");
			return;
		}

		std::vector<vk::DescriptorSet> handles;
		handles.reserve(aDescriptorSets.size());
		for (const auto& dset : aDescriptorSets)
		{
			handles.push_back(dset.handle());
		}

		if (aDescriptorSets.empty()) {
			return;
		}

		// Issue one or multiple bindDescriptorSets commands. We can only bind CONSECUTIVELY NUMBERED sets.
		size_t descIdx = 0;
		while (descIdx < aDescriptorSets.size()) {
			const uint32_t setId = aDescriptorSets[descIdx].set_id();
			uint32_t count = 1u;
			while ((descIdx + count) < aDescriptorSets.size() && aDescriptorSets[descIdx + count].set_id() == (setId + count)) {
				++count;
			}

			handle().bindDescriptorSets(
				aBindingPoint, 
				aLayoutHandle, 
				setId, count,
				&handles[descIdx], 
				0, // TODO: Dynamic offset count
				nullptr); // TODO: Dynamic offset

			descIdx += count;
		}
	}
#pragma endregion

#pragma compute pipeline definitions
	owning_resource<compute_pipeline_t> root::create_compute_pipeline(compute_pipeline_config aConfig, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		compute_pipeline_t result;

		// 1. Compile and store the one and only shader:
		if (!aConfig.mShaderInfo.has_value()) {
			throw ak::logic_error("Shader missing in compute_pipeline_config! A compute pipeline can not be constructed without a shader.");
		}
		//    Compile the shader
		result.mShader = std::move(shader::create(aConfig.mShaderInfo.value()));
		assert(result.mShader.has_been_built());
		//    Just fill in the create struct
		result.mShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo{}
			.setStage(to_vk_shader_stage(result.mShader.info().mShaderType))
			.setModule(result.mShader.handle())
			.setPName(result.mShader.info().mEntryPoint.c_str());
		
		// 2. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((aConfig.mPipelineSettings & cfg::pipeline_settings::disable_optimization) == cfg::pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 3. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		allocate_descriptor_set_layouts(result.mAllDescriptorSetLayouts);
		
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 4. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Create the PIPELINE LAYOUT
		result.mPipelineLayout = device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::ComputePipelineCreateInfo{}
			.setFlags(result.mPipelineCreateFlags)
			.setStage(result.mShaderStageCreateInfo)
			.setLayout(result.layout_handle())
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional
		result.mPipeline = device().createComputePipelineUnique(nullptr, pipelineInfo);

		return result;
	}
#pragma endregion

#pragma descriptor alloc request
	descriptor_alloc_request::descriptor_alloc_request(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		mNumSets = static_cast<uint32_t>(aLayouts.size());

		for (const auto& layout : aLayouts) {
			// Accumulate all the memory requirements of all the sets
			for (const auto& entry : layout.get().required_pool_sizes()) {
				auto it = std::lower_bound(std::begin(mAccumulatedSizes), std::end(mAccumulatedSizes), 
					entry,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				if (it != std::end(mAccumulatedSizes) && it->type == entry.type) {
					it->descriptorCount += entry.descriptorCount;
				}
				else {
					mAccumulatedSizes.insert(it, entry);
				}
			}
		}
	}

	void descriptor_alloc_request::add_size_requirements(vk::DescriptorPoolSize aToAdd)
	{
		auto it = std::lower_bound(std::begin(mAccumulatedSizes), std::end(mAccumulatedSizes), 
			aToAdd,
			[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
				using EnumType = std::underlying_type<vk::DescriptorType>::type;
				return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
			});
		if (it != std::end(mAccumulatedSizes) && it->type == aToAdd.type) {
			it->descriptorCount += aToAdd.descriptorCount;
		}
		else {
			mAccumulatedSizes.insert(it, aToAdd);
		}
	}

	descriptor_alloc_request descriptor_alloc_request::multiply_size_requirements(uint32_t mFactor) const
	{
		auto copy = descriptor_alloc_request{*this};
		for (auto& sr : copy.mAccumulatedSizes) {
			sr.descriptorCount *= mFactor;
		}
		return copy;
	}
#pragma endregion

#pragma region descriptor pool definitions
	std::shared_ptr<descriptor_pool> root::create_descriptor_pool(const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets)
	{
		descriptor_pool result;
		result.mInitialCapacities = aSizeRequirements;
		result.mRemainingCapacities = aSizeRequirements;
		result.mNumInitialSets = aNumSets;
		result.mNumRemainingSets = aNumSets;

		// Create it:
		auto createInfo = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(static_cast<uint32_t>(result.mInitialCapacities.size()))
			.setPPoolSizes(result.mInitialCapacities.data())
			.setMaxSets(aNumSets) 
			.setFlags(vk::DescriptorPoolCreateFlags()); // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, so we don't need this flag. [10]
		result.mDescriptorPool = device().createDescriptorPoolUnique(createInfo);
		
		AK_LOG_DEBUG("Allocated pool with flags[" + vk::to_string(createInfo.flags) + "], maxSets[" + std::to_string(createInfo.maxSets) + "], remaining-sets[" + std::to_string(result.mNumRemainingSets) + "], size-entries[" + std::to_string(createInfo.poolSizeCount) + "]");
#if defined(_DEBUG)
		for (size_t i=0; i < aSizeRequirements.size(); ++i) {
			AK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: descriptorCount[" + std::to_string(aSizeRequirements[i].descriptorCount) + "], descriptorType[" + vk::to_string(aSizeRequirements[i].type) + "]");
		}
#endif
		
		return std::make_shared<descriptor_pool>(std::move(result));
	}

	bool descriptor_pool::has_capacity_for(const descriptor_alloc_request& pRequest) const
	{
		//if (mNumRemainingSets < static_cast<int>(pRequest.num_sets())) {
		//	return false;
		//}

		const auto& weNeed = pRequest.accumulated_pool_sizes();
		const auto& weHave = mRemainingCapacities;

		// Accumulate all the requirements of all the sets
		using EnumType = std::underlying_type<vk::DescriptorType>::type;
		size_t n = 0, h = 0, N = weNeed.size(), H = weHave.size();
		while (n < N && h < H) {
			auto needType = static_cast<EnumType>(weNeed[n].type);
			auto haveType = static_cast<EnumType>(weHave[h].type);
			if (haveType < needType) {
				h++; 
				continue;
			}
			if (needType == haveType && weNeed[n].descriptorCount <= weHave[n].descriptorCount) {
				n++;
				h++;
				continue;
			}
			return false;
		}
		return n == h; // if true => all checks have passed, if false => checks failed
	}

	std::vector<vk::DescriptorSet> descriptor_pool::allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		std::vector<vk::DescriptorSetLayout> setLayouts;
		setLayouts.reserve(aLayouts.size());
		for (auto& in : aLayouts) {
			setLayouts.emplace_back(in.get().handle());
		}
		
		auto allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(mDescriptorPool.get()) 
			.setDescriptorSetCount(static_cast<uint32_t>(setLayouts.size()))
			.setPSetLayouts(setLayouts.data());

		AK_LOG_DEBUG_VERBOSE("Allocated pool with remaining-sets[" + std::to_string(mNumRemainingSets) + "] and remaining-capacities:");
#if defined(_DEBUG)
		for (size_t i=0; i < mRemainingCapacities.size(); ++i) {
			AK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: descriptorCount[" + std::to_string(mRemainingCapacities[i].descriptorCount) + "], descriptorType[" + vk::to_string(mRemainingCapacities[i].type) + "]");
		}
#endif
		AK_LOG_DEBUG_VERBOSE("...going to allocate " + std::to_string(aLayouts.size()) + " set(s) of the following:");
#if defined(_DEBUG)
		for (size_t i=0; i < aLayouts.size(); ++i) {
			AK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: number_of_bindings[" + std::to_string(aLayouts[i].get().number_of_bindings()) + "]");
			for (size_t j=0; j < aLayouts[i].get().number_of_bindings(); j++) {
				AK_LOG_DEBUG_VERBOSE("               [" + std::to_string(j) + "]: descriptorCount[" + std::to_string(aLayouts[i].get().binding_at(j).descriptorCount) + "], descriptorType[" + vk::to_string(aLayouts[i].get().binding_at(j).descriptorType) + "]");
			}
			AK_LOG_DEBUG_VERBOSE("          [" + std::to_string(i) + "]: required pool sizes (whatever the difference to 'bindings' is)");
			auto& rps = aLayouts[i].get().required_pool_sizes();
			for (size_t j=0; j < rps.size(); j++) {
				AK_LOG_DEBUG_VERBOSE("               [" + std::to_string(j) + "]: descriptorCount[" + std::to_string(rps[j].descriptorCount) + "], descriptorType[" + vk::to_string(rps[j].type) + "]");
			}
		}
#endif

		assert(mDescriptorPool);
		auto result = mDescriptorPool.getOwner().allocateDescriptorSets(allocInfo);

		// Update the pool's stats:
		for (const descriptor_set_layout& dsl : aLayouts) {
			for (const auto& dps : dsl.required_pool_sizes()) {
				auto it = std::find_if(std::begin(mRemainingCapacities), std::end(mRemainingCapacities), [&dps](vk::DescriptorPoolSize& el){
					return el.type == dps.type;
				});
				if (std::end(mRemainingCapacities) == it) {
					AK_LOG_WARNING("Couldn't find the descriptor type that we have just allocated in mRemainingCapacities. How could this have happened?");
				}
				else {
					it->descriptorCount -= std::min(dps.descriptorCount, it->descriptorCount);
				}
			}
		}

		mNumRemainingSets -= static_cast<int>(aLayouts.size());

		return result;
	}
#pragma endregion

#pragma region descriptor set layout definitions
	void root::allocate_descriptor_set_layout(descriptor_set_layout& aLayoutToBeAllocated)
	{
		if (!aLayoutToBeAllocated.mLayout) {
			// Allocate the layout and return the result:
			auto createInfo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(static_cast<uint32_t>(aLayoutToBeAllocated.mOrderedBindings.size()))
				.setPBindings(aLayoutToBeAllocated.mOrderedBindings.data());
			aLayoutToBeAllocated.mLayout = device().createDescriptorSetLayoutUnique(createInfo);
		}
		else {
			AK_LOG_ERROR("descriptor_set_layout's handle already has a value => it most likely has already been allocated. Won't do it again.");
		}
	}

	set_of_descriptor_set_layouts set_of_descriptor_set_layouts::prepare(std::vector<binding_data> pBindings)
	{
		set_of_descriptor_set_layouts result;
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : pBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}

		// Step 2: assemble the separate sets
		result.mFirstSetId = minSetId;
		result.mLayouts.reserve(maxSetId); // Also create layouts for sets that have no bindings, i.e. ALWAYS prepare ALL sets for EACH set-id from 0 to maxSetId
		for (uint32_t setId = 0u; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			// For empty sets, lb==ub, which means no descriptors will be regarded. This should be fine.
			result.mLayouts.push_back(descriptor_set_layout::prepare(lb, ub));
		}

		// Step 3: Accumulate the binding requirements a.k.a. vk::DescriptorPoolSize entries
		for (auto& dsl : result.mLayouts) {
			for (auto& dps : dsl.required_pool_sizes()) {
				// find position where to insert in vector
				auto it = std::lower_bound(std::begin(result.mBindingRequirements), std::end(result.mBindingRequirements),
					dps,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				// Maybe accumulate
				if (it != std::end(result.mBindingRequirements) && it->type == dps.type) {
					it->descriptorCount += dps.descriptorCount;
				}
				else {
					result.mBindingRequirements.insert(it, dps);
				}
			}
		}

		// Done with the preparation. (None of the descriptor_set_layout members have been allocated at this point.)
		return result;
	}

	void root::allocate_descriptor_set_layouts(set_of_descriptor_set_layouts& aLayoutsToBeAllocated)
	{
		for (auto& dsl : aLayoutsToBeAllocated.mLayouts) {
			allocate_descriptor_set_layout(dsl);
		}
	}

	std::vector<vk::DescriptorSetLayout> set_of_descriptor_set_layouts::layout_handles() const
	{
		std::vector<vk::DescriptorSetLayout> allHandles;
		allHandles.reserve(mLayouts.size());
		for (auto& dsl : mLayouts) {
			allHandles.push_back(dsl.handle());
		}
		return allHandles;
	}
#pragma endregion

#pragma region standard descriptor set

	const descriptor_set_layout& standard_descriptor_cache::get_or_alloc_layout(root& aRoot, descriptor_set_layout aPreparedLayout)
	{
		const auto it = mLayouts.find(aPreparedLayout);
		if (mLayouts.end() != it) {
			assert(it->handle());
			return *it;
		}

		aRoot.allocate_descriptor_set_layout(aPreparedLayout);
		
		const auto result = mLayouts.insert(std::move(aPreparedLayout));
		assert(result.second);
		return *result.first;
	}

	std::optional<descriptor_set> standard_descriptor_cache::get_descriptor_set_from_cache(const descriptor_set& aPreparedSet)
	{
		const auto it = mSets.find(aPreparedSet);
		if (mSets.end() != it) {
			auto found = *it;
			return found;
		}
		return {};
	}

	std::vector<descriptor_set> standard_descriptor_cache::alloc_new_descriptor_sets(root& aRoot, const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets)
	{
		assert(aLayouts.size() == aPreparedSets.size());

		std::vector<descriptor_set> result;
		const auto n = aLayouts.size();
#ifdef _DEBUG // Perform an extensive sanity check:
		for (size_t i = 0; i < n; ++i) {
			const auto dbgB = aLayouts[i].get().number_of_bindings();
			assert(dbgB == aPreparedSets[i].number_of_writes());
			for (size_t j = 0; j < dbgB; ++j) {
				assert(aLayouts[i].get().binding_at(j).binding			== aPreparedSets[i].write_at(j).dstBinding);
				assert(aLayouts[i].get().binding_at(j).descriptorCount	== aPreparedSets[i].write_at(j).descriptorCount);
				assert(aLayouts[i].get().binding_at(j).descriptorType	== aPreparedSets[i].write_at(j).descriptorType);
			}
		}
#endif
		
		auto allocRequest = descriptor_alloc_request{ aLayouts };

		std::shared_ptr<descriptor_pool> pool = nullptr;
		std::vector<vk::DescriptorSet> setHandles;
		
		auto poolToTry = get_descriptor_pool_for_layouts(aRoot, allocRequest, 'stch');
		
		int maxTries = 3;
		while (!pool && maxTries-- > 0) {
			try {
				assert(poolToTry->has_capacity_for(allocRequest));
				// Alloc the whole thing:
				setHandles = poolToTry->allocate(aLayouts);
				assert(setHandles.size() == aPreparedSets.size());
				// Success
				pool = poolToTry;
			}
			catch (vk::OutOfPoolMemoryError& fail) {
				AK_LOG_ERROR(std::string("Failed to allocate descriptor sets from pool: ") + fail.what());
				switch (maxTries) {
				case 1:
					AK_LOG_INFO("Trying again with doubled size requirements...");
					allocRequest = allocRequest.multiply_size_requirements(2u);
					poolToTry = get_descriptor_pool_for_layouts(aRoot, allocRequest, 'stch');
				default:
					AK_LOG_INFO("Trying again with new pool..."); // and possibly doubled size requirements, depending on whether maxTries is 2 or 0
					poolToTry = get_descriptor_pool_for_layouts(aRoot, allocRequest, 'stch', true);
				}
			}
		}

		assert(pool);
		assert(setHandles.size() > 0);
			
		for (size_t i = 0; i < n; ++i) {
			auto& setToBeCompleted = aPreparedSets[i];
			setToBeCompleted.link_to_handle_and_pool(std::move(setHandles[i]), pool);
			setToBeCompleted.update_data_pointers();
			setToBeCompleted.write_descriptors();
			
			// Your soul... is mine:
			const auto cachedSet = mSets.insert(std::move(setToBeCompleted));
			assert(cachedSet.second); // TODO: Maybe remove this because the application should not really fail in that case.
			// Done. Store for result:
			result.push_back(*cachedSet.first); // Make a copy!
		}

		return result;
	}
	
	void standard_descriptor_cache::cleanup()
	{
		mSets.clear();
		mLayouts.clear();
	}

	std::shared_ptr<descriptor_pool> standard_descriptor_cache::get_descriptor_pool_for_layouts(root& aRoot, const descriptor_alloc_request& aAllocRequest, int aPoolName, bool aRequestNewPool)
	{
		// We'll allocate the pools per (thread and name)
		auto pId = pool_id{std::this_thread::get_id(), aPoolName};
		auto& pools = mDescriptorPools[pId];

		// First of all, do some cleanup => remove all pools which no longer exist:
		pools.erase(std::remove_if(std::begin(pools), std::end(pools), [](const std::weak_ptr<descriptor_pool>& ptr) {
			return ptr.expired();
		}), std::end(pools));

		// Find a pool which is capable of allocating this:
		if (!aRequestNewPool) {
			for (auto& pool : pools) {
				if (auto sptr = pool.lock()) {
					if (sptr->has_capacity_for(aAllocRequest)) {
						return sptr;
					}
				}
			}
		}
		
		// We weren't lucky (or new pool has been requested) => create a new pool:
		AK_LOG_INFO("Allocating new descriptor pool for thread[" + [tid = pId.mThreadId]() { std::stringstream ss; ss << tid; return ss.str(); }() + "] and name['" + fourcc_to_string(pId.mName) + "'/" + std::to_string(pId.mName) + "]");
		
		// TODO: On AMD, it seems that all the entries have to be multiplied as well, while on NVIDIA, only multiplying the number of sets seems to be sufficient
		//       => How to handle this? Overallocation is as bad as underallocation. Shall we make use of exceptions? Shall we 'if' on the vendor?

		const bool isNvidia = 0x12d2 == aRoot.physical_device().getProperties().vendorID;
		auto amplifiedAllocRequest = aAllocRequest.multiply_size_requirements(sDescriptorPoolPreallocFactor);
		//if (!isNvidia) { // Let's 'if' on the vendor and see what happens...
		//}

		auto newPool = aRoot.create_descriptor_pool(
			isNvidia 
			  ? aAllocRequest.accumulated_pool_sizes()
			  : amplifiedAllocRequest.accumulated_pool_sizes(),
			isNvidia
			  ? aAllocRequest.num_sets() * sDescriptorPoolPreallocFactor
			  : aAllocRequest.num_sets() * sDescriptorPoolPreallocFactor * 2 // the last factor is a "magic number"/"educated guess"/"preemtive strike"
		);

		//  However, set the stored capacities to the amplified version, to not mess up our internal "has_capacity_for-logic":
		newPool->set_remaining_capacities(amplifiedAllocRequest.accumulated_pool_sizes());
		//if (!isNvidia) {
		//}
		
		pools.emplace_back(newPool); // Store as a weak_ptr
		return newPool;
	}
#pragma endregion

#pragma region descriptor set definitions
	void descriptor_set::update_data_pointers()
	{
		for (auto& w : mOrderedDescriptorDataWrites) {
			assert(w.dstSet == mOrderedDescriptorDataWrites[0].dstSet);
			{
				auto it = std::find_if(std::begin(mStoredImageInfos), std::end(mStoredImageInfos), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredImageInfos)) {
					w.pImageInfo = std::get<std::vector<vk::DescriptorImageInfo>>(*it).data();
				}
				else {
					w.pImageInfo = nullptr;
				}
			}
			{
				auto it = std::find_if(std::begin(mStoredBufferInfos), std::end(mStoredBufferInfos), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredBufferInfos)) {
					w.pBufferInfo = std::get<std::vector<vk::DescriptorBufferInfo>>(*it).data();
				}
				else {
					w.pBufferInfo = nullptr;
				}
			}
			{
				auto it = std::find_if(std::begin(mStoredAccelerationStructureWrites), std::end(mStoredAccelerationStructureWrites), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredAccelerationStructureWrites)) {
					auto& tpl = std::get<1>(*it);
					w.pNext = &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(tpl);
					// Also update the pointer WITHIN the vk::WriteDescriptorSetAccelerationStructureKHR... OMG!
					std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(tpl).pAccelerationStructures = std::get<1>(tpl).data();
				}
				else {
					w.pNext = nullptr;
				}
			}
			{
				auto it = std::find_if(std::begin(mStoredBufferViews), std::end(mStoredBufferViews), [binding = w.dstBinding](const auto& element) { return std::get<uint32_t>(element) == binding; });
				if (it != std::end(mStoredBufferViews)) {
					w.pTexelBufferView = std::get<std::vector<vk::BufferView>>(*it).data();
				}
				else {
					w.pTexelBufferView = nullptr;
				}
			}
		}
	}

	void descriptor_set::link_to_handle_and_pool(vk::DescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool)
	{
		mDescriptorSet = aHandle;
		for (auto& w : mOrderedDescriptorDataWrites) {
			w.setDstSet(handle());
		}
		mPool = std::move(aPool);
	}
	
	void descriptor_set::write_descriptors()
	{
		assert(mDescriptorSet);
		update_data_pointers();
		mPool.get()->mDescriptorPool.getOwner().updateDescriptorSets(static_cast<uint32_t>(mOrderedDescriptorDataWrites.size()), mOrderedDescriptorDataWrites.data(), 0u, nullptr);
	}
	
	std::vector<descriptor_set> root::get_or_create_descriptor_sets(std::initializer_list<binding_data> aBindings)
	{
		auto& descriptorCache = descriptor_cache();
		
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : aBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}
		
		std::vector<std::reference_wrapper<const descriptor_set_layout>> layouts;
		std::vector<descriptor_set> preparedSets;
		std::vector<descriptor_set> cachedSets;
		std::vector<bool>           validSets;

		// Step 2: go through all the sets, get or alloc layouts, and see if the descriptor sets are already in cache, by chance.
		for (uint32_t setId = minSetId; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});

			// Handle empty sets:
			if (lb == ub) {
				continue;
			}

			const auto& layout = descriptorCache.get_or_alloc_layout(*this, descriptor_set_layout::prepare(lb, ub));
			layouts.emplace_back(layout);
			auto preparedSet = descriptor_set::prepare(lb, ub);
			auto cachedSet = descriptorCache.get_descriptor_set_from_cache(preparedSet);
			if (cachedSet.has_value()) {
				cachedSets.emplace_back(std::move(cachedSet));
				validSets.push_back(true);
			}
			else {
				cachedSets.emplace_back();
				validSets.push_back(false);
			}
			preparedSets.emplace_back(std::move(preparedSet));
		}

		if (static_cast<int>(cachedSets.size()) == std::count(std::begin(validSets), std::end(validSets), true)) {
			// Everything is cached; we're done.
			return cachedSets;
		}

		// HOWEVER, if not...
		std::vector<std::reference_wrapper<const descriptor_set_layout>> layoutsForAlloc;
		std::vector<descriptor_set> toBeAlloced;
		std::vector<size_t> indexMapping;
		for (size_t i = 0; i < cachedSets.size(); ++i) {
			if (!validSets[i]) {
				layoutsForAlloc.push_back(layouts[i]);
				toBeAlloced.push_back(std::move(preparedSets[i]));
				indexMapping.push_back(i);
			}
		}
		auto nowAlsoInCache = descriptorCache.alloc_new_descriptor_sets(*this, layoutsForAlloc, std::move(toBeAlloced));
		for (size_t i = 0; i < indexMapping.size(); ++i) {
			cachedSets[indexMapping[i]] = nowAlsoInCache[i];
		}
		return cachedSets;
	}
#pragma endregion

#pragma region fence definitions
	fence_t::~fence_t()
	{
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
		// Destroy the dependant instance before destroying myself
		// ^ This is ensured by the order of the members
		//   See: https://isocpp.org/wiki/faq/dtors#calling-member-dtors
	}

	fence_t& fence_t::set_designated_queue(device_queue& _Queue)
	{
		mQueue = &_Queue;
		return *this;
	}

	void fence_t::wait_until_signalled() const
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		mFence.getOwner().waitForFences(1u, handle_ptr(), VK_TRUE, UINT64_MAX);
	}

	void fence_t::reset()
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		mFence.getOwner().resetFences(1u, handle_ptr());
		if (mCustomDeleter.has_value() && *mCustomDeleter) {
			// If there is a custom deleter => call it now
			(*mCustomDeleter)();
			mCustomDeleter.reset();
		}
	}

	ak::owning_resource<fence_t> root::create_fence(bool aCreateInSignalledState, std::function<void(fence_t&)> aAlterConfigBeforeCreation)
	{
		fence_t result;
		result.mCreateInfo = vk::FenceCreateInfo()
			.setFlags(aCreateInSignalledState 
						? vk::FenceCreateFlagBits::eSignaled
						: vk::FenceCreateFlags() 
			);

		// Maybe alter the config?
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mFence = device().createFenceUnique(result.mCreateInfo);
		return result;
	}
#pragma endregion

#pragma framebuffer definitions
	void root::check_and_config_attachments_based_on_views(std::vector<attachment>& aAttachments, std::vector<image_view>& aImageViews)
	{
		if (aAttachments.size() != aImageViews.size()) {
			throw ak::runtime_error("Incomplete config for framebuffer creation: number of attachments (" + std::to_string(aAttachments.size()) + ") does not equal the number of image views (" + std::to_string(aImageViews.size()) + ")");
		}
		auto n = aAttachments.size();
		for (size_t i = 0; i < n; ++i) {
			auto& a = aAttachments[i];
			auto& v = aImageViews[i];
			if ((is_depth_format(v->get_image().format()) || has_stencil_component(v->get_image().format())) && !a.is_used_as_depth_stencil_attachment()) {
				AK_LOG_WARNING("Possibly misconfigured framebuffer: image[" + std::to_string(i) + "] is a depth/stencil format, but it is never indicated to be used as such in the attachment-description[" + std::to_string(i) + "]");
			}
			// TODO: Maybe further checks?
			if (!a.mImageUsageHintBefore.has_value() && !a.mImageUsageHintAfter.has_value()) {
				a.mImageUsageHintAfter = a.mImageUsageHintBefore = v->get_image().usage_config();
			}
		}
	}

	owning_resource<framebuffer_t> root::create_framebuffer(renderpass aRenderpass, std::vector<ak::image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		framebuffer_t result;
		result.mRenderpass = std::move(aRenderpass);
		result.mImageViews = std::move(aImageViews);

		std::vector<vk::ImageView> imageViewHandles;
		for (const auto& iv : result.mImageViews) {
			imageViewHandles.push_back(iv->handle());
		}

		result.mCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(result.mRenderpass->handle())
			.setAttachmentCount(static_cast<uint32_t>(imageViewHandles.size()))
			.setPAttachments(imageViewHandles.data())
			.setWidth(aWidth)
			.setHeight(aHeight)
			// TODO: Support multiple layers of image arrays!
			.setLayers(1u); // number of layers in image arrays [6]

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mFramebuffer = device().createFramebufferUnique(result.mCreateInfo);

		// Set the right layouts for the images:
		const auto n = result.mImageViews.size();
		const auto& attDescs = result.mRenderpass->attachment_descriptions();
		for (size_t i = 0; i < n; ++i) {
			result.mImageViews[i]->get_image().transition_to_layout(attDescs[i].initialLayout);
		}
		
		return result;
	}

	owning_resource<framebuffer_t> root::create_framebuffer(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			renderpass_t::create(std::move(aAttachments)),
			std::move(aImageViews),
			aWidth, aHeight,
			std::move(aAlterConfigBeforeCreation)
		);
	}

	owning_resource<framebuffer_t> root::create_framebuffer(renderpass aRenderpass, std::vector<ak::image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		assert(!aImageViews.empty());
		auto extent = aImageViews.front()->get_image().config().extent;
		return create_framebuffer(std::move(aRenderpass), std::move(aImageViews), extent.width, extent.height, std::move(aAlterConfigBeforeCreation));
	}

	owning_resource<framebuffer_t> root::create_framebuffer(std::vector<ak::attachment> aAttachments, std::vector<ak::image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation)
	{
		check_and_config_attachments_based_on_views(aAttachments, aImageViews);
		return create_framebuffer(
			std::move(renderpass_t::create(std::move(aAttachments))),
			std::move(aImageViews),
			std::move(aAlterConfigBeforeCreation)
		);
	}

	std::optional<command_buffer> framebuffer_t::initialize_attachments(sync aSync)
	{
		aSync.establish_barrier_before_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring
		
		const int n = mImageViews.size();
		assert (n == mRenderpass->attachment_descriptions().size());
		for (size_t i = 0; i < n; ++i) {
			mImageViews[i]->get_image().transition_to_layout(mRenderpass->attachment_descriptions()[i].finalLayout, sync::auxiliary_with_barriers(aSync, {}, {}));
		}

		aSync.establish_barrier_after_the_operation(pipeline_stage::transfer, {}); // TODO: Don't use transfer after barrier-stage-refactoring
		return aSync.submit_and_sync();
	}
#pragma endregion

#pragma region geometry instance definitions
	geometry_instance::geometry_instance(const bottom_level_acceleration_structure_t& aBlas)
		: mTransform{ { 1.0f, 0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f } }
		, mInstanceCustomIndex{ 0 }
		, mMask{ 0xff }
		, mInstanceOffset{ 0 }
		, mFlags{ vk::GeometryInstanceFlagsKHR() }
		, mAccelerationStructureDeviceHandle{ aBlas.device_address() }		
	{ }

	geometry_instance& geometry_instance::set_transform(VkTransformMatrixKHR aTransformationMatrix)
	{
		mTransform = aTransformationMatrix;
		return *this;
	}
	
	geometry_instance& geometry_instance::set_transform(std::array<float, 12> aTransformationMatrix)
	{
		// transpose it along the way:
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[1];
		mTransform.matrix[0][2] = aTransformationMatrix[2];
		mTransform.matrix[0][3] = aTransformationMatrix[3];
		mTransform.matrix[1][0] = aTransformationMatrix[4];
		mTransform.matrix[1][1] = aTransformationMatrix[5];
		mTransform.matrix[1][2] = aTransformationMatrix[6];
		mTransform.matrix[1][3] = aTransformationMatrix[7];
		mTransform.matrix[2][0] = aTransformationMatrix[8];
		mTransform.matrix[2][1] = aTransformationMatrix[9];
		mTransform.matrix[2][2] = aTransformationMatrix[10];
		mTransform.matrix[2][3] = aTransformationMatrix[11];
		// TODO: Which order ^ or v ?
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[3];
		mTransform.matrix[0][2] = aTransformationMatrix[6];
		mTransform.matrix[0][3] = aTransformationMatrix[9];
		mTransform.matrix[1][0] = aTransformationMatrix[1];
		mTransform.matrix[1][1] = aTransformationMatrix[4];
		mTransform.matrix[1][2] = aTransformationMatrix[7];
		mTransform.matrix[1][3] = aTransformationMatrix[10];
		mTransform.matrix[2][0] = aTransformationMatrix[2];
		mTransform.matrix[2][1] = aTransformationMatrix[5];
		mTransform.matrix[2][2] = aTransformationMatrix[8];
		mTransform.matrix[2][3] = aTransformationMatrix[11];
		return *this;
	}
	
	geometry_instance& geometry_instance::set_transform(std::array<float, 16> aTransformationMatrix)
	{
		// transpose it along the way:
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[1];
		mTransform.matrix[0][2] = aTransformationMatrix[2];
		mTransform.matrix[0][3] = aTransformationMatrix[3];
		mTransform.matrix[1][0] = aTransformationMatrix[4];
		mTransform.matrix[1][1] = aTransformationMatrix[5];
		mTransform.matrix[1][2] = aTransformationMatrix[6];
		mTransform.matrix[1][3] = aTransformationMatrix[7];
		mTransform.matrix[2][0] = aTransformationMatrix[8];
		mTransform.matrix[2][1] = aTransformationMatrix[9];
		mTransform.matrix[2][2] = aTransformationMatrix[10];
		mTransform.matrix[2][3] = aTransformationMatrix[11];
		// TODO: Which order ^ or v ?
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[3];
		mTransform.matrix[0][2] = aTransformationMatrix[6];
		mTransform.matrix[0][3] = aTransformationMatrix[9];
		mTransform.matrix[1][0] = aTransformationMatrix[1];
		mTransform.matrix[1][1] = aTransformationMatrix[4];
		mTransform.matrix[1][2] = aTransformationMatrix[7];
		mTransform.matrix[1][3] = aTransformationMatrix[10];
		mTransform.matrix[2][0] = aTransformationMatrix[2];
		mTransform.matrix[2][1] = aTransformationMatrix[5];
		mTransform.matrix[2][2] = aTransformationMatrix[8];
		mTransform.matrix[2][3] = aTransformationMatrix[11];
		// TODO: ...or is it one of the following??
		mTransform.matrix[0][0] = aTransformationMatrix[0];
		mTransform.matrix[0][1] = aTransformationMatrix[4];
		mTransform.matrix[0][2] = aTransformationMatrix[8];
		mTransform.matrix[0][3] = aTransformationMatrix[12];
		mTransform.matrix[1][0] = aTransformationMatrix[1];
		mTransform.matrix[1][1] = aTransformationMatrix[5];
		mTransform.matrix[1][2] = aTransformationMatrix[9];
		mTransform.matrix[1][3] = aTransformationMatrix[13];
		mTransform.matrix[2][0] = aTransformationMatrix[2];
		mTransform.matrix[2][1] = aTransformationMatrix[6];
		mTransform.matrix[2][2] = aTransformationMatrix[10];
		mTransform.matrix[2][3] = aTransformationMatrix[14];
		return *this;
	}

	geometry_instance& geometry_instance::set_custom_index(uint32_t aCustomIndex)
	{
		mInstanceCustomIndex = aCustomIndex;
		return *this;
	}

	geometry_instance& geometry_instance::set_mask(uint32_t aMask)
	{
		mMask = aMask;
		return *this;
	}

	geometry_instance& geometry_instance::set_instance_offset(size_t aOffset)
	{
		mInstanceOffset = aOffset;
		return *this;
	}

	geometry_instance& geometry_instance::set_flags(vk::GeometryInstanceFlagsKHR aFlags)
	{
		mFlags = aFlags;
		return *this;
	}

	geometry_instance& geometry_instance::add_flags(vk::GeometryInstanceFlagsKHR aFlags)
	{
		mFlags |= aFlags;
		return *this;
	}

	geometry_instance& geometry_instance::disable_culling()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable;
		return *this;
	}

	geometry_instance& geometry_instance::define_front_faces_to_be_counter_clockwise()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise;
		return *this;
	}

	geometry_instance& geometry_instance::force_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::force_non_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceNoOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::reset_flags()
	{
		mFlags = vk::GeometryInstanceFlagsKHR();
		return *this;
	}

	VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst)
	{
		VkAccelerationStructureInstanceKHR element;
		//auto matrix = glm::transpose(aGeomInst.mTransform);
		//memcpy(&element.transform, glm::value_ptr(matrix), sizeof(element.transform));
		element.transform = aGeomInst.mTransform;
		element.instanceCustomIndex = aGeomInst.mInstanceCustomIndex;
		element.mask = aGeomInst.mMask;
		element.instanceShaderBindingTableRecordOffset = aGeomInst.mInstanceOffset;
		element.flags = static_cast<uint32_t>(aGeomInst.mFlags);
		element.accelerationStructureReference = aGeomInst.mAccelerationStructureDeviceHandle;
		return element;
	}

	std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances)
	{
		if (aGeomInstances.size() == 0) {
			AK_LOG_WARNING("Empty vector of geometry instances");
		}

		std::vector<VkAccelerationStructureInstanceKHR> instancesGpu;
		instancesGpu.reserve(aGeomInstances.size());
		for (auto& data : aGeomInstances) {
			instancesGpu.emplace_back(convert_for_gpu_usage(data));			
		}
		return instancesGpu;
	}
#pragma endregion

#pragma region graphics pipeline definitions
	owning_resource<graphics_pipeline_t> root::create_graphics_pipeline(graphics_pipeline_config aConfig, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		using namespace cpplinq;
		using namespace cfg;

		graphics_pipeline_t result;

		// 0. Own the renderpass
		{
			assert(aConfig.mRenderPassSubpass.has_value());
			auto [rp, sp] = std::move(aConfig.mRenderPassSubpass.value());
			result.mRenderPass = std::move(rp);
			result.mSubpassIndex = sp;
		}

		// 1. Compile the array of vertex input binding descriptions
		{ 
			// Select DISTINCT bindings:
			auto bindings = from(aConfig.mInputBindingLocations)
				>> select([](const input_binding_location_data& _BindingData) { return _BindingData.mGeneralData; })
				>> distinct() // see what I did there
				>> orderby([](const input_binding_general_data& _GeneralData) { return _GeneralData.mBinding; })
				>> to_vector();
			result.mVertexInputBindingDescriptions.reserve(bindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!

			for (auto& bindingData : bindings) {

				const auto numRecordsWithSameBinding = std::count_if(std::begin(bindings), std::end(bindings), 
					[bindingId = bindingData.mBinding](const input_binding_general_data& _GeneralData) {
						return _GeneralData.mBinding == bindingId;
					});
				if (1 != numRecordsWithSameBinding) {
					throw ak::runtime_error("The input binding #" + std::to_string(bindingData.mBinding) + " is defined in different ways. Make sure to define it uniformly across different bindings/attribute descriptions!");
				}

				result.mVertexInputBindingDescriptions.push_back(vk::VertexInputBindingDescription()
					// The following parameters are guaranteed to be the same. We have checked this.
					.setBinding(bindingData.mBinding)
					.setStride(static_cast<uint32_t>(bindingData.mStride))
					.setInputRate(to_vk_vertex_input_rate(bindingData.mKind))
					// Don't need the location here
				);
			}
		}

		// 2. Compile the array of vertex input attribute descriptions
		//  They will reference the bindings created in step 1.
		result.mVertexInputAttributeDescriptions.reserve(aConfig.mInputBindingLocations.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& attribData : aConfig.mInputBindingLocations) {
			result.mVertexInputAttributeDescriptions.push_back(vk::VertexInputAttributeDescription()
				.setBinding(attribData.mGeneralData.mBinding)
				.setLocation(attribData.mMemberMetaData.mLocation)
				.setFormat(attribData.mMemberMetaData.mFormat)
				.setOffset(static_cast<uint32_t>(attribData.mMemberMetaData.mOffset))
			);
		}

		// 3. With the data from 1. and 2., create the complete vertex input info struct, passed to the pipeline creation
		result.mPipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(static_cast<uint32_t>(result.mVertexInputBindingDescriptions.size()))
			.setPVertexBindingDescriptions(result.mVertexInputBindingDescriptions.data())
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(result.mVertexInputAttributeDescriptions.size()))
			.setPVertexAttributeDescriptions(result.mVertexInputAttributeDescriptions.data());

		// 4. Set how the data (from steps 1.-3.) is to be interpreted (e.g. triangles, points, lists, patches, etc.)
		result.mInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(to_vk_primitive_topology(aConfig.mPrimitiveTopology))
			.setPrimitiveRestartEnable(VK_FALSE);

		// 5. Compile and store the shaders:
		result.mShaders.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		result.mShaderStageCreateInfos.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& shaderInfo : aConfig.mShaderInfos) {
			// 5.0 Sanity check
			if (result.mShaders.end() != std::find_if(std::begin(result.mShaders), std::end(result.mShaders), [&shaderInfo](const shader& existing) { return existing.info().mShaderType == shaderInfo.mShaderType; })) {
				throw ak::runtime_error("There's already a " + vk::to_string(to_vk_shader_stages(shaderInfo.mShaderType)) + "-type shader contained in this graphics pipeline. Can not add another one of the same type.");
			}
			// 5.1 Compile the shader
			result.mShaders.push_back(shader::create(shaderInfo));
			assert(result.mShaders.back().has_been_built());
			// 5.2 Combine
			result.mShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo{}
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str())
			);
		}

		// 6. Viewport configuration
		{
			// 6.1 Viewport and depth configuration(s):
			result.mViewports.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			result.mScissors.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (auto& vp : aConfig.mViewportDepthConfig) {
				result.mViewports.push_back(vk::Viewport{}
					.setX(vp.x())
					.setY(vp.y())
					.setWidth(vp.width())
					.setHeight(vp.height())
					.setMinDepth(vp.min_depth())
					.setMaxDepth(vp.max_depth())
				);
				// 6.2 Skip scissors for now
				// TODO: Implement scissors support properly
				result.mScissors.push_back(vk::Rect2D{}
					.setOffset({static_cast<int32_t>(vp.x()), static_cast<int32_t>(vp.y())})
					.setExtent({static_cast<uint32_t>(vp.width()), static_cast<uint32_t>(vp.height())})
				);
			}
			// 6.3 Add everything together
			result.mViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
				.setViewportCount(static_cast<uint32_t>(result.mViewports.size()))
				.setPViewports(result.mViewports.data())
				.setScissorCount(static_cast<uint32_t>(result.mScissors.size()))
				.setPScissors(result.mScissors.data());
		}

		// 7. Rasterization state
		result.mRasterizationStateCreateInfo =  vk::PipelineRasterizationStateCreateInfo{}
			// Various, but important settings:
			.setRasterizerDiscardEnable(to_vk_bool(aConfig.mRasterizerGeometryMode == rasterizer_geometry_mode::discard_geometry))
			.setPolygonMode(to_vk_polygon_mode(aConfig.mPolygonDrawingModeAndConfig.drawing_mode()))
			.setLineWidth(aConfig.mPolygonDrawingModeAndConfig.line_width())
			.setCullMode(to_vk_cull_mode(aConfig.mCullingMode))
			.setFrontFace(to_vk_front_face(aConfig.mFrontFaceWindingOrder.winding_order_of_front_faces()))
			// Depth-related settings:
			.setDepthClampEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_clamp_to_frustum_enabled()))
			.setDepthBiasEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_depth_bias_enabled()))
			.setDepthBiasConstantFactor(aConfig.mDepthClampBiasConfig.bias_constant_factor())
			.setDepthBiasClamp(aConfig.mDepthClampBiasConfig.bias_clamp_value())
			.setDepthBiasSlopeFactor(aConfig.mDepthClampBiasConfig.bias_slope_factor());

		// 8. Depth-stencil config
		result.mDepthStencilConfig = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(to_vk_bool(aConfig.mDepthTestConfig.is_enabled()))
			.setDepthCompareOp(to_vk_compare_op(aConfig.mDepthTestConfig.depth_compare_operation()))
			.setDepthWriteEnable(to_vk_bool(aConfig.mDepthWriteConfig.is_enabled()))
			.setDepthBoundsTestEnable(to_vk_bool(aConfig.mDepthBoundsConfig.is_enabled()))
			.setMinDepthBounds(aConfig.mDepthBoundsConfig.min_bounds())
			.setMaxDepthBounds(aConfig.mDepthBoundsConfig.max_bounds())
			.setStencilTestEnable(VK_FALSE);

		// TODO: Add better support for stencil testing (better abstraction!)
		if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().mEnabled) {
			result.mDepthStencilConfig
				.setStencilTestEnable(VK_TRUE)
				.setFront(aConfig.mStencilTest.value().mFrontStencilTestActions)
				.setBack(aConfig.mStencilTest.value().mBackStencilTestActions);
		}

		// 9. Color Blending
		{ 
			// Do we have an "universal" color blending config? That means, one that is not assigned to a specific color target attachment id.
			auto universalConfig = from(aConfig.mColorBlendingPerAttachment)
				>> where([](const color_blending_config& config) { return !config.mTargetAttachment.has_value(); })
				>> to_vector();

			if (universalConfig.size() > 1) {
				throw ak::runtime_error("Ambiguous 'universal' color blending configurations. Either provide only one 'universal' "
					"config (which is not attached to a specific color target) or assign them to specific color target attachment ids.");
			}

			// Iterate over all color target attachments and set a color blending config
			if (result.subpass_id() >= result.mRenderPass->attachment_descriptions().size()) {
				throw ak::runtime_error("There are fewer subpasses in the renderpass (" + std::to_string(result.mRenderPass->attachment_descriptions().size()) + ") as the subpass index indicates (" + std::to_string(result.subpass_id()) + "). I.e. subpass index is out of bounds.");
			}
			const auto n = result.mRenderPass->color_attachments_for_subpass(result.subpass_id()).size(); /////////////////// TODO: (doublecheck or) FIX this section (after renderpass refactoring)
			result.mBlendingConfigsForColorAttachments.reserve(n); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (size_t i = 0; i < n; ++i) {
				// Do we have a specific blending config for color attachment i?
				auto configForI = from(aConfig.mColorBlendingPerAttachment)
					>> where([i](const color_blending_config& config) { return config.mTargetAttachment.has_value() && config.mTargetAttachment.value() == i; })
					>> to_vector();
				if (configForI.size() > 1) {
					throw ak::runtime_error("Ambiguous color blending configuration for color attachment at index #" + std::to_string(i) + ". Provide only one config per color attachment!");
				}
				// Determine which color blending to use for this attachment:
				color_blending_config toUse = configForI.size() == 1 ? configForI[0] : color_blending_config::disable();
				result.mBlendingConfigsForColorAttachments.push_back(vk::PipelineColorBlendAttachmentState()
					.setColorWriteMask(to_vk_color_components(toUse.affected_color_channels()))
					.setBlendEnable(to_vk_bool(toUse.is_blending_enabled())) // If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. [4]
					.setSrcColorBlendFactor(to_vk_blend_factor(toUse.color_source_factor())) 
					.setDstColorBlendFactor(to_vk_blend_factor(toUse.color_destination_factor()))
					.setColorBlendOp(to_vk_blend_operation(toUse.color_operation()))
					.setSrcAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_source_factor()))
					.setDstAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_destination_factor()))
					.setAlphaBlendOp(to_vk_blend_operation(toUse.alpha_operation()))
				);
			}

			// General blending settings and reference to the array of color attachment blending configs
			result.mColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(to_vk_bool(aConfig.mColorBlendingSettings.is_logic_operation_enabled())) // If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. [4]
				.setLogicOp(to_vk_logic_operation(aConfig.mColorBlendingSettings.logic_operation())) 
				.setAttachmentCount(static_cast<uint32_t>(result.mBlendingConfigsForColorAttachments.size()))
				.setPAttachments(result.mBlendingConfigsForColorAttachments.data())
				.setBlendConstants(aConfig.mColorBlendingSettings.blend_constants());
		}

		// 10. Multisample state
		// TODO: Can the settings be inferred from the renderpass' color attachments (as they are right now)? If they can't, how to handle this situation? 
		{ /////////////////// TODO: FIX this section (after renderpass refactoring)
			vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;

			// See what is configured in the render pass
			auto colorAttConfigs = from ((*result.mRenderPass).color_attachments_for_subpass(result.subpass_id()))
				>> where ([](const vk::AttachmentReference& colorAttachment) { return colorAttachment.attachment != VK_ATTACHMENT_UNUSED; })
				// The color_attachments() contain indices of the actual attachment_descriptions() => select the latter!
				>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& colorAttachment) { return rp.attachment_descriptions()[colorAttachment.attachment]; })
				>> to_vector();

			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
				numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
			}

#if defined(_DEBUG) 
			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				if (config.samples != numSamples) {
					AK_LOG_DEBUG("Not all of the color target attachments have the same number of samples configured, fyi. This might be fine, though.");
				}
			}
#endif
			
			if (vk::SampleCountFlagBits::e1 == numSamples) {
				auto depthAttConfigs = from ((*result.mRenderPass).depth_stencil_attachments_for_subpass(result.subpass_id()))
					>> where ([](const vk::AttachmentReference& depthStencilAttachment) { return depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED; })
					>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& depthStencilAttachment) { return rp.attachment_descriptions()[depthStencilAttachment.attachment]; })
					>> to_vector();

				for (const vk::AttachmentDescription& config: depthAttConfigs) {
					typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
					numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
				}

#if defined(_DEBUG) 
					for (const vk::AttachmentDescription& config: depthAttConfigs) {
						if (config.samples != numSamples) {
							AK_LOG_DEBUG("Not all of the depth/stencil target attachments have the same number of samples configured, fyi. This might be fine, though.");
						}
					}
#endif

#if defined(_DEBUG) 
					for (const vk::AttachmentDescription& config: colorAttConfigs) {
						if (config.samples != numSamples) {
							AK_LOG_DEBUG("Some of the color target attachments have different numbers of samples configured as the depth/stencil attachments, fyi. This might be fine, though.");
						}
					}
#endif
			}
			
			// Evaluate and set the PER SAMPLE shading configuration:
			auto perSample = aConfig.mPerSampleShading.value_or(per_sample_shading_config{ false, 1.0f });
			
			result.mMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
				.setRasterizationSamples(numSamples)
				.setSampleShadingEnable(perSample.mPerSampleShadingEnabled ? VK_TRUE : VK_FALSE) // enable/disable Sample Shading
				.setMinSampleShading(perSample.mMinFractionOfSamplesShaded) // specifies a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
				.setPSampleMask(nullptr) // If pSampleMask is NULL, it is treated as if the mask has all bits enabled, i.e. no coverage is removed from fragments. See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#fragops-samplemask
				.setAlphaToCoverageEnable(VK_FALSE) // controls whether a temporary coverage value is generated based on the alpha component of the fragment�s first color output as specified in the Multisample Coverage section.
				.setAlphaToOneEnable(VK_FALSE); // controls whether the alpha component of the fragment�s first color output is replaced with one as described in Multisample Coverage.
			// TODO: That is probably not enough for every case. Further customization options should be added!
		}

		// 11. Dynamic state
		{
			// Don't need to pre-alloc the storage for this one

			// Check for viewport dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_viewport_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eViewport);
				}
			}
			// Check for scissor dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_scissor_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eScissor);
				}
			}
			// Check for dynamic line width
			if (aConfig.mPolygonDrawingModeAndConfig.dynamic_line_width()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eLineWidth);
			}
			// Check for dynamic depth bias
			if (aConfig.mDepthClampBiasConfig.is_dynamic_depth_bias_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBias);
			}
			// Check for dynamic depth bounds
			if (aConfig.mDepthBoundsConfig.is_dynamic_depth_bounds_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBounds);
			}
			// Check for dynamic stencil values // TODO: make them configurable separately
			if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().is_dynamic_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilCompareMask);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilReference);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilWriteMask);
			}
			// TODO: Support further dynamic states

			result.mDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
				.setDynamicStateCount(static_cast<uint32_t>(result.mDynamicStateEntries.size()))
				.setPDynamicStates(result.mDynamicStateEntries.data());
		}

		// 12. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((aConfig.mPipelineSettings & pipeline_settings::disable_optimization) == pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 13. Patch Control Points for Tessellation
		if (aConfig.mTessellationPatchControlPoints.has_value()) {
			result.mPipelineTessellationStateCreateInfo = vk::PipelineTessellationStateCreateInfo{}
				.setPatchControlPoints(aConfig.mTessellationPatchControlPoints.value().mPatchControlPoints);
		}

		// 14. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		allocate_descriptor_set_layouts(result.mAllDescriptorSetLayouts);
		
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}
		// These uniform values (Anm.: passed to shaders) need to be specified during pipeline creation by creating a VkPipelineLayout object. [4]
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size())) 
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Create the PIPELINE LAYOUT
		result.mPipelineLayout = device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		assert (aConfig.mRenderPassSubpass.has_value());
		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::GraphicsPipelineCreateInfo{}
			// 0. Render Pass
			.setRenderPass((*result.mRenderPass).handle())
			.setSubpass(result.mSubpassIndex)
			// 1., 2., and 3.
			.setPVertexInputState(&result.mPipelineVertexInputStateCreateInfo)
			// 4.
			.setPInputAssemblyState(&result.mInputAssemblyStateCreateInfo)
			// 5.
			.setStageCount(static_cast<uint32_t>(result.mShaderStageCreateInfos.size()))
			.setPStages(result.mShaderStageCreateInfos.data())
			// 6.
			.setPViewportState(&result.mViewportStateCreateInfo)
			// 7.
			.setPRasterizationState(&result.mRasterizationStateCreateInfo)
			// 8.
			.setPDepthStencilState(&result.mDepthStencilConfig)
			// 9.
			.setPColorBlendState(&result.mColorBlendStateCreateInfo)
			// 10.
			.setPMultisampleState(&result.mMultisampleStateCreateInfo)
			// 11.
			.setPDynamicState(result.mDynamicStateEntries.size() == 0 ? nullptr : &result.mDynamicStateCreateInfo) // Optional
			// 12.
			.setFlags(result.mPipelineCreateFlags)
			// LAYOUT:
			.setLayout(result.layout_handle())
			// Base pipeline:
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional

		// 13.
		if (result.mPipelineTessellationStateCreateInfo.has_value()) {
			pipelineInfo.setPTessellationState(&result.mPipelineTessellationStateCreateInfo.value());
		}

		// TODO: Shouldn't the config be altered HERE, after the pipelineInfo has been compiled?!
		
		result.mPipeline = device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
		return result;
	}
#pragma endregion
	
#pragma region image definitions
	image_t::image_t(const image_t& aOther)
	{
		if (std::holds_alternative<vk::Image>(aOther.mImage)) {
			assert(!aOther.mMemory);
			mInfo = aOther.mInfo; 
			mImage = std::get<vk::Image>(aOther.mImage);
			mTargetLayout = aOther.mTargetLayout;
			mCurrentLayout = aOther.mCurrentLayout;
			mImageUsage = aOther.mImageUsage;
			mAspectFlags = aOther.mAspectFlags;
		}
		else {
			throw ak::runtime_error("Can not copy this image instance!");
		}
	}
	
	owning_resource<image_t> root::create_image(uint32_t aWidth, uint32_t aHeight, std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, int aNumLayers, memory_usage aMemoryUsage, image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Determine image usage flags, image layout, and memory usage flags:
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);
		
		vk::MemoryPropertyFlags memoryFlags{};
		switch (aMemoryUsage) {
		case ak::memory_usage::host_visible:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;
			break;
		case ak::memory_usage::host_coherent:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			break;
		case ak::memory_usage::host_cached:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
			break;
		case ak::memory_usage::device:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst; 
			break;
		case ak::memory_usage::device_readback:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
			break;
		case ak::memory_usage::device_protected:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eProtected;
			imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
			break;
		}

		// How many MIP-map levels are we going to use?
		auto mipLevels = ak::has_flag(aImageUsage, ak::image_usage::mip_mapped)
			? static_cast<uint32_t>(1 + std::floor(std::log2(std::max(aWidth, aHeight))))
			: 1u;

		const auto format = std::get<vk::Format>(aFormatAndSamples);
		const auto samples = std::get<vk::SampleCountFlagBits>(aFormatAndSamples);
		
		if (ak::has_flag(imageUsage, vk::ImageUsageFlagBits::eDepthStencilAttachment) && vk::ImageTiling::eOptimal == imageTiling) { // only for AMD |-(
			auto formatProps = physical_device().getFormatProperties(format);
			if (!has_flag(formatProps.optimalTilingFeatures, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
				imageTiling = vk::ImageTiling::eLinear;
			}
		}
		
		vk::ImageAspectFlags aspectFlags = {};
		if (is_depth_format(format)) {
			aspectFlags |= vk::ImageAspectFlagBits::eDepth;
		}
		if (has_stencil_component(format)) {
			aspectFlags |= vk::ImageAspectFlagBits::eStencil;
		}
		if (!aspectFlags) {
			aspectFlags = vk::ImageAspectFlagBits::eColor;
			// TODO: maybe support further aspect flags?!
		}
		
		image_t result;
		result.mInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D) // TODO: Support 3D textures
			.setExtent(vk::Extent3D(static_cast<uint32_t>(aWidth), static_cast<uint32_t>(aHeight), 1u))
			.setMipLevels(mipLevels)
			.setArrayLayers(1u) // TODO: support multiple array layers!!!!!!!!!
			.setFormat(format)
			.setTiling(imageTiling)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(imageUsage)
			.setSharingMode(vk::SharingMode::eExclusive) // TODO: Not sure yet how to handle this one, Exclusive should be the default, though.
			.setSamples(samples)
			.setFlags(imageCreateFlags);
		result.mTargetLayout = targetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = aspectFlags;

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		// Create the image...
		result.mImage = device().createImageUnique(result.mInfo);
		
		// ... and the memory:
		auto memRequirements = device().getImageMemoryRequirements(result.handle());
		auto allocInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memRequirements.size)
			.setMemoryTypeIndex(find_memory_type_index(memRequirements.memoryTypeBits, memoryFlags));
		result.mMemory = device().allocateMemoryUnique(allocInfo);

		// bind them together:
		device().bindImageMemory(result.handle(), result.memory_handle(), 0);
		
		return result;
	}

	owning_resource<image_t> root::create_image(uint32_t aWidth, uint32_t aHeight, vk::Format aFormat, int aNumLayers, memory_usage aMemoryUsage, ak::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		return create_image(aWidth, aHeight, std::make_tuple(aFormat, vk::SampleCountFlagBits::e1), aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
	}


	owning_resource<image_t> root::create_depth_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat, int aNumLayers,  memory_usage aMemoryUsage, ak::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Select a suitable depth format
		if (!aFormat) {
			std::array depthFormats = { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm };
			for (auto format : depthFormats) {
				if (is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					aFormat = format;
					break;
				}
			}
		}
		if (!aFormat) {
			throw ak::runtime_error("No suitable depth format could be found.");
		}

		aImageUsage |= ak::image_usage::depth_stencil_attachment;

		// Create the image (by default only on the device which should be sufficient for a depth buffer => see pMemoryUsage's default value):
		auto result = create_image(aWidth, aHeight, *aFormat, aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
		result->mAspectFlags |= vk::ImageAspectFlagBits::eDepth;
		return result;
	}

	owning_resource<image_t> root::create_depth_stencil_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat, int aNumLayers,  memory_usage aMemoryUsage, ak::image_usage aImageUsage, std::function<void(image_t&)> aAlterConfigBeforeCreation)
	{
		// Select a suitable depth+stencil format
		if (!aFormat) {
			std::array depthFormats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint };
			for (auto format : depthFormats) {
				if (is_format_supported(format, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
					aFormat = format;
					break;
				}
			}
		}
		if (!aFormat) {
			throw ak::runtime_error("No suitable depth+stencil format could be found.");
		}

		// Create the image (by default only on the device which should be sufficient for a depth+stencil buffer => see pMemoryUsage's default value):
		auto result = create_depth_image(aWidth, aHeight, *aFormat, aNumLayers, aMemoryUsage, aImageUsage, std::move(aAlterConfigBeforeCreation));
		result->mAspectFlags |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return result;
	}

	image_t root::wrap_image(vk::Image aImageToWrap, vk::ImageCreateInfo aImageCreateInfo, ak::image_usage aImageUsage, vk::ImageAspectFlags aImageAspectFlags)
	{
		auto [imageUsage, targetLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageUsage);
		
		image_t result;
		result.mInfo = aImageCreateInfo;
		result.mImage = aImageToWrap;		
		result.mTargetLayout = targetLayout;
		result.mCurrentLayout = vk::ImageLayout::eUndefined;
		result.mImageUsage = aImageUsage;
		result.mAspectFlags = aImageAspectFlags; 
		return result;
	}
	
	vk::ImageSubresourceRange image_t::entire_subresource_range() const
	{
		return vk::ImageSubresourceRange{
			mAspectFlags,
			0u, mInfo.mipLevels,	// MIP info
			0u, mInfo.arrayLayers	// Layers info
		};
	}

	std::optional<command_buffer> image_t::transition_to_layout(std::optional<vk::ImageLayout> aTargetLayout, sync aSyncHandler)
	{
		const auto curLayout = current_layout();
		const auto trgLayout = aTargetLayout.value_or(target_layout());
		mTargetLayout = trgLayout;

		if (curLayout == trgLayout) {
			return {}; // done (:
		}
		if (vk::ImageLayout::eUndefined == trgLayout || vk::ImageLayout::ePreinitialized == trgLayout) {
			AK_LOG_VERBOSE("Won't transition into layout " + vk::to_string(trgLayout));
			return {}; // Won't do it!
		}
		
		// Not done => perform a transition via an image memory barrier inside a command buffer
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(
			pipeline_stage::transfer,	// Just use the transfer stage to create an execution dependency chain
			read_memory_access{memory_access::transfer_read_access}
		);

		// An image's layout is tranformed by the means of an image memory barrier:
		commandBuffer.establish_image_memory_barrier(*this,
			pipeline_stage::transfer, pipeline_stage::transfer,				// Execution dependency chain
			std::optional<memory_access>{}, std::optional<memory_access>{}	// There should be no need to make any memory available or visible... the image should be available already (see above)
		); // establish_image_memory_barrier ^ will set the mCurrentLayout to mTargetLayout

		aSyncHandler.establish_barrier_after_the_operation(
			pipeline_stage::transfer,	// The end of the execution dependency chain
			write_memory_access{memory_access::transfer_write_access}
		);
		return aSyncHandler.submit_and_sync();
	}


	std::optional<command_buffer> image_t::generate_mip_maps(sync aSyncHandler)
	{
		if (config().mipLevels <= 1u) {
			return {};
		}

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{ memory_access::transfer_read_access }); // Make memory visible

		const auto originalLayout = current_layout();
		const auto targetLayout = target_layout();
		auto w = static_cast<int32_t>(width());
		auto h = static_cast<int32_t>(height());

		std::array layoutTransitions = { // during the loop, we'll use 1 or 2 of these
			vk::ImageMemoryBarrier{
				{}, {}, // Memory is available AND already visible for transfer read because that has been established in establish_barrier_before_the_operation above.
				originalLayout, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, 0u, 1u, 0u, 1u }},
			vk::ImageMemoryBarrier{
				{}, vk::AccessFlagBits::eTransferWrite, // This is the first mip-level we're going to write to
				originalLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, 1u, 1u, 0u, 1u }},
			vk::ImageMemoryBarrier{} // To be used in loop
		};

		commandBuffer.handle().pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer, // Can we also use bottom of pipe here??
			vk::DependencyFlags{},
			0u, nullptr,
			0u, nullptr,
			2u /* initially, only 2 required */, layoutTransitions.data()
		);

		for (uint32_t i = 1u; i < config().mipLevels; ++i) {

			commandBuffer.handle().blitImage(
				handle(), vk::ImageLayout::eTransferSrcOptimal,
				handle(), vk::ImageLayout::eTransferDstOptimal,
				{ vk::ImageBlit{
					vk::ImageSubresourceLayers{ mAspectFlags, i-1, 0u, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w      , h      , 1 } },
					vk::ImageSubresourceLayers{ mAspectFlags, i  , 0u, 1u }, { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ w > 1 ? w / 2 : 1, h > 1 ? h / 2 : 1, 1 } }
				  }
				},
				vk::Filter::eLinear
			);

			// mip-level  i-1  is done:
			layoutTransitions[0] = vk::ImageMemoryBarrier{
				{}, {}, // Blit Read -> Done
				vk::ImageLayout::eTransferSrcOptimal, targetLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i-1, 1u, 0u, 1u }};
			// mip-level   i   has been transfer destination, but is going to be transfer source:
			layoutTransitions[1] = vk::ImageMemoryBarrier{
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, // Blit Write -> Blit Read
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i, 1u, 0u, 1u }};
			// mip-level  i+1  is entering the game:
			layoutTransitions[2] = vk::ImageMemoryBarrier{
				{}, vk::AccessFlagBits::eTransferWrite, // make visible to Blit Write
				originalLayout, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, handle(), vk::ImageSubresourceRange{ mAspectFlags, i+1, 1u, 0u, 1u }};

			uint32_t numBarriersRequired = std::min(3u, config().mipLevels - i + 1);
			if (config().mipLevels - 1 == i) {
				layoutTransitions[1].newLayout = targetLayout; // Last one => done
			}
			
			commandBuffer.handle().pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer, // Dependency from previous BLIT to subsequent BLIT
				vk::DependencyFlags{},
				0u, nullptr,
				0u, nullptr,
				numBarriersRequired, layoutTransitions.data()
			);

			w = w > 1 ? w / 2 : 1;
			h = h > 1 ? h / 2 : 1;
		}
		
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{ memory_access::transfer_write_access });
		return aSyncHandler.submit_and_sync();
	}
#pragma endregion

	owning_resource<image_view_t> root::create_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<ak::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), {}, aImageViewUsage, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> root::create_depth_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<ak::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), vk::ImageAspectFlagBits::eDepth, aImageViewUsage, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> root::create_stencil_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat, std::optional<ak::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = std::move(aImageToOwn);

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), vk::ImageAspectFlagBits::eStencil, aImageViewUsage, std::move(aAlterConfigBeforeCreation));
		
		return result;
	}

	owning_resource<image_view_t> root::create_image_view(image_t aImageToWrap, std::optional<vk::Format> aViewFormat, std::optional<ak::image_usage> aImageViewUsage)
	{
		image_view_t result;
		
		// Transfer ownership:
		result.mImage = image_view_t::helper_t( std::move(aImageToWrap) );

		// What's the format of the image view?
		if (!aViewFormat.has_value()) {
			aViewFormat = result.get_image().format();
		}

		finish_configuration(result, aViewFormat.value(), {}, aImageViewUsage, nullptr);
		
		return result;
	}

	void root::finish_configuration(image_view_t& aImageView, vk::Format aViewFormat, std::optional<vk::ImageAspectFlags> aImageAspectFlags, std::optional<ak::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		if (!aImageAspectFlags.has_value()) {
			const auto imageFormat = aImageView.get_image().config().format;
			aImageAspectFlags = aImageView.get_image().aspect_flags();
			
			if (is_depth_format(imageFormat)) {
				if (has_stencil_component(imageFormat)) {
					AK_LOG_ERROR("Can infer whether the image view shall refer to the depth component or to the stencil component => State it explicitly by using image_view_t::create_depth or image_view_t::create_stencil");
				}
				aImageAspectFlags = vk::ImageAspectFlagBits::eDepth;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eStencil rather than only setting eDepth!
			}
			else if(has_stencil_component(imageFormat)) {
				aImageAspectFlags = vk::ImageAspectFlagBits::eStencil;
				// TODO: use vk::ImageAspectFlagBits' underlying type and exclude eDepth rather than only setting eStencil!
			}
		}
		
		// Proceed with config creation (and use the imageAspectFlags there):
		aImageView.mInfo = vk::ImageViewCreateInfo{}
			.setImage(aImageView.get_image().handle())
			.setViewType(to_image_view_type(aImageView.get_image().config()))
			.setFormat(aViewFormat)
			.setComponents(vk::ComponentMapping() // The components field allows you to swizzle the color channels around. In our case we'll stick to the default mapping. [3]
							  .setR(vk::ComponentSwizzle::eIdentity)
							  .setG(vk::ComponentSwizzle::eIdentity)
							  .setB(vk::ComponentSwizzle::eIdentity)
							  .setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange() // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping levels or multiple layers. [3]
				.setAspectMask(aImageAspectFlags.value())
				.setBaseMipLevel(0u)
				.setLevelCount(aImageView.get_image().config().mipLevels)
				.setBaseArrayLayer(0u)
				.setLayerCount(aImageView.get_image().config().arrayLayers));

		if (aImageViewUsage.has_value()) {
			auto [imageUsage, imageLayout, imageTiling, imageCreateFlags] = determine_usage_layout_tiling_flags_based_on_image_usage(aImageViewUsage.value());
			aImageView.mUsageInfo = vk::ImageViewUsageCreateInfo()
				.setUsage(imageUsage);
			aImageView.mInfo.setPNext(&aImageView.mUsageInfo);
		}

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(aImageView);
		}

		aImageView.mImageView = device().createImageViewUnique(aImageView.mInfo);
		aImageView.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(aImageView.handle())
			.setImageLayout(aImageView.get_image().target_layout()); // TODO: Better use the image's current layout or its target layout? 
	}

	owning_resource<sampler_t> root::create_sampler(filter_mode aFilterMode, border_handling_mode aBorderHandlingMode, float aMipMapMaxLod, std::function<void(sampler_t&)> aAlterConfigBeforeCreation)
	{
		vk::Filter magFilter;
		vk::Filter minFilter;
		vk::SamplerMipmapMode mipmapMode;
		vk::Bool32 enableAnisotropy = VK_FALSE;
		float maxAnisotropy = 1.0f;
		switch (aFilterMode)
		{
		case filter_mode::nearest_neighbor:
			magFilter = vk::Filter::eNearest;
			minFilter = vk::Filter::eNearest;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case filter_mode::bilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case filter_mode::trilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			break;
		case filter_mode::cubic: // I have no idea what I'm doing.
			magFilter = vk::Filter::eCubicIMG;
			minFilter = vk::Filter::eCubicIMG;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			break;
		case filter_mode::anisotropic_2x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 2.0f;
			break;
		case filter_mode::anisotropic_4x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 4.0f;
			break;
		case filter_mode::anisotropic_8x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 8.0f;
			break;
		case filter_mode::anisotropic_16x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 16.0f;
			break;
		case filter_mode::anisotropic_32x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 32.0f;
			break;
		case filter_mode::anisotropic_64x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 64.0f;
			break;
		default:
			throw ak::runtime_error("invalid filter_mode");
		}

		// Determine how to handle the borders:
		vk::SamplerAddressMode addressMode;
		switch (aBorderHandlingMode)
		{
		case border_handling_mode::clamp_to_edge:
			addressMode = vk::SamplerAddressMode::eClampToEdge;
			break;
		case border_handling_mode::mirror_clamp_to_edge:
			addressMode = vk::SamplerAddressMode::eMirrorClampToEdge;
			break;
		case border_handling_mode::clamp_to_border:
			addressMode = vk::SamplerAddressMode::eClampToEdge;
			break;
		case border_handling_mode::repeat:
			addressMode = vk::SamplerAddressMode::eRepeat;
			break;
		case border_handling_mode::mirrored_repeat:
			addressMode = vk::SamplerAddressMode::eMirroredRepeat;
			break;
		default:
			throw ak::runtime_error("invalid border_handling_mode");
		}

		// Compile the config for this sampler:
		sampler_t result;
		result.mInfo = vk::SamplerCreateInfo()
			.setMagFilter(magFilter)
			.setMinFilter(minFilter)
			.setAddressModeU(addressMode)
			.setAddressModeV(addressMode)
			.setAddressModeW(addressMode)
			.setAnisotropyEnable(enableAnisotropy)
			.setMaxAnisotropy(maxAnisotropy)
			.setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
			// The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image. 
			// If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range.
			// If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes. Real-world applications almost 
			// always use normalized coordinates, because then it's possible to use textures of varying resolutions with the exact 
			// same coordinates. [4]
			.setUnnormalizedCoordinates(VK_FALSE)
			// If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison 
			// is used in filtering operations. This is mainly used for percentage-closer filtering on shadow maps. [4]
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(mipmapMode)
			.setMipLodBias(0.0f)
			.setMinLod(0.0f)
			.setMaxLod(aMipMapMaxLod);

		// Call custom config function
		if (aAlterConfigBeforeCreation) {
			aAlterConfigBeforeCreation(result);
		}

		result.mSampler = device().createSamplerUnique(result.config());
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setSampler(result.handle());
		result.mDescriptorType = vk::DescriptorType::eSampler;
		return result;
	}

	owning_resource<image_sampler_t> root::create(image_view aImageView, sampler aSampler)
	{
		image_sampler_t result;
		result.mImageView = std::move(aImageView);
		result.mSampler = std::move(aSampler);

		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(result.view_handle())
			.setSampler(result.sampler_handle());
		result.mDescriptorInfo.setImageLayout(result.mImageView->get_image().target_layout());
		
		result.mDescriptorType = vk::DescriptorType::eCombinedImageSampler;
		return result;
	}
	
#pragma region vulkan helper functions
	vk::IndexType to_vk_index_type(size_t aSize)
	{
		if (aSize == sizeof(uint16_t)) {
			return vk::IndexType::eUint16;
		}
		if (aSize == sizeof(uint32_t)) {
			return vk::IndexType::eUint32;
		}
		AK_LOG_ERROR("The given size[" + std::to_string(aSize) + "] does not correspond to a valid vk::IndexType");
		return vk::IndexType::eNoneKHR;
	}

	vk::Bool32 to_vk_bool(bool value)
	{
		return value ? VK_TRUE : VK_FALSE;
	}

	vk::ShaderStageFlagBits to_vk_shader_stage(shader_type aType)
	{
		switch (aType) {
		case ak::shader_type::vertex:
			return vk::ShaderStageFlagBits::eVertex;
		case ak::shader_type::tessellation_control:
			return vk::ShaderStageFlagBits::eTessellationControl;
		case ak::shader_type::tessellation_evaluation:
			return vk::ShaderStageFlagBits::eTessellationEvaluation;
		case ak::shader_type::geometry:
			return vk::ShaderStageFlagBits::eGeometry;
		case ak::shader_type::fragment:
			return vk::ShaderStageFlagBits::eFragment;
		case ak::shader_type::compute:
			return vk::ShaderStageFlagBits::eCompute;
		case ak::shader_type::ray_generation:
			return vk::ShaderStageFlagBits::eRaygenKHR;
		case ak::shader_type::any_hit:
			return vk::ShaderStageFlagBits::eAnyHitKHR;
		case ak::shader_type::closest_hit:
			return vk::ShaderStageFlagBits::eClosestHitKHR;
		case ak::shader_type::miss:
			return vk::ShaderStageFlagBits::eMissKHR;
		case ak::shader_type::intersection:
			return vk::ShaderStageFlagBits::eIntersectionKHR;
		case ak::shader_type::callable:
			return vk::ShaderStageFlagBits::eCallableKHR;
		case ak::shader_type::task:
			return vk::ShaderStageFlagBits::eTaskNV;
		case ak::shader_type::mesh:
			return vk::ShaderStageFlagBits::eMeshNV;
		default:
			throw ak::runtime_error("Invalid shader_type");
		}
	}

	vk::ShaderStageFlags to_vk_shader_stages(shader_type aType)
	{
		vk::ShaderStageFlags result;
		if ((aType & ak::shader_type::vertex) == ak::shader_type::vertex) {
			result |= vk::ShaderStageFlagBits::eVertex;
		}
		if ((aType & ak::shader_type::tessellation_control) == ak::shader_type::tessellation_control) {
			result |= vk::ShaderStageFlagBits::eTessellationControl;
		}
		if ((aType & ak::shader_type::tessellation_evaluation) == ak::shader_type::tessellation_evaluation) {
			result |= vk::ShaderStageFlagBits::eTessellationEvaluation;
		}
		if ((aType & ak::shader_type::geometry) == ak::shader_type::geometry) {
			result |= vk::ShaderStageFlagBits::eGeometry;
		}
		if ((aType & ak::shader_type::fragment) == ak::shader_type::fragment) {
			result |= vk::ShaderStageFlagBits::eFragment;
		}
		if ((aType & ak::shader_type::compute) == ak::shader_type::compute) {
			result |= vk::ShaderStageFlagBits::eCompute;
		}
		if ((aType & ak::shader_type::ray_generation) == ak::shader_type::ray_generation) {
			result |= vk::ShaderStageFlagBits::eRaygenKHR;
		}
		if ((aType & ak::shader_type::any_hit) == ak::shader_type::any_hit) {
			result |= vk::ShaderStageFlagBits::eAnyHitKHR;
		}
		if ((aType & ak::shader_type::closest_hit) == ak::shader_type::closest_hit) {
			result |= vk::ShaderStageFlagBits::eClosestHitKHR;
		}
		if ((aType & ak::shader_type::miss) == ak::shader_type::miss) {
			result |= vk::ShaderStageFlagBits::eMissKHR;
		}
		if ((aType & ak::shader_type::intersection) == ak::shader_type::intersection) {
			result |= vk::ShaderStageFlagBits::eIntersectionKHR;
		}
		if ((aType & ak::shader_type::callable) == ak::shader_type::callable) {
			result |= vk::ShaderStageFlagBits::eCallableKHR;
		}
		if ((aType & ak::shader_type::task) == ak::shader_type::task) {
			result |= vk::ShaderStageFlagBits::eTaskNV;
		}
		if ((aType & ak::shader_type::mesh) == ak::shader_type::mesh) {
			result |= vk::ShaderStageFlagBits::eMeshNV;
		}
		return result;
	}

	vk::VertexInputRate to_vk_vertex_input_rate(input_binding_general_data::kind aValue)
	{
		switch (aValue) {
		case input_binding_general_data::kind::instance:
			return vk::VertexInputRate::eInstance;
		case input_binding_general_data::kind::vertex:
			return vk::VertexInputRate::eVertex;
		default:
			throw std::invalid_argument("Invalid vertex input rate");
		}
	}

	vk::PrimitiveTopology to_vk_primitive_topology(cfg::primitive_topology aValue)
	{
		using namespace cfg;
		
		switch (aValue) {
		case primitive_topology::points:
			return vk::PrimitiveTopology::ePointList;
		case primitive_topology::lines: 
			return vk::PrimitiveTopology::eLineList;
		case primitive_topology::line_strip:
			return vk::PrimitiveTopology::eLineStrip;
		case primitive_topology::triangles: 
			return vk::PrimitiveTopology::eTriangleList;
		case primitive_topology::triangle_strip:
			return vk::PrimitiveTopology::eTriangleStrip;
		case primitive_topology::triangle_fan: 
			return vk::PrimitiveTopology::eTriangleFan;
		case primitive_topology::lines_with_adjacency:
			return vk::PrimitiveTopology::eLineListWithAdjacency;
		case primitive_topology::line_strip_with_adjacency: 
			return vk::PrimitiveTopology::eLineStripWithAdjacency;
		case primitive_topology::triangles_with_adjacency: 
			return vk::PrimitiveTopology::eTriangleListWithAdjacency;
		case primitive_topology::triangle_strip_with_adjacency: 
			return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
		case primitive_topology::patches: 
			return vk::PrimitiveTopology::ePatchList;
		default:
			throw std::invalid_argument("Invalid primitive topology");
		}
	}

	vk::PolygonMode to_vk_polygon_mode(cfg::polygon_drawing_mode aValue)
	{
		using namespace cfg;
		
		switch (aValue) {
		case polygon_drawing_mode::fill: 
			return vk::PolygonMode::eFill;
		case polygon_drawing_mode::line:
			return vk::PolygonMode::eLine;
		case polygon_drawing_mode::point:
			return vk::PolygonMode::ePoint;
		default:
			throw std::invalid_argument("Invalid polygon drawing mode.");
		}
	}

	vk::CullModeFlags to_vk_cull_mode(cfg::culling_mode aValue)
	{
		using namespace cfg;
		
		switch (aValue) {
		case culling_mode::disabled:
			return vk::CullModeFlagBits::eNone;
		case culling_mode::cull_front_faces:
			return vk::CullModeFlagBits::eFront;
		case culling_mode::cull_back_faces:
			return vk::CullModeFlagBits::eBack;
		case culling_mode::cull_front_and_back_faces:
			return vk::CullModeFlagBits::eFrontAndBack;
		default:
			throw std::invalid_argument("Invalid culling mode.");
		}
	}

	vk::FrontFace to_vk_front_face(cfg::winding_order aValue)
	{
		using namespace cfg;
		
		switch (aValue) {
		case winding_order::counter_clockwise:
			return vk::FrontFace::eCounterClockwise;
		case winding_order::clockwise:
			return vk::FrontFace::eClockwise;
		default:
			throw std::invalid_argument("Invalid front face winding order.");
		}
	}

	vk::CompareOp to_vk_compare_op(cfg::compare_operation aValue)
	{
		using namespace cfg;
		
		switch(aValue) {
		case compare_operation::never:
			return vk::CompareOp::eNever;
		case compare_operation::less: 
			return vk::CompareOp::eLess;
		case compare_operation::equal: 
			return vk::CompareOp::eEqual;
		case compare_operation::less_or_equal: 
			return vk::CompareOp::eLessOrEqual;
		case compare_operation::greater: 
			return vk::CompareOp::eGreater;
		case compare_operation::not_equal: 
			return vk::CompareOp::eNotEqual;
		case compare_operation::greater_or_equal: 
			return vk::CompareOp::eGreaterOrEqual;
		case compare_operation::always: 
			return vk::CompareOp::eAlways;
		default:
			throw std::invalid_argument("Invalid compare operation.");
		}
	}

	vk::ColorComponentFlags to_vk_color_components(cfg::color_channel aValue)
	{
		using namespace cfg;
		
		switch (aValue)	{
		case color_channel::none:
			return vk::ColorComponentFlags{};
		case color_channel::red:
			return vk::ColorComponentFlagBits::eR;
		case color_channel::green:
			return vk::ColorComponentFlagBits::eG;
		case color_channel::blue:
			return vk::ColorComponentFlagBits::eB;
		case color_channel::alpha:
			return vk::ColorComponentFlagBits::eA;
		case color_channel::rg:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG;
		case color_channel::rgb:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
		case color_channel::rgba:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		default:
			throw std::invalid_argument("Invalid color channel value.");
		}
	}

	vk::BlendFactor to_vk_blend_factor(cfg::blending_factor aValue)
	{
		using namespace cfg;
		
		switch (aValue) {
		case blending_factor::zero:
			return vk::BlendFactor::eZero;
		case blending_factor::one: 
			return vk::BlendFactor::eOne;
		case blending_factor::source_color: 
			return vk::BlendFactor::eSrcColor;
		case blending_factor::one_minus_source_color: 
			return vk::BlendFactor::eOneMinusSrcColor;
		case blending_factor::destination_color: 
			return vk::BlendFactor::eDstColor;
		case blending_factor::one_minus_destination_color: 
			return vk::BlendFactor::eOneMinusDstColor;
		case blending_factor::source_alpha: 
			return vk::BlendFactor::eSrcAlpha;
		case blending_factor::one_minus_source_alpha: 
			return vk::BlendFactor::eOneMinusSrcAlpha;
		case blending_factor::destination_alpha: 
			return vk::BlendFactor::eDstAlpha;
		case blending_factor::one_minus_destination_alpha:
			return vk::BlendFactor::eOneMinusDstAlpha;
		case blending_factor::constant_color: 
			return vk::BlendFactor::eConstantColor;
		case blending_factor::one_minus_constant_color: 
			return vk::BlendFactor::eOneMinusConstantColor;
		case blending_factor::constant_alpha: 
			return vk::BlendFactor::eConstantAlpha;
		case blending_factor::one_minus_constant_alpha: 
			return vk::BlendFactor::eOneMinusConstantAlpha;
		case blending_factor::source_alpha_saturate: 
			return vk::BlendFactor::eSrcAlphaSaturate;
		default:
			throw std::invalid_argument("Invalid blend factor value.");
		}
	}

	vk::BlendOp to_vk_blend_operation(cfg::color_blending_operation aValue)
	{
		using namespace cfg;
		
		switch (aValue)
		{
		case color_blending_operation::add: 
			return vk::BlendOp::eAdd;
		case color_blending_operation::subtract: 
			return vk::BlendOp::eSubtract;
		case color_blending_operation::reverse_subtract: 
			return vk::BlendOp::eReverseSubtract;
		case color_blending_operation::min: 
			return vk::BlendOp::eMin;
		case color_blending_operation::max: 
			return vk::BlendOp::eMax;
		default:
			throw std::invalid_argument("Invalid color blending operation.");
		}
	}

	vk::LogicOp to_vk_logic_operation(cfg::blending_logic_operation aValue)
	{
		using namespace cfg;
		
		switch (aValue)
		{
		case blending_logic_operation::op_clear:
			return vk::LogicOp::eClear;
		case blending_logic_operation::op_and: 
			return vk::LogicOp::eAnd;
		case blending_logic_operation::op_and_reverse: 
			return vk::LogicOp::eAndReverse;
		case blending_logic_operation::op_copy: 
			return vk::LogicOp::eCopy;
		case blending_logic_operation::op_and_inverted: 
			return vk::LogicOp::eAndInverted;
		case blending_logic_operation::no_op: 
			return vk::LogicOp::eNoOp;
		case blending_logic_operation::op_xor: 
			return vk::LogicOp::eXor;
		case blending_logic_operation::op_or: 
			return vk::LogicOp::eOr;
		case blending_logic_operation::op_nor: 
			return vk::LogicOp::eNor;
		case blending_logic_operation::op_equivalent: 
			return vk::LogicOp::eEquivalent;
		case blending_logic_operation::op_invert: 
			return vk::LogicOp::eInvert;
		case blending_logic_operation::op_or_reverse: 
			return vk::LogicOp::eOrReverse;
		case blending_logic_operation::op_copy_inverted: 
			return vk::LogicOp::eCopyInverted;
		case blending_logic_operation::op_or_inverted: 
			return vk::LogicOp::eOrInverted;
		case blending_logic_operation::op_nand: 
			return vk::LogicOp::eNand;
		case blending_logic_operation::op_set: 
			return vk::LogicOp::eSet;
		default: 
			throw std::invalid_argument("Invalid blending logic operation.");
		}
	}

	vk::AttachmentLoadOp to_vk_load_op(on_load aValue)
	{
		switch (aValue) {
		case on_load::dont_care:
			return vk::AttachmentLoadOp::eDontCare;
		case on_load::clear: 
			return vk::AttachmentLoadOp::eClear;
		case on_load::load: 
			return vk::AttachmentLoadOp::eLoad;
		default:
			throw std::invalid_argument("Invalid attachment load operation.");
		}
	}

	vk::AttachmentStoreOp to_vk_store_op(on_store aValue)
	{
		switch (aValue) {
		case on_store::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
		case on_store::store:
		case on_store::store_in_presentable_format:
			return vk::AttachmentStoreOp::eStore;
		default:
			throw std::invalid_argument("Invalid attachment store operation.");
		}
	}

	vk::PipelineStageFlags to_vk_pipeline_stage_flags(ak::pipeline_stage aValue)
	{
		vk::PipelineStageFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (ak::is_included(aValue, ak::pipeline_stage::top_of_pipe					)) { result |= vk::PipelineStageFlagBits::eTopOfPipe					; }
		if (ak::is_included(aValue, ak::pipeline_stage::draw_indirect					)) { result |= vk::PipelineStageFlagBits::eDrawIndirect					; }
		if (ak::is_included(aValue, ak::pipeline_stage::vertex_input					)) { result |= vk::PipelineStageFlagBits::eVertexInput					; }
		if (ak::is_included(aValue, ak::pipeline_stage::vertex_shader					)) { result |= vk::PipelineStageFlagBits::eVertexShader					; }
		if (ak::is_included(aValue, ak::pipeline_stage::tessellation_control_shader	)) { result |= vk::PipelineStageFlagBits::eTessellationControlShader	; }
		if (ak::is_included(aValue, ak::pipeline_stage::tessellation_evaluation_shader)) { result |= vk::PipelineStageFlagBits::eTessellationEvaluationShader	; }
		if (ak::is_included(aValue, ak::pipeline_stage::geometry_shader				)) { result |= vk::PipelineStageFlagBits::eGeometryShader				; }
		if (ak::is_included(aValue, ak::pipeline_stage::fragment_shader				)) { result |= vk::PipelineStageFlagBits::eFragmentShader				; }
		if (ak::is_included(aValue, ak::pipeline_stage::early_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eEarlyFragmentTests			; }
		if (ak::is_included(aValue, ak::pipeline_stage::late_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eLateFragmentTests			; }
		if (ak::is_included(aValue, ak::pipeline_stage::color_attachment_output		)) { result |= vk::PipelineStageFlagBits::eColorAttachmentOutput		; }
		if (ak::is_included(aValue, ak::pipeline_stage::compute_shader				)) { result |= vk::PipelineStageFlagBits::eComputeShader				; }
		if (ak::is_included(aValue, ak::pipeline_stage::transfer						)) { result |= vk::PipelineStageFlagBits::eTransfer						; }
		if (ak::is_included(aValue, ak::pipeline_stage::bottom_of_pipe				)) { result |= vk::PipelineStageFlagBits::eBottomOfPipe					; }
		if (ak::is_included(aValue, ak::pipeline_stage::host							)) { result |= vk::PipelineStageFlagBits::eHost							; }
		if (ak::is_included(aValue, ak::pipeline_stage::all_graphics			)) { result |= vk::PipelineStageFlagBits::eAllGraphics					; }
		if (ak::is_included(aValue, ak::pipeline_stage::all_commands					)) { result |= vk::PipelineStageFlagBits::eAllCommands					; }
		if (ak::is_included(aValue, ak::pipeline_stage::transform_feedback			)) { result |= vk::PipelineStageFlagBits::eTransformFeedbackEXT			; }
		if (ak::is_included(aValue, ak::pipeline_stage::conditional_rendering			)) { result |= vk::PipelineStageFlagBits::eConditionalRenderingEXT		; }
#if VK_HEADER_VERSION >= 135
		if (ak::is_included(aValue, ak::pipeline_stage::command_preprocess			)) { result |= vk::PipelineStageFlagBits::eCommandPreprocessNV			; }
#else 
		if (ak::is_included(aValue, ak::pipeline_stage::command_preprocess			)) { result |= vk::PipelineStageFlagBits::eCommandProcessNVX			; }
#endif
		if (ak::is_included(aValue, ak::pipeline_stage::shading_rate_image			)) { result |= vk::PipelineStageFlagBits::eShadingRateImageNV			; }
		if (ak::is_included(aValue, ak::pipeline_stage::ray_tracing_shaders			)) { result |= vk::PipelineStageFlagBits::eRayTracingShaderKHR			; }
		if (ak::is_included(aValue, ak::pipeline_stage::acceleration_structure_build	)) { result |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR; }
		if (ak::is_included(aValue, ak::pipeline_stage::task_shader					)) { result |= vk::PipelineStageFlagBits::eTaskShaderNV					; }
		if (ak::is_included(aValue, ak::pipeline_stage::mesh_shader					)) { result |= vk::PipelineStageFlagBits::eMeshShaderNV					; }
		if (ak::is_included(aValue, ak::pipeline_stage::fragment_density_process		)) { result |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT	; }
		return result;
	}
	
	vk::PipelineStageFlags to_vk_pipeline_stage_flags(std::optional<ak::pipeline_stage> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_pipeline_stage_flags(aValue.value());
		}
		return vk::PipelineStageFlags{};
	}

	vk::AccessFlags to_vk_access_flags(ak::memory_access aValue)
	{
		vk::AccessFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (ak::is_included(aValue, ak::memory_access::indirect_command_data_read_access			)) { result |= vk::AccessFlagBits::eIndirectCommandRead; }
		if (ak::is_included(aValue, ak::memory_access::index_buffer_read_access					)) { result |= vk::AccessFlagBits::eIndexRead; }
		if (ak::is_included(aValue, ak::memory_access::vertex_buffer_read_access					)) { result |= vk::AccessFlagBits::eVertexAttributeRead; }
		if (ak::is_included(aValue, ak::memory_access::uniform_buffer_read_access					)) { result |= vk::AccessFlagBits::eUniformRead; }
		if (ak::is_included(aValue, ak::memory_access::input_attachment_read_access				)) { result |= vk::AccessFlagBits::eInputAttachmentRead; }
		if (ak::is_included(aValue, ak::memory_access::shader_buffers_and_images_read_access		)) { result |= vk::AccessFlagBits::eShaderRead; }
		if (ak::is_included(aValue, ak::memory_access::shader_buffers_and_images_write_access		)) { result |= vk::AccessFlagBits::eShaderWrite; }
		if (ak::is_included(aValue, ak::memory_access::color_attachment_read_access				)) { result |= vk::AccessFlagBits::eColorAttachmentRead; }
		if (ak::is_included(aValue, ak::memory_access::color_attachment_write_access				)) { result |= vk::AccessFlagBits::eColorAttachmentWrite; }
		if (ak::is_included(aValue, ak::memory_access::depth_stencil_attachment_read_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentRead; }
		if (ak::is_included(aValue, ak::memory_access::depth_stencil_attachment_write_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentWrite; }
		if (ak::is_included(aValue, ak::memory_access::transfer_read_access						)) { result |= vk::AccessFlagBits::eTransferRead; }
		if (ak::is_included(aValue, ak::memory_access::transfer_write_access						)) { result |= vk::AccessFlagBits::eTransferWrite; }
		if (ak::is_included(aValue, ak::memory_access::host_read_access							)) { result |= vk::AccessFlagBits::eHostRead; }
		if (ak::is_included(aValue, ak::memory_access::host_write_access							)) { result |= vk::AccessFlagBits::eHostWrite; }
		if (ak::is_included(aValue, ak::memory_access::any_read_access							)) { result |= vk::AccessFlagBits::eMemoryRead; }
		if (ak::is_included(aValue, ak::memory_access::any_write_access					 		)) { result |= vk::AccessFlagBits::eMemoryWrite; }
		if (ak::is_included(aValue, ak::memory_access::transform_feedback_write_access			)) { result |= vk::AccessFlagBits::eTransformFeedbackWriteEXT; }
		if (ak::is_included(aValue, ak::memory_access::transform_feedback_counter_read_access		)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterReadEXT; }
		if (ak::is_included(aValue, ak::memory_access::transform_feedback_counter_write_access	)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT; }
		if (ak::is_included(aValue, ak::memory_access::conditional_rendering_predicate_read_access)) { result |= vk::AccessFlagBits::eConditionalRenderingReadEXT; }
#if VK_HEADER_VERSION >= 135
		if (ak::is_included(aValue, ak::memory_access::command_preprocess_read_access				)) { result |= vk::AccessFlagBits::eCommandPreprocessReadNV; }
		if (ak::is_included(aValue, ak::memory_access::command_preprocess_write_access			)) { result |= vk::AccessFlagBits::eCommandPreprocessWriteNV; }
#else
		if (ak::is_included(aValue, ak::memory_access::command_preprocess_read_access				)) { result |= vk::AccessFlagBits::eCommandProcessReadNVX; }
		if (ak::is_included(aValue, ak::memory_access::command_preprocess_write_access			)) { result |= vk::AccessFlagBits::eCommandProcessWriteNVX; }
#endif
		if (ak::is_included(aValue, ak::memory_access::color_attachment_noncoherent_read_access	)) { result |= vk::AccessFlagBits::eColorAttachmentReadNoncoherentEXT; }
		if (ak::is_included(aValue, ak::memory_access::shading_rate_image_read_access				)) { result |= vk::AccessFlagBits::eShadingRateImageReadNV; }
		if (ak::is_included(aValue, ak::memory_access::acceleration_structure_read_access			)) { result |= vk::AccessFlagBits::eAccelerationStructureReadKHR; }
		if (ak::is_included(aValue, ak::memory_access::acceleration_structure_write_access		)) { result |= vk::AccessFlagBits::eAccelerationStructureWriteKHR; }
		if (ak::is_included(aValue, ak::memory_access::fragment_density_map_attachment_read_access)) { result |= vk::AccessFlagBits::eFragmentDensityMapReadEXT; }

		return result;
	}

	vk::AccessFlags to_vk_access_flags(std::optional<ak::memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_access_flags(aValue.value());
		}
		return vk::AccessFlags{};
	}

	ak::memory_access to_memory_access(ak::read_memory_access aValue)
	{
		return static_cast<ak::memory_access>(aValue);
	}
	
	std::optional<ak::memory_access> to_memory_access(std::optional<ak::read_memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_memory_access(aValue.value());
		}
		return {};
	}
	
	ak::memory_access to_memory_access(ak::write_memory_access aValue)
	{
		return static_cast<ak::memory_access>(aValue);
	}
	
	std::optional<ak::memory_access> to_memory_access(std::optional<ak::write_memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_memory_access(aValue.value());
		}
		return {};
	}

	ak::filter_mode to_cgb_filter_mode(float aVulkanAnisotropy, bool aMipMappingAvailable)
	{
		if (aMipMappingAvailable) {
			if (aVulkanAnisotropy > 1.0f) {
				if (std::fabs(aVulkanAnisotropy - 16.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_16x;
				}
				if (std::fabs(aVulkanAnisotropy - 8.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_8x;
				}
				if (std::fabs(aVulkanAnisotropy - 4.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_4x;
				}
				if (std::fabs(aVulkanAnisotropy - 2.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_2x;
				}
				if (std::fabs(aVulkanAnisotropy - 32.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_32x;
				}
				if (std::fabs(aVulkanAnisotropy - 64.0f) <= std::numeric_limits<float>::epsilon()) {
					return ak::filter_mode::anisotropic_64x;
				}
				AK_LOG_WARNING("Encountered a strange anisotropy value of " + std::to_string(aVulkanAnisotropy));
			}
			return ak::filter_mode::trilinear;
		}
		return ak::filter_mode::bilinear;
	}

	vk::ImageViewType to_image_view_type(const vk::ImageCreateInfo& info)
	{
		switch (info.imageType)
		{
		case vk::ImageType::e1D:
			if (info.arrayLayers > 1) {
				return vk::ImageViewType::e1DArray;
			}
			else {
				return vk::ImageViewType::e1D;
			}
		case vk::ImageType::e2D:
			if (info.arrayLayers > 1) {
				return vk::ImageViewType::e2DArray;
			}
			else {
				return vk::ImageViewType::e2D;
			}
		case vk::ImageType::e3D:
			return vk::ImageViewType::e3D;
		}
		throw new ak::runtime_error("It might be that the implementation of to_image_view_type(const vk::ImageCreateInfo& info) is incomplete. Please complete it!");
	}
#pragma endregion
}