#pragma once

#include <vector>
#include <deque>
#include <array>
#include <string>
#include <string_view>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>
#include <variant>
#include <iomanip>
#include <optional>
#include <typeinfo>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <cassert>

#include <cpplinq.hpp>

#include <ak/ak_log.hpp>
#include <ak/ak_error.hpp>
#include <ak/cpp_utils.hpp>

#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.hpp>

#include <ak/image_color_channel_order.hpp>
#include <ak/image_color_channel_format.hpp>
#include <ak/image_usage.hpp>
#include <ak/filter_mode.hpp>
#include <ak/border_handling_mode.hpp>
#include <ak/vk_utils.hpp>

#include <ak/memory_access.hpp>
#include <ak/memory_usage.hpp>
#include <ak/on_load.hpp>
#include <ak/on_store.hpp>
#include <ak/usage_type.hpp>
#include <ak/usage_desc.hpp>

#include <ak/shader_type.hpp>
#include <ak/shader_info.hpp>
#include <ak/aabb.hpp>
#include <ak/pipeline_stage.hpp>

#include <ak/semaphore.hpp>
#include <ak/fence.hpp>
#include <ak/descriptor_cache_interface.hpp>
#include <ak/command_buffer.hpp>
#include <ak/sync.hpp>
#include <ak/image.hpp>
#include <ak/image_view.hpp>
#include <ak/sampler.hpp>
#include <ak/image_sampler.hpp>
#include <ak/attachment.hpp>

#include <ak/buffer_meta.hpp>
#include <ak/buffer_declaration.hpp>

#include <ak/input_description.hpp>

#include <ak/push_constants.hpp>

#include <ak/command_pool.hpp>

#include <ak/buffer.hpp>
#include <ak/buffer_view.hpp>
#include <ak/queue.hpp>
#include <ak/renderpass_sync.hpp>
#include <ak/renderpass.hpp>
#include <ak/framebuffer.hpp>

#include <ak/geometry_instance.hpp>
#include <ak/acceleration_structure_size_requirements.hpp>
#include <ak/bottom_level_acceleration_structure.hpp>
#include <ak/top_level_acceleration_structure.hpp>
#include <ak/shader.hpp>
#include <ak/descriptor_alloc_request.hpp>
#include <ak/descriptor_pool.hpp>
#include <ak/descriptor_set_layout.hpp>
#include <ak/set_of_descriptor_set_layouts.hpp>
#include <ak/descriptor_set.hpp>
#include <ak/standard_descriptor_cache.hpp>
#include <ak/binding_data.hpp>
#include <ak/pipeline_settings.hpp>
#include <ak/graphics_pipeline_config.hpp>
#include <ak/compute_pipeline_config.hpp>
#include <ak/ray_tracing_pipeline_config.hpp>
#include <ak/graphics_pipeline.hpp>
#include <ak/compute_pipeline.hpp>
#include <ak/ray_tracing_pipeline.hpp>
#include <ak/shader_binding_table.hpp>

#include <ak/vulkan_helper_functions.hpp>

#include <ak/bindings.hpp>

#include <ak/ray_tracing_pipeline_config_convenience_functions.hpp>

#include <ak/commands.hpp>
#include <ak/vk_utils2.hpp>

namespace ak
{
	// T must provide:
	//    .physical_device()			returning a vk::PhysicalDevice&
	//    .device()						returning a vk::Device&
	//    .queue()						returning a vk::Queue&
	//    .queue_family_index()         returning a uint32_t
	//    .dynamic_dispatch()			returning a vk::DispatchLoaderDynamic&
	//    .command_pool_for_flags()     returning a vk::CommandPool&
	//    .descriptor_cache()           returning a descriptor_cache_interface&
	class root
	{
	public:
		root() {}
		virtual vk::PhysicalDevice& physical_device()		                                      = 0;
		virtual vk::Device& device()						                                      = 0;
		virtual vk::Queue& queue()							                                      = 0;
		virtual uint32_t queue_family_index()                                                     = 0;
		virtual vk::DispatchLoaderDynamic& dynamic_dispatch()                                     = 0;
		virtual vk::CommandPool& command_pool_for_flags(vk::CommandPoolCreateFlags aCreateFlags)  = 0;
		virtual descriptor_cache_interface& descriptor_cache()                                    = 0;

#pragma region root helper functions
		/** Find (index of) memory with parameters
		 *	@param aMemoryTypeBits		Bit field of the memory types that are suitable for the buffer. [9]
		 *	@param aMemoryProperties	Special features of the memory, like being able to map it so we can write to it from the CPU. [9]
		 */
		uint32_t find_memory_type_index(uint32_t aMemoryTypeBits, vk::MemoryPropertyFlags aMemoryProperties);

		bool is_format_supported(vk::Format pFormat, vk::ImageTiling pTiling, vk::FormatFeatureFlags aFormatFeatures);

		// Helper function used for creating both, bottom level and top level acceleration structures
		template <typename T>
		void finish_acceleration_structure_creation(T& result, std::function<void(T&)> aAlterConfigBeforeMemoryAlloc) noexcept
		{
			// ------------- Memory ------------
			// 5. Query memory requirements
			{
				auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
					.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject)
					.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
					.setAccelerationStructure(result.acceleration_structure_handle());
				result.mMemoryRequirementsForAccelerationStructure = device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, dynamic_dispatch());
			}
			{
				auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
					.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch)
					.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
					.setAccelerationStructure(result.acceleration_structure_handle());
				result.mMemoryRequirementsForBuildScratchBuffer = device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, dynamic_dispatch());
			}
			{
				auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
					.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eUpdateScratch)
					.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
					.setAccelerationStructure(result.acceleration_structure_handle());
				result.mMemoryRequirementsForScratchBufferUpdate = device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, dynamic_dispatch());
			}

			// 6. Assemble memory info
			result.mMemoryAllocateInfo = vk::MemoryAllocateInfo{}
				.setAllocationSize(result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.size)
				.setMemoryTypeIndex(find_memory_type_index(
					result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.memoryTypeBits,
					vk::MemoryPropertyFlagBits::eDeviceLocal)); // TODO: Does it make sense to support other memory locations as eDeviceLocal?

			// 7. Maybe alter the config?
			if (aAlterConfigBeforeMemoryAlloc) {
				aAlterConfigBeforeMemoryAlloc(result);
			}

			// 8. Allocate the memory
			result.mMemory = device().allocateMemoryUnique(result.mMemoryAllocateInfo);

			// 9. Bind memory to the acceleration structure
			auto memBindInfo = vk::BindAccelerationStructureMemoryInfoKHR{}
				.setAccelerationStructure(result.acceleration_structure_handle())
				.setMemory(result.memory_handle())
				.setMemoryOffset(0) // TODO: support memory offsets
				.setDeviceIndexCount(0) // TODO: What is this?
				.setPDeviceIndices(nullptr);
			device().bindAccelerationStructureMemoryKHR({ memBindInfo }, dynamic_dispatch());

			// 10. Get an "opaque" handle which can be used on the device
			auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR{}
				.setAccelerationStructure(result.acceleration_structure_handle());

			result.mDynamicDispatch = dynamic_dispatch();
			result.mDeviceAddress = device().getAccelerationStructureAddressKHR(&addressInfo, dynamic_dispatch());
		}

		vk::PhysicalDeviceRayTracingPropertiesKHR get_ray_tracing_properties();

		vk::DeviceAddress get_buffer_address(vk::Buffer aBufferHandle);

		void finish_configuration(buffer_view_t& aBufferViewToBeFinished, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation);
#pragma endregion

#pragma region bottom level acceleration structure
		ak::owning_resource<bottom_level_acceleration_structure_t> create_bottom_level_acceleration_structure(std::vector<ak::acceleration_structure_size_requirements> aGeometryDescriptions, bool aAllowUpdates, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation = {}, std::function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc = {});
#pragma endregion 

#pragma region buffer
		/**	Create a buffer which is always created with exclusive access for a queue.
		*	If different queues are being used, ownership has to be transferred explicitly.
		*/
		template <typename Meta>
		ak::owning_resource<buffer_t<Meta>> create_buffer(const Meta& aMetaData, vk::BufferUsageFlags aBufferUsage, vk::MemoryPropertyFlags aMemoryProperties, vk::MemoryAllocateFlags aMemoryAllocateFlags, std::optional<vk::DescriptorType> aDescriptorType)
		{
			auto bufferSize = aMetaData.total_size();

			// Create (possibly multiple) buffer(s):
			auto bufferCreateInfo = vk::BufferCreateInfo()
				.setSize(static_cast<vk::DeviceSize>(bufferSize))
				.setUsage(aBufferUsage)
				// Always grant exclusive ownership to the queue.
				.setSharingMode(vk::SharingMode::eExclusive)
				// The flags parameter is used to configure sparse buffer memory, which is not relevant right now. We'll leave it at the default value of 0. [2]
				.setFlags(vk::BufferCreateFlags()); 

			// Create the buffer on the logical device
			auto vkBuffer = device().createBufferUnique(bufferCreateInfo);

			// The buffer has been created, but it doesn't actually have any memory assigned to it yet. 
			// The first step of allocating memory for the buffer is to query its memory requirements [2]
			const auto memRequirements = device().getBufferMemoryRequirements(vkBuffer.get());

			auto allocInfo = vk::MemoryAllocateInfo()
				.setAllocationSize(memRequirements.size)
				.setMemoryTypeIndex(find_memory_type_index(
					memRequirements.memoryTypeBits, 
					aMemoryProperties));

			auto allocateFlagsInfo = vk::MemoryAllocateFlagsInfo{};
			if (aMemoryAllocateFlags) {
				allocateFlagsInfo.setFlags(aMemoryAllocateFlags);
				allocInfo.setPNext(&allocateFlagsInfo);
			}

			// Allocate the memory for the buffer:
			auto vkMemory = device().allocateMemoryUnique(allocInfo);

			// If memory allocation was successful, then we can now associate this memory with the buffer
			device().bindBufferMemory(vkBuffer.get(), vkMemory.get(), 0);
			// TODO: if(!succeeded) { throw ak::runtime_error("Binding memory to buffer failed."); }

			buffer_t<buffer_meta> b;
			b.mMetaData = static_cast<buffer_meta>(aMetaData);;
			b.mCreateInfo = bufferCreateInfo;
			b.mMemoryPropertyFlags = aMemoryProperties;
			b.mMemory = std::move(vkMemory);
			b.mBufferUsageFlags = aBufferUsage;
			b.mBuffer = std::move(vkBuffer);

			if (ak::has_flag(b.buffer_usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddress) || ak::has_flag(b.buffer_usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressKHR) || ak::has_flag(b.buffer_usage_flags(), vk::BufferUsageFlagBits::eShaderDeviceAddressEXT)) {
				b.mDeviceAddress = get_buffer_address(b.buffer_handle());
			}
			
			return std::move(b);
		}
			
		// CREATE 
		template <typename Meta, typename... Metas>
		ak::owning_resource<buffer_t<buffer_meta>> create_buffer(
			Meta aConfig, Metas... aConfigs,
			ak::memory_usage aMemoryUsage,
			vk::BufferUsageFlags aUsage = vk::BufferUsageFlags())
		{
			assert(((aConfig.total_size() == aConfigs.total_size()) && ...));
			auto bufferSize = aConfig.total_size();
			std::optional<vk::DescriptorType> descriptorType = {};
			vk::MemoryPropertyFlags memoryFlags;
			vk::MemoryAllocateFlags memoryAllocateFlags;

			// We've got two major branches here: 
			// 1) Memory will stay on the host and there will be no dedicated memory on the device
			// 2) Memory will be transfered to the device. (Only in this case, we'll need to make use of sync.)
			switch (aMemoryUsage)
			{
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
				aUsage |= vk::BufferUsageFlagBits::eTransferDst; 
				break;
			case ak::memory_usage::device_readback:
				memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
				aUsage |= vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
				break;
			case ak::memory_usage::device_protected:
				memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eProtected;
				aUsage |= vk::BufferUsageFlagBits::eTransferDst;
				break;
			}

			// If buffer was created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR bit set, memory must have been allocated with the 
			// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR bit set. The Vulkan spec states: If the VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress
			// feature is enabled and buffer was created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT bit set, memory must have been allocated with the
			// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT bit set
			if (ak::has_flag(aUsage, vk::BufferUsageFlagBits::eShaderDeviceAddress) || ak::has_flag(aUsage, vk::BufferUsageFlagBits::eShaderDeviceAddressKHR) || ak::has_flag(aUsage, vk::BufferUsageFlagBits::eShaderDeviceAddressEXT)) {
				memoryAllocateFlags |= vk::MemoryAllocateFlagBits::eDeviceAddress;
			}
			
			aUsage |= aConfig.buffer_usage_flags();
			if constexpr (sizeof...(aConfigs) > 0) {
				aUsage |= (... | aConfigs);
			}

			// Create buffer here to make use of named return value optimization.
			// How it will be filled depends on where the memory is located at.
			return create_buffer(aConfig, aUsage, memoryFlags, memoryAllocateFlags, descriptorType);
		}
#pragma endregion

#pragma region buffer view 
		owning_resource<buffer_view_t> create_buffer_view(ak::uniform_texel_buffer aBufferToOwn, std::optional<vk::Format> aViewFormat = {}, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<buffer_view_t> create_buffer_view(ak::storage_texel_buffer aBufferToOwn, std::optional<vk::Format> aViewFormat = {}, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<buffer_view_t> create_buffer_view(vk::Buffer aBufferToReference, vk::BufferCreateInfo aBufferInfo, vk::Format aViewFormat, std::function<void(buffer_view_t&)> aAlterConfigBeforeCreation = {});
#pragma endregion

#pragma region command pool and command buffer
		command_pool create_command_pool(vk::CommandPoolCreateFlags aCreateFlags = vk::CommandPoolCreateFlags());

		std::vector<ak::owning_resource<command_buffer_t>> create_command_buffers(uint32_t aCount, vk::CommandPoolCreateFlags aCommandPoolFlags, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel = vk::CommandBufferLevel::ePrimary);
			
		ak::owning_resource<command_buffer_t> create_command_buffer(vk::CommandPoolCreateFlags aCommandPoolFlags, vk::CommandBufferUsageFlags aUsageFlags, vk::CommandBufferLevel aLevel = vk::CommandBufferLevel::ePrimary);

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		command_buffer create_command_buffer(bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		std::vector<command_buffer> create_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 */
		command_buffer create_single_use_command_buffer(bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 *	@param	aNumBuffers					How many command buffers to be created.
		 */
		std::vector<command_buffer> create_single_use_command_buffers(uint32_t aNumBuffers, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		command_buffer create_resettable_command_buffer(bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		std::vector<command_buffer> create_resettable_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);
#pragma endregion 

#pragma region compute pipeline
		owning_resource<compute_pipeline_t> create_compute_pipeline(compute_pipeline_config aConfig, std::function<void(compute_pipeline_t&)> aAlterConfigBeforeCreation = {});

		/**	Convenience function for gathering the compute pipeline's configuration.
		 *
		 *	It supports the following types
		 *		- ak::cfg::pipeline_settings
		 *		- ak::shader_info or std::string_view
		 *		- ak::binding_data
		 *		- ak::push_constant_binding_data
		 *		- ak::context_specific_function<void(compute_pipeline_t&)>
		 *
		 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref compute_pipeline_t::create
		 */
		template <typename... Ts>
		ak::owning_resource<compute_pipeline_t> create_compute_pipeline_for(Ts... args)
		{
			// 1. GATHER CONFIG
			std::function<void(compute_pipeline_t&)> alterConfigFunction;
			compute_pipeline_config config;
			add_config(config, alterConfigFunction, std::move(args)...);

			// 2. CREATE PIPELINE according to the config
			// ============================================ Vk ============================================ 
			//    => VULKAN CODE HERE:
			return create_compute_pipeline(std::move(config), std::move(alterConfigFunction));
			// ============================================================================================ 
		}
#pragma endregion

#pragma region descriptor pool
		std::shared_ptr<descriptor_pool> create_descriptor_pool(const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets);
#pragma endregion

#pragma region descriptor set layout and set of descriptor set layouts
		void allocate_descriptor_set_layout(descriptor_set_layout& aLayoutToBeAllocated);
		void allocate_descriptor_set_layouts(set_of_descriptor_set_layouts& aLayoutsToBeAllocated);
#pragma endregion

#pragma region descriptor set
		std::vector<descriptor_set> get_or_create_descriptor_sets(std::initializer_list<binding_data> aBindings);
#pragma endregion

#pragma region fence
		owning_resource<fence_t> create_fence(bool aCreateInSignalledState = false, std::function<void(fence_t&)> aAlterConfigBeforeCreation = {});
#pragma endregion

#pragma region framebuffer
		// Helper methods for the create methods that take attachments and image views
		void check_and_config_attachments_based_on_views(std::vector<attachment>& aAttachments, std::vector<image_view>& aImageViews);
		
		owning_resource<framebuffer_t> create_framebuffer(renderpass aRenderpass, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<framebuffer_t> create_framebuffer(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<framebuffer_t> create_framebuffer(renderpass aRenderpass, std::vector<image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<framebuffer_t> create_framebuffer(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, std::function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});

		template <typename ...ImViews>
		ak::owning_resource<framebuffer_t> create_framebuffer(std::vector<ak::attachment> aAttachments, ImViews... aImViews)
		{
			std::vector<image_view> imageViews;
			(imageViews.push_back(std::move(aImViews)), ...);
			return create_framebuffer(std::move(aAttachments), std::move(imageViews));
		}

		template <typename ...ImViews>
		ak::owning_resource<framebuffer_t> create_framebuffer(ak::renderpass aRenderpass, ImViews... aImViews)
		{
			std::vector<image_view> imageViews;
			(imageViews.push_back(std::move(aImViews)), ...);
			return create_framebuffer(std::move(aRenderpass), std::move(imageViews));
		}
#pragma endregion

#pragma region graphics pipeline
		ak::owning_resource<graphics_pipeline_t> create_graphics_pipeline(graphics_pipeline_config aConfig, std::function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation = {});

		/**	Convenience function for gathering the graphic pipeline's configuration.
		 *	
		 *	It supports the following types 
		 *
		 *
		 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref graphics_pipeline_t::create
		 */
		template <typename... Ts>
		ak::owning_resource<graphics_pipeline_t> create_graphics_pipeline_for(Ts... args)
		{
			// 1. GATHER CONFIG
			std::vector<ak::attachment> renderPassAttachments;
			std::function<void(graphics_pipeline_t&)> alterConfigFunction;
			graphics_pipeline_config config;
			add_config(config, renderPassAttachments, alterConfigFunction, std::move(args)...);

			// Check if render pass attachments are in renderPassAttachments XOR config => only in that case, it is clear how to proceed, fail in other cases
			if (renderPassAttachments.size() > 0 == (config.mRenderPassSubpass.has_value() && nullptr != std::get<renderpass>(*config.mRenderPassSubpass)->handle())) {
				if (renderPassAttachments.size() == 0) {
					throw ak::runtime_error("No renderpass config provided! Please provide a renderpass or attachments!");
				}
				throw ak::runtime_error("Ambiguous renderpass config! Either set a renderpass XOR provide attachments!");
			}
			// ^ that was the sanity check. See if we have to build the renderpass from the attachments:
			if (renderPassAttachments.size() > 0) {
				add_config(config, renderPassAttachments, alterConfigFunction, renderpass_t::create(std::move(renderPassAttachments)));
			}

			// 2. CREATE PIPELINE according to the config
			// ============================================ Vk ============================================ 
			//    => VULKAN CODE HERE:
			return create_graphics_pipeline(std::move(config), std::move(alterConfigFunction));
			// ============================================================================================ 
		}
#pragma endregion

#pragma region image
		/** Creates a new image
		 *	@param	aWidth						The width of the image to be created
		 *	@param	aHeight						The height of the image to be created
		 *	@param	aFormatAndSamples			The image format and the number of samples of the image to be created
		 *	@param	aMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		 *	@param	aImageUsage					How this image is intended to being used.
		 *	@param	aNumLayers					How many layers the image to be created shall contain.
		 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		 *	@return	Returns a newly created image.
		 */
		owning_resource<image_t> create_image(uint32_t aWidth, uint32_t aHeight, std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, int aNumLayers = 1, memory_usage aMemoryUsage = memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_image, std::function<void(image_t&)> aAlterConfigBeforeCreation = {});
		
		/** Creates a new image
		 *	@param	aWidth						The width of the image to be created
		 *	@param	aHeight						The height of the image to be created
		 *	@param	aFormat						The image format of the image to be created
		 *	@param	aMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		 *	@param	aImageUsage					How this image is intended to being used.
		 *	@param	aNumLayers					How many layers the image to be created shall contain.
		 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		 *	@return	Returns a newly created image.
		 */
		owning_resource<image_t> create_image(uint32_t aWidth, uint32_t aHeight, vk::Format aFormat, int aNumLayers = 1, memory_usage aMemoryUsage = memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_image, std::function<void(image_t&)> aAlterConfigBeforeCreation = {});

		/** Creates a new image
		*	@param	aWidth						The width of the depth buffer to be created
		*	@param	aHeight						The height of the depth buffer to be created
		*	@param	aFormat						The image format of the image to be created, or a default depth format if not specified.
		*	@param	aMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		*	@param	aNumLayers					How many layers the image to be created shall contain.
		*	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created depth buffer.
		*/
		owning_resource<image_t> create_depth_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat = std::nullopt, int aNumLayers = 1,  memory_usage aMemoryUsage = memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_depth_stencil_attachment, std::function<void(image_t&)> aAlterConfigBeforeCreation = {});

		/** Creates a new image
		*	@param	aWidth						The width of the depth+stencil buffer to be created
		*	@param	aHeight						The height of the depth+stencil buffer to be created
		*	@param	aFormat						The image format of the image to be created, or a default depth format if not specified.
		*	@param	aMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		*	@param	aNumLayers					How many layers the image to be created shall contain.
		*	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created depth+stencil buffer.
		*/
		owning_resource<image_t> create_depth_stencil_image(uint32_t aWidth, uint32_t aHeight, std::optional<vk::Format> aFormat = std::nullopt, int aNumLayers = 1,  memory_usage aMemoryUsage = memory_usage::device, ak::image_usage aImageUsage = ak::image_usage::general_depth_stencil_attachment, std::function<void(image_t&)> aAlterConfigBeforeCreation = {});

		image_t wrap_image(vk::Image aImageToWrap, vk::ImageCreateInfo aImageCreateInfo, ak::image_usage aImageUsage, vk::ImageAspectFlags aImageAspectFlags);
#pragma endregion
		
		/** Creates a new image view upon a given image
		*	@param	aImageToOwn					The image which to create an image view for
		*	@param	aViewFormat					The format of the image view. If none is specified, it will be set to the same format as the image.
		*	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageViewCreateInfo` just before the image view will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created image.
		*/
		owning_resource<image_view_t> create_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat = std::nullopt, std::optional<ak::image_usage> aImageViewUsage = {}, std::function<void(image_view_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<image_view_t> create_depth_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat = std::nullopt, std::optional<ak::image_usage> aImageViewUsage = {}, std::function<void(image_view_t&)> aAlterConfigBeforeCreation = {});
		owning_resource<image_view_t> create_stencil_image_view(image aImageToOwn, std::optional<vk::Format> aViewFormat = std::nullopt, std::optional<ak::image_usage> aImageViewUsage = {}, std::function<void(image_view_t&)> aAlterConfigBeforeCreation = {});

		owning_resource<image_view_t> create_image_view(image_t aImageToWrap, std::optional<vk::Format> aViewFormat = std::nullopt, std::optional<ak::image_usage> aImageViewUsage = {});

		void finish_configuration(image_view_t& aImageView, vk::Format aViewFormat, std::optional<vk::ImageAspectFlags> aImageAspectFlags, std::optional<ak::image_usage> aImageViewUsage, std::function<void(image_view_t&)> aAlterConfigBeforeCreation);

		/**	Create a new sampler with the given configuration parameters
		 *	@param	aFilterMode					Filtering strategy for the sampler to be created
		 *	@param	aBorderHandlingMode			Border handling strategy for the sampler to be created
		 *	@param	aMipMapMaxLod				Default value = house number
		 *	@param	aAlterConfigBeforeCreation	A context-specific function which allows to alter the configuration before the sampler is created.
		 */
		owning_resource<sampler_t> create_sampler(filter_mode aFilterMode, border_handling_mode aBorderHandlingMode, float aMipMapMaxLod = 20.0f, std::function<void(sampler_t&)> aAlterConfigBeforeCreation = {});

		owning_resource<image_sampler_t> create(image_view aImageView, sampler aSampler);


	};
}