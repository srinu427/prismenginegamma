#include "PrismRenderer.h"

#include <set>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static void delegate_gen_final_cmd_bufs(PrismRenderer* renderer, uint32_t frameNo) {
	renderer->genFinalCmdBuffers(frameNo);
}

void PrismRenderer::getVkInstance()
{
	//Struct with app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Prism App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Prism Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	instance = vkutils::createVKInstance(
		appInfo,
		enableValidationLayers,
		validationLayers
	);

	if (enableValidationLayers) debugMessenger = vkutils::setupDebugMessenger(instance);
}

void PrismRenderer::createSurface() {
	if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void PrismRenderer::getVkLogicalDevice()
{
	physicalDevice = vkutils::pickPhysicalDevice(instance, surface, deviceExtensions, true);
	queueFamilyIndices = vkutils::findQueueFamilies(physicalDevice, surface);

	//Struct to create required Queues
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() , queueFamilyIndices.transferFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	//Struct to create logical device
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceVulkan11Features deviceFeatures11{};
	deviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	deviceFeatures11.shaderDrawParameters = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pNext = &deviceFeatures11;

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}
	//Creating the logical device
	if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	//Get queue handles
	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.transferFamily.value(), 0, &transferQueue);
}

void PrismRenderer::createSwapChain(SwapChainSupportDetails swapChainSupport)
{
	VkSurfaceFormatKHR surfaceFormat = vkutils::chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = vkutils::chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = vkutils::chooseSwapExtent(window, swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//Struct to create swap chain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t qfindices[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.transferFamily.value() };

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily && queueFamilyIndices.graphicsFamily != queueFamilyIndices.transferFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 3;
		createInfo.pQueueFamilyIndices = qfindices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = NULL; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//Create swapchain
	if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	frameDatas.resize(imageCount);
	MAX_FRAMES_IN_FLIGHT = imageCount;

	std::vector<VkImage> swapChainImages = std::vector<VkImage>(frameDatas.size());
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	for (size_t i = 0; i < imageCount; i++) {
		frameDatas[i].swapChainImage = vkutils::createGPUImage(
			device,
			swapChainImages[i],
			VK_IMAGE_VIEW_TYPE_2D,
			swapChainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
}

void PrismRenderer::createDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 200 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 200 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 40;

	if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void PrismRenderer::addDsetLayout(std::string name, uint32_t binding, VkDescriptorType dType, uint32_t dCount, VkShaderStageFlags stageFlag)
{
	dSetLayouts[name] = vkutils::createDescriptorSetLayout(device, binding, dType, dCount, stageFlag);
}

void PrismRenderer::makeBasicDSetLayouts()
{
	addDsetLayout("vert_uniform", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
	addDsetLayout("vert_storage", 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
	addDsetLayout("frag_uniform", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	addDsetLayout("frag_sampler_1", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	addDsetLayout("frag_sampler_3", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, VK_SHADER_STAGE_FRAGMENT_BIT);
	addDsetLayout("frag_sampler_5", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, VK_SHADER_STAGE_FRAGMENT_BIT);
	addDsetLayout("vert_frag_uniform", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);
	addDsetLayout("frag_plight_sampler", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(MAX_POINT_LIGHTS), VK_SHADER_STAGE_FRAGMENT_BIT);
	addDsetLayout("frag_dlight_sampler", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(MAX_DIRECTIONAL_LIGHTS), VK_SHADER_STAGE_FRAGMENT_BIT);
}

void PrismRenderer::makeBasicCmdPools()
{
	for (int i = 0; i < frameDatas.size(); i++) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		if (vkCreateCommandPool(device, &poolInfo, NULL, &frameDatas[i].commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
	poolInfo.flags = 0; // Optional
	if (vkCreateCommandPool(device, &poolInfo, NULL, &uploadCmdPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void PrismRenderer::createDepthImage()
{
	depthFormat = vkutils::findDepthFormat(physicalDevice);
	depthImage = vkutils::createGPUImage(
		device,
		physicalDevice,
		swapChainExtent.width, swapChainExtent.height,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);
}

void PrismRenderer::makeDLightMaps()
{
	lights.resize(MAX_NS_LIGHTS + MAX_DIRECTIONAL_LIGHTS + MAX_POINT_LIGHTS);
	for (uint32_t i = 0; i < frameDatas.size(); i++) {
		for (uint32_t j = 0; j < MAX_DIRECTIONAL_LIGHTS; j++) {
			GPUImage gimg = vkutils::createGPUImage(
				device,
				physicalDevice,
				dlight_smap_extent.width, dlight_smap_extent.height,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			vkutils::transitionImageLayout(
				device,
				uploadCmdPool,
				transferQueue,
				gimg._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
			);
			frameDatas[i].shadow_dir_maps.push_back(gimg);
		}
		frameDatas[i].shadow_dir_dset = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_dlight_sampler"],
			frameDatas[i].shadow_dir_maps,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear_clamped"]
		);
		frameDatas[i].dShadowMapTemp = vkutils::createGPUImage(
			device,
			physicalDevice,
			dlight_smap_extent.width, dlight_smap_extent.height,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].dShadowMapTemp._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);
		frameDatas[i].dShadowDepthImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			dlight_smap_extent.width, dlight_smap_extent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_DEPTH_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].dShadowDepthImage._image,
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
		);
	}
}

void PrismRenderer::makePLightMaps()
{
	//lights.resize(MAX_POINT_LIGHTS);
	for (uint32_t i = 0; i < frameDatas.size(); i++) {
		for (uint32_t j = 0; j < MAX_POINT_LIGHTS; j++) {
			GPUImage gimg = vkutils::createGPUImage(
				device,
				physicalDevice,
				plight_smap_extent.width, plight_smap_extent.height,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				(uint32_t)6,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_VIEW_TYPE_CUBE,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			vkutils::transitionImageLayout(
				device,
				uploadCmdPool,
				transferQueue,
				gimg._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
			);
			frameDatas[i].shadow_cube_maps.push_back(gimg);
		}
		frameDatas[i].shadow_cube_dset = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_plight_sampler"],
			frameDatas[i].shadow_cube_maps,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear"]
		);
		frameDatas[i].pShadowMapTemp = vkutils::createGPUImage(
			device,
			physicalDevice,
			plight_smap_extent.width, plight_smap_extent.height,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].pShadowMapTemp._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);
		frameDatas[i].pShadowDepthImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			plight_smap_extent.width, plight_smap_extent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_DEPTH_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].pShadowDepthImage._image,
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
		);
	}
}

void PrismRenderer::createShadowRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_R32_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;


	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &shadowRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shadow cube render pass!");
	}
}

void PrismRenderer::createGbufferRenderPass() {
	VkAttachmentDescription gbufferAttachments[4];
	VkAttachmentReference gbufferAttachmentsRef[4] = { {} };

	for (size_t ai = 0; ai < 4; ai++) {
		gbufferAttachments[ai].flags = 0;
		gbufferAttachments[ai].samples = VK_SAMPLE_COUNT_1_BIT;
		gbufferAttachments[ai].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		gbufferAttachments[ai].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		gbufferAttachments[ai].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		gbufferAttachments[ai].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		//gbufferAttachments[ai].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		gbufferAttachments[ai].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		gbufferAttachmentsRef[ai].attachment = ai;
		gbufferAttachmentsRef[ai].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	gbufferAttachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	gbufferAttachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	gbufferAttachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	gbufferAttachments[3].format = VK_FORMAT_R8G8B8A8_UNORM;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 4;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 4;
	subpass.pColorAttachments = gbufferAttachmentsRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency subpassDependencies[2] = { {} };
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkAttachmentDescription attachments[5];
	attachments[0] = gbufferAttachments[0];
	attachments[1] = gbufferAttachments[1];
	attachments[2] = gbufferAttachments[2];
	attachments[3] = gbufferAttachments[3];
	attachments[4] = depthAttachment;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 5;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = subpassDependencies;

	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &gbufferRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create gbuffer render pass!");
	}
}

void PrismRenderer::createAmbientRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_R8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = NULL;

	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = uint32_t(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &ambientRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void PrismRenderer::createFinalRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = NULL;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &finalRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void PrismRenderer::makeBasicDSets()
{
	for (size_t i = 0; i < frameDatas.size(); i++) {
		frameDatas[i].setBuffers["scene"] = vkutils::createSetBuffer(
			device,
			physicalDevice,
			sizeof(GPUSceneData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			descriptorPool,
			dSetLayouts["frag_uniform"],
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		);
		frameDatas[i].setBuffers["light"] = vkutils::createSetBuffer(
			device,
			physicalDevice,
			sizeof(GPULight) * (MAX_NS_LIGHTS + MAX_DIRECTIONAL_LIGHTS + MAX_POINT_LIGHTS),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			descriptorPool,
			dSetLayouts["vert_frag_uniform"],
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		);
		frameDatas[i].setBuffers["camera"] = vkutils::createSetBuffer(
			device,
			physicalDevice,
			sizeof(GPUCameraData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			descriptorPool,
			dSetLayouts["vert_frag_uniform"],
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		);
		frameDatas[i].setBuffers["object"] = vkutils::createSetBuffer(
			device,
			physicalDevice,
			sizeof(GPUObjectData) * MAX_OBJECTS,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			descriptorPool,
			dSetLayouts["vert_storage"],
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		);
		frameDatas[i].positionDset = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_sampler_1"],
			{frameDatas[i].positionImage},
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear"]
		);
		frameDatas[i].normalImageDset = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_sampler_1"],
			{ frameDatas[i].normalImage },
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear"]
		);
		frameDatas[i].finalComposeDset = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_sampler_5"],
			{ frameDatas[i].colorImage, frameDatas[i].positionImage, frameDatas[i].normalImage, frameDatas[i].seImage, frameDatas[i].ambientImage },
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear"]
		);
	}
}

void PrismRenderer::addSimplePipeline(std::string name, VkRenderPass rPass, std::unordered_map<VkShaderStageFlagBits, std::string> stage_shader_map, std::vector<std::string> reqDSetLayouts, VkOffset2D scissorOffset, VkExtent2D scissorExtent, uint32_t VPWidth, uint32_t VPHeight, bool invert_VP_Y, std::vector<VkPushConstantRange> pushConstantRanges)
{
	GPUPipeline gPipeline;
	std::vector<VkShaderModule> shaders;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
	for (auto it : stage_shader_map) {
		VkShaderModule shader = vkutils::createShader(device, it.second);
		shaders.push_back(shader);
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = it.first;
		shaderStageInfo.module = shader;
		shaderStageInfo.pName = "main";
		shaderStageInfos.push_back(shaderStageInfo);
	}

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	if (invert_VP_Y) {
		viewport.x = 0.0f;
		viewport.y = float(VPHeight);
		viewport.width = float(VPWidth);
		viewport.height = -float(VPHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}
	else {
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = float(VPWidth);
		viewport.height = float(VPHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}

	VkRect2D scissor{};
	scissor.offset = scissorOffset;
	scissor.extent = scissorExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> pipelineSetLayouts;
	for (std::string dsl_name : reqDSetLayouts) {
		auto it = dSetLayouts.find(dsl_name);
		if (it == dSetLayouts.end()) { std::runtime_error("Cannot find the DSet Layout " + dsl_name); }
		else pipelineSetLayouts.push_back(dSetLayouts[dsl_name]);
	}
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = uint32_t(pipelineSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = pipelineSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = uint32_t(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &gPipeline._pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = uint32_t(shaderStageInfos.size());
	pipelineInfo.pStages = shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional
	pipelineInfo.layout = gPipeline._pipelineLayout;
	pipelineInfo.renderPass = rPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!"); }

	pipelines[name] = gPipeline;

	for (VkShaderModule smod : shaders) vkDestroyShaderModule(device, smod, NULL);
}

void PrismRenderer::makeDLightShadowPipeline()
{
	addSimplePipeline(
		"dlight_smap",
		shadowRenderPass,
		{ {VK_SHADER_STAGE_VERTEX_BIT , "shaders/light_smap.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT , "shaders/light_smap.frag.spv"} },
		{ "vert_frag_uniform", "vert_storage" },
		{ 0, 0 }, dlight_smap_extent,
		dlight_smap_extent.width, dlight_smap_extent.height,
		true,
		{ {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPULightPC)} }
	);
}

void PrismRenderer::makePLightShadowPipeline()
{
	addSimplePipeline(
		"plight_smap",
		shadowRenderPass,
		{ {VK_SHADER_STAGE_VERTEX_BIT , "shaders/light_smap.vert.spv"}, {VK_SHADER_STAGE_FRAGMENT_BIT , "shaders/light_smap.frag.spv"} },
		{ "vert_frag_uniform", "vert_storage" },
		{ 0, 0 }, plight_smap_extent,
		plight_smap_extent.width, plight_smap_extent.height,
		true,
		{ {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPULightPC)} }
	);
}

void PrismRenderer::makeGbufferPipeline() {
	GPUPipeline gPipeline;
	VkShaderModule shaders[2];
	VkPipelineShaderStageCreateInfo shaderStageInfos[2] = { {} };

	shaders[0] = vkutils::createShader(device, "shaders/gbuffer.vert.spv");
	shaders[1] = vkutils::createShader(device, "shaders/gbuffer.frag.spv");
	
	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].module = shaders[0];
	shaderStageInfos[0].pName = "main";
	shaderStageInfos[0].pNext = NULL;
	shaderStageInfos[0].pSpecializationInfo = NULL;
	shaderStageInfos[0].flags = 0;

	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].module = shaders[1];
	shaderStageInfos[1].pName = "main";
	shaderStageInfos[1].pNext = NULL;
	shaderStageInfos[1].pSpecializationInfo = NULL;
	shaderStageInfos[1].flags = 0;


	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};

	viewport.x = 0.0f;
	viewport.y = float(swapChainExtent.height);
	viewport.width = float(swapChainExtent.width);
	viewport.height = -float(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachments[4] = { {} };
	for (size_t cbai = 0; cbai < 4; cbai++) {
		colorBlendAttachments[cbai].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachments[cbai].blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 4;
	colorBlending.pAttachments = colorBlendAttachments;
	colorBlending.pNext = NULL;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.pNext = NULL;

	
	std::vector<VkDescriptorSetLayout> pipelineSetLayouts;
	pipelineSetLayouts.push_back(dSetLayouts["vert_frag_uniform"]);
	pipelineSetLayouts.push_back(dSetLayouts["vert_storage"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_sampler_3"]);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = uint32_t(pipelineSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = pipelineSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pNext = NULL;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &gPipeline._pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStageInfos;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional
	pipelineInfo.layout = gPipeline._pipelineLayout;
	pipelineInfo.renderPass = gbufferRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.pNext = NULL;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	pipelines["gbuffer"] = gPipeline;

	for (VkShaderModule smod : shaders) vkDestroyShaderModule(device, smod, NULL);
}

void PrismRenderer::makeAmbientPipeline() {
	GPUPipeline gPipeline;
	VkShaderModule shaders[2];
	VkPipelineShaderStageCreateInfo shaderStageInfos[2] = { {} };

	shaders[0] = vkutils::createShader(device, "shaders/screensize.vert.spv");
	shaders[1] = vkutils::createShader(device, "shaders/ambient.frag.spv");

	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].module = shaders[0];
	shaderStageInfos[0].pName = "main";

	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].module = shaders[1];
	shaderStageInfos[1].pName = "main";


	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};

	viewport.x = 0.0f;
	viewport.y = float(swapChainExtent.height);
	viewport.width = float(swapChainExtent.width);
	viewport.height = -float(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachments[1] = { {} };

	colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachments[0].blendEnable = VK_FALSE;


	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = colorBlendAttachments;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> pipelineSetLayouts;
	pipelineSetLayouts.push_back(dSetLayouts["frag_sampler_1"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_sampler_1"]);
	pipelineSetLayouts.push_back(dSetLayouts["vert_frag_uniform"]);

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = uint32_t(pipelineSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = pipelineSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &gPipeline._pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStageInfos;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional
	pipelineInfo.layout = gPipeline._pipelineLayout;
	pipelineInfo.renderPass = ambientRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	pipelines["ambient"] = gPipeline;

	for (VkShaderModule smod : shaders) vkDestroyShaderModule(device, smod, NULL);
}

void PrismRenderer::makeFinalPipeline()
{
	GPUPipeline gPipeline;
	VkShaderModule shaders[2];
	VkPipelineShaderStageCreateInfo shaderStageInfos[2] = { {} };

	shaders[0] = vkutils::createShader(device, "shaders/screensize.vert.spv");
	shaders[1] = vkutils::createShader(device, "shaders/finalimage.frag.spv");

	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].module = shaders[0];
	shaderStageInfos[0].pName = "main";

	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].module = shaders[1];
	shaderStageInfos[1].pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};

	viewport.x = 0.0f;
	viewport.y = 0;
	viewport.width = float(swapChainExtent.width);
	viewport.height = float(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> pipelineSetLayouts;
	pipelineSetLayouts.push_back(dSetLayouts["vert_frag_uniform"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_uniform"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_sampler_5"]);
	pipelineSetLayouts.push_back(dSetLayouts["vert_frag_uniform"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_plight_sampler"]);
	pipelineSetLayouts.push_back(dSetLayouts["frag_dlight_sampler"]);

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = uint32_t(pipelineSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = pipelineSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &gPipeline._pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStageInfos;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = NULL; // Optional
	pipelineInfo.layout = gPipeline._pipelineLayout;
	pipelineInfo.renderPass = finalRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	pipelines["final_image"] = gPipeline;

	for (VkShaderModule smod : shaders) vkDestroyShaderModule(device, smod, NULL);
}

void PrismRenderer::createShadowFrameBuffers()
{
	for (size_t i = 0; i < frameDatas.size(); i++) {
		std::vector<GPUImage> sfbattachments = { frameDatas[i].pShadowMapTemp, frameDatas[i].pShadowDepthImage };
		frameDatas[i].pShadowFrameBuffer = vkutils::createFrameBuffer(
			device,
			shadowRenderPass,
			sfbattachments,
			plight_smap_extent.width, plight_smap_extent.height,
			1
		);
		sfbattachments = { frameDatas[i].dShadowMapTemp, frameDatas[i].dShadowDepthImage };
		frameDatas[i].dShadowFrameBuffer = vkutils::createFrameBuffer(
			device,
			shadowRenderPass,
			sfbattachments,
			dlight_smap_extent.width, dlight_smap_extent.height,
			1
		);
	}
}

void PrismRenderer::createGbufferFrameBuffers()
{
	for (size_t i = 0; i < frameDatas.size(); i++) {
		frameDatas[i].colorImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			swapChainExtent.width, swapChainExtent.height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].colorImage._image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		frameDatas[i].positionImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			swapChainExtent.width, swapChainExtent.height,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].positionImage._image,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		frameDatas[i].normalImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			swapChainExtent.width, swapChainExtent.height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].normalImage._image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		frameDatas[i].seImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			swapChainExtent.width, swapChainExtent.height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		vkutils::transitionImageLayout(
			device,
			uploadCmdPool,
			transferQueue,
			frameDatas[i].seImage._image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		std::vector<GPUImage> attachments = { frameDatas[i].colorImage, frameDatas[i].positionImage, frameDatas[i].normalImage, frameDatas[i].seImage, depthImage };
		frameDatas[i].gbufferFrameBuffer = vkutils::createFrameBuffer(
			device,
			gbufferRenderPass,
			attachments,
			swapChainExtent.width, swapChainExtent.height,
			1
		);
	}
}

void PrismRenderer::createAmbientFrameBuffers()
{
	for (size_t i = 0; i < frameDatas.size(); i++) {
		frameDatas[i].ambientImage = vkutils::createGPUImage(
			device,
			physicalDevice,
			swapChainExtent.width, swapChainExtent.height,
			VK_FORMAT_R8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		std::vector<GPUImage> attachments = { frameDatas[i].ambientImage };
		frameDatas[i].ambientFrameBuffer = vkutils::createFrameBuffer(
			device,
			ambientRenderPass,
			attachments,
			swapChainExtent.width, swapChainExtent.height,
			1
		);
	}
}

void PrismRenderer::createFinalFrameBuffers()
{
	for (size_t i = 0; i < frameDatas.size(); i++) {
		std::vector<GPUImage> attachments = { frameDatas[i].swapChainImage };
		frameDatas[i].swapChainFrameBuffer = vkutils::createFrameBuffer(
			device,
			finalRenderPass,
			attachments,
			swapChainExtent.width, swapChainExtent.height,
			1
		);
	}
}

void PrismRenderer::makeIndirectCmdBuffer()
{
}

void PrismRenderer::addDLightCmds(VkCommandBuffer cmdBuffer, size_t frameNo)
{
	std::vector<VkClearValue> clearValues = std::vector<VkClearValue>(2);
	clearValues[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	GPUPipeline dLightPipeline = pipelines["dlight_smap"];

	for (size_t lidx = 0; lidx < MAX_DIRECTIONAL_LIGHTS; lidx++) {
		if (!(lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + lidx].flags.x & 1)) continue;
		GPULightPC tlpc;
		tlpc.idx.x = int(MAX_NS_LIGHTS + MAX_POINT_LIGHTS + lidx);
		tlpc.viewproj = glm::mat4(1);

		vkutils::beginRenderPass(shadowRenderPass, frameDatas[frameNo].dShadowFrameBuffer, dlight_smap_extent, cmdBuffer, clearValues);
		dLightPipeline.bindPipeline(cmdBuffer);
		dLightPipeline.bindPipelineDSets(
			cmdBuffer,
			{ frameDatas[frameNo].setBuffers["light"]._dSet, frameDatas[frameNo].setBuffers["object"]._dSet },
			{ { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPULightPC)}, &tlpc } }
		);
		size_t robjCount = renderObjects.size();
		for (size_t ro_idx = 0; ro_idx < robjCount; ro_idx++) {
			RenderObject robj = renderObjects[ro_idx];
			if (!robj.renderable || !robj.shadowcasting) continue;

			VkBuffer vertexBuffers[] = { (robj.maintained_mesh) ? robj.mmesh->_vertexBuffer[frameNo]._buffer : robj.mesh->_vertexBuffer._buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, (robj.maintained_mesh) ? robj.mmesh->_indexBuffer[frameNo]._buffer : robj.mesh->_indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);

			robj.drawMesh(cmdBuffer, ro_idx);
		}
		vkCmdEndRenderPass(cmdBuffer);

		vkutils::transitionImageLayout(
			cmdBuffer,
			frameDatas[frameNo].shadow_dir_maps[lidx]._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);
		vkutils::transitionImageLayout(
			cmdBuffer,
			frameDatas[frameNo].dShadowMapTemp._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);

		VkImageCopy copyRegion = {};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width = dlight_smap_extent.width;
		copyRegion.extent.height = dlight_smap_extent.height;
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(
			cmdBuffer,
			frameDatas[frameNo].dShadowMapTemp._image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			frameDatas[frameNo].shadow_dir_maps[lidx]._image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion
		);

		vkutils::transitionImageLayout(
			cmdBuffer,
			frameDatas[frameNo].shadow_dir_maps[lidx]._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);
		vkutils::transitionImageLayout(
			cmdBuffer,
			frameDatas[frameNo].dShadowMapTemp._image,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);
	}

}

void PrismRenderer::addPLightCmds(VkCommandBuffer cmdBuffer, size_t frameNo)
{
	std::vector<VkClearValue> clearValues = std::vector<VkClearValue>(2);
	clearValues[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	GPUPipeline pLightPipeline = pipelines["plight_smap"];
	// 90 degree proj mat
	glm::mat4 projMatrix = glm::perspective(glm::radians(90.0f), float(plight_smap_extent.width) / float(plight_smap_extent.height), 0.01f, 1000.0f);

	for (uint32_t lidx = 0; lidx < MAX_POINT_LIGHTS; lidx++) {
		if (!(lights[MAX_NS_LIGHTS + lidx].flags.x & 1)) continue;
		for (uint32_t face = 0; face < 6; face++) {
			glm::mat4 viewMatrix = glm::mat4(1);

			switch (face)
			{
			case 0: // POSITIVE_X
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
				break;
			case 1:	// NEGATIVE_X
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
				break;
			case 2:	// POSITIVE_Y
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			case 3:	// NEGATIVE_Y
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
				break;
			case 4:	// POSITIVE_Z
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
				break;
			case 5:	// NEGATIVE_Z
				viewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
				break;
			}
			GPULightPC tlpc;
			tlpc.idx.x = lidx;
			tlpc.viewproj = projMatrix * viewMatrix;
			vkutils::beginRenderPass(shadowRenderPass, frameDatas[frameNo].pShadowFrameBuffer, plight_smap_extent, cmdBuffer, clearValues);
			pLightPipeline.bindPipeline(cmdBuffer);
			pLightPipeline.bindPipelineDSets(
				cmdBuffer,
				{ frameDatas[frameNo].setBuffers["light"]._dSet, frameDatas[frameNo].setBuffers["object"]._dSet },
				{ { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPULightPC)}, &tlpc } }
			);
			size_t robjCount = renderObjects.size();
			for (size_t ro_idx = 0; ro_idx < robjCount; ro_idx++) {
				RenderObject robj = renderObjects[ro_idx];
				if (!robj.renderable || !robj.shadowcasting) continue;

				VkBuffer vertexBuffers[] = { (robj.maintained_mesh) ? robj.mmesh->_vertexBuffer[frameNo]._buffer : robj.mesh->_vertexBuffer._buffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, (robj.maintained_mesh) ? robj.mmesh->_indexBuffer[frameNo]._buffer :robj.mesh->_indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
				robj.drawMesh(cmdBuffer, ro_idx);
			}
			vkCmdEndRenderPass(cmdBuffer);

			VkImageSubresourceRange cubeFaceSubresourceRange = {};
			cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			cubeFaceSubresourceRange.baseMipLevel = 0;
			cubeFaceSubresourceRange.levelCount = 1;
			cubeFaceSubresourceRange.baseArrayLayer = face;
			cubeFaceSubresourceRange.layerCount = 1;
			vkutils::transitionImageLayout(
				cmdBuffer,
				frameDatas[frameNo].shadow_cube_maps[lidx]._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				cubeFaceSubresourceRange
			);
			vkutils::transitionImageLayout(
				cmdBuffer,
				frameDatas[frameNo].pShadowMapTemp._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
			);

			VkImageCopy copyRegion = {};
			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = face;
			copyRegion.dstSubresource.mipLevel = 0;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = plight_smap_extent.width;
			copyRegion.extent.height = plight_smap_extent.height;
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				cmdBuffer,
				frameDatas[frameNo].pShadowMapTemp._image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				frameDatas[frameNo].shadow_cube_maps[lidx]._image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion
			);

			vkutils::transitionImageLayout(
				cmdBuffer,
				frameDatas[frameNo].shadow_cube_maps[lidx]._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				cubeFaceSubresourceRange
			);
			vkutils::transitionImageLayout(
				cmdBuffer,
				frameDatas[frameNo].pShadowMapTemp._image,
				VK_FORMAT_R32_SFLOAT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
			);
		}
	}
}

void PrismRenderer::addGbufferCmds(VkCommandBuffer cmdBuffer, size_t frameNo)
{
	std::vector<VkClearValue> clearValues = std::vector<VkClearValue>(5);
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[4].depthStencil = { 1.0f, 0 };

	vkutils::beginRenderPass(gbufferRenderPass, frameDatas[frameNo].gbufferFrameBuffer, swapChainExtent, cmdBuffer, clearValues);

	size_t robjCount = renderObjects.size();

	GPUPipeline gbuffer_pipeline = pipelines["gbuffer"];
	gbuffer_pipeline.bindPipeline(cmdBuffer);

	for (size_t ro_idx = 0; ro_idx < robjCount; ro_idx++) {
		RenderObject robj = renderObjects[ro_idx];
		if (!robj.renderable) continue;

		VkBuffer vertexBuffers[] = { (robj.maintained_mesh) ? robj.mmesh->_vertexBuffer[frameNo]._buffer : robj.mesh->_vertexBuffer._buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, (robj.maintained_mesh) ? robj.mmesh->_indexBuffer[frameNo]._buffer : robj.mesh->_indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
		gbuffer_pipeline.bindPipelineDSets(
			cmdBuffer,
			{
				frameDatas[frameNo].setBuffers["camera"]._dSet,
				frameDatas[frameNo].setBuffers["object"]._dSet,
				robj.texmaps->_dSet,
			},
			{}
		);
		robj.drawMesh(cmdBuffer, ro_idx);
	}
	vkCmdEndRenderPass(cmdBuffer);
}

void PrismRenderer::addAmbientCmds(VkCommandBuffer cmdBuffer, size_t frameNo)
{
	std::vector<VkClearValue> clearValues = std::vector<VkClearValue>(1);
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	vkutils::beginRenderPass(ambientRenderPass, frameDatas[frameNo].ambientFrameBuffer, swapChainExtent, cmdBuffer, clearValues);

	size_t robjCount = renderObjects.size();

	GPUPipeline ambient_pipeline = pipelines["ambient"];
	ambient_pipeline.bindPipeline(cmdBuffer);
	ambient_pipeline.bindPipelineDSets(
		cmdBuffer,
		{
			frameDatas[frameNo].positionDset,
			frameDatas[frameNo].normalImageDset,
			frameDatas[frameNo].setBuffers["camera"]._dSet,
		},
		{}
	);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(cmdBuffer);
}

void PrismRenderer::addFinalMeshCmds(VkCommandBuffer cmdBuffer, size_t frameNo)
{
	std::vector<VkClearValue> clearValues = std::vector<VkClearValue>(2);
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	vkutils::beginRenderPass(finalRenderPass, frameDatas[frameNo].swapChainFrameBuffer, swapChainExtent, cmdBuffer, clearValues);

	size_t robjCount = renderObjects.size();

	GPUPipeline final_pipeline = pipelines["final_image"];
	final_pipeline.bindPipeline(cmdBuffer);
	final_pipeline.bindPipelineDSets(
		cmdBuffer,
		{
			frameDatas[frameNo].setBuffers["camera"]._dSet,
			frameDatas[frameNo].setBuffers["scene"]._dSet,
			frameDatas[frameNo].finalComposeDset,
			frameDatas[frameNo].setBuffers["light"]._dSet,
			frameDatas[frameNo].shadow_cube_dset,
			frameDatas[frameNo].shadow_dir_dset,
		},
		{}
	);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);
}

void PrismRenderer::createFinalCmdBuffers() {
	for (size_t i = 0; i < frameDatas.size(); i++) {
		frameDatas[i].commandBuffer = vkutils::createCmdBuffer(device, frameDatas[i].commandPool);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = NULL; // Optional
		if (vkBeginCommandBuffer(frameDatas[i].commandBuffer, &beginInfo) != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer!");
		addDLightCmds(frameDatas[i].commandBuffer, i);
		addPLightCmds(frameDatas[i].commandBuffer, i);
		addGbufferCmds(frameDatas[i].commandBuffer, i);
		addAmbientCmds(frameDatas[i].commandBuffer, i);
		addFinalMeshCmds(frameDatas[i].commandBuffer, i);
		if (vkEndCommandBuffer(frameDatas[i].commandBuffer) != VK_SUCCESS) throw std::runtime_error("failed to record command buffer!");
	}
}

void PrismRenderer::refreshFinalCmdBuffers() {
	for (size_t i = 0; i < frameDatas.size(); i++) {
		if (vkResetCommandBuffer(frameDatas[i].commandBuffer, 0) != VK_SUCCESS) throw std::runtime_error("failed to reset command buffers!");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
		beginInfo.pInheritanceInfo = NULL; // Optional
		if (vkBeginCommandBuffer(frameDatas[i].commandBuffer, &beginInfo) != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer!");
		addDLightCmds(frameDatas[i].commandBuffer, i);
		addPLightCmds(frameDatas[i].commandBuffer, i);
		addGbufferCmds(frameDatas[i].commandBuffer, i);
		addAmbientCmds(frameDatas[i].commandBuffer, i);
		addFinalMeshCmds(frameDatas[i].commandBuffer, i);
		if (vkEndCommandBuffer(frameDatas[i].commandBuffer) != VK_SUCCESS) throw std::runtime_error("failed to record command buffer!");
	}
}

void PrismRenderer::genFinalCmdBuffers(size_t frameNo) {
	if (vkResetCommandBuffer(frameDatas[frameNo].commandBuffer, 0) != VK_SUCCESS) throw std::runtime_error("failed to reset command buffers!");

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
	beginInfo.pInheritanceInfo = NULL; // Optional

	if (vkBeginCommandBuffer(frameDatas[frameNo].commandBuffer, &beginInfo) != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer!");
	addDLightCmds(frameDatas[frameNo].commandBuffer, frameNo);
	addPLightCmds(frameDatas[frameNo].commandBuffer, frameNo);
	addGbufferCmds(frameDatas[frameNo].commandBuffer, frameNo);
	addAmbientCmds(frameDatas[frameNo].commandBuffer, frameNo);
	addFinalMeshCmds(frameDatas[frameNo].commandBuffer, frameNo);
	if (vkEndCommandBuffer(frameDatas[frameNo].commandBuffer) != VK_SUCCESS) throw std::runtime_error("failed to record command buffer!");
}

void PrismRenderer::createSyncObjects() {
	imagesInFlight.resize(frameDatas.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &frameDatas[i].presentSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, NULL, &frameDatas[i].renderSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, NULL, &frameDatas[i].renderFence) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void PrismRenderer::createBasicSamplers()
{
	//Create Basic Texture Sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	//get max supported anisotropy
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	//mipmap settings
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler basicTexSampler;
	if (vkCreateSampler(device, &samplerInfo, nullptr, &basicTexSampler) != VK_SUCCESS) throw std::runtime_error("failed to create texture sampler!");

	texSamplers["linear"] = basicTexSampler;

	//Create Basic Texture Sampler
	VkSamplerCreateInfo samplerInfo2{};
	samplerInfo2.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo2.magFilter = VK_FILTER_LINEAR;
	samplerInfo2.minFilter = VK_FILTER_LINEAR;
	samplerInfo2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo2.anisotropyEnable = VK_TRUE;
	//get max supported anisotropy
	VkPhysicalDeviceProperties properties2{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties2);
	samplerInfo2.maxAnisotropy = properties2.limits.maxSamplerAnisotropy;
	samplerInfo2.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo2.unnormalizedCoordinates = VK_FALSE;
	samplerInfo2.compareEnable = VK_FALSE;
	samplerInfo2.compareOp = VK_COMPARE_OP_ALWAYS;
	//mipmap settings
	samplerInfo2.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo2.mipLodBias = 0.0f;
	samplerInfo2.minLod = 0.0f;
	samplerInfo2.maxLod = 0.0f;

	VkSampler basicTexSampler2;
	if (vkCreateSampler(device, &samplerInfo2, nullptr, &basicTexSampler2) != VK_SUCCESS) throw std::runtime_error("failed to create texture sampler!");

	texSamplers["linear_clamped"] = basicTexSampler2;
}

void PrismRenderer::initVulkan()
{
	getVkInstance();
	createSurface();
	getVkLogicalDevice();

	createSwapChain(vkutils::querySwapChainSupport(physicalDevice, surface));
	makeBasicCmdPools();
	createDescriptorPool();
	makeBasicDSetLayouts();
	createBasicSamplers();

	createDepthImage();
	makeDLightMaps();
	makePLightMaps();

	createShadowRenderPass();
	createGbufferRenderPass();
	createAmbientRenderPass();
	createFinalRenderPass();

	createShadowFrameBuffers();
	createGbufferFrameBuffers();
	createAmbientFrameBuffers();
	createFinalFrameBuffers();

	makeBasicDSets();

	makeDLightShadowPipeline();
	makePLightShadowPipeline();
	makeGbufferPipeline();
	makeAmbientPipeline();
	makeFinalPipeline();

	createFinalCmdBuffers();

	createSyncObjects();
}

void PrismRenderer::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
	}
	vkDeviceWaitIdle(device);

	SwapChainSupportDetails scdetails = vkutils::querySwapChainSupport(physicalDevice, surface);
	uint32_t imageCount = scdetails.capabilities.minImageCount + 1;
	if (scdetails.capabilities.maxImageCount > 0 && imageCount > scdetails.capabilities.maxImageCount) {
		imageCount = scdetails.capabilities.maxImageCount;
	}
	bool partial_cleanup = frameDatas.size() == imageCount;
	cleanupSwapChain(partial_cleanup);

	createSwapChain(scdetails);
	if (partial_cleanup) {
		createDepthImage();
		createShadowFrameBuffers();
		createGbufferFrameBuffers();
		createAmbientFrameBuffers();
		createFinalFrameBuffers();
		makeGbufferPipeline();
		makeAmbientPipeline();
		makeFinalPipeline();
		createFinalCmdBuffers();
	}
	else{
		makeBasicCmdPools();
		createDepthImage();
		createFinalFrameBuffers();
		createGbufferFrameBuffers();
		createAmbientFrameBuffers();
		createShadowFrameBuffers();
		makeBasicDSets();
		makeGbufferPipeline();
		makeAmbientPipeline();
		makeFinalPipeline();
		makePLightShadowPipeline();
		createFinalCmdBuffers();
		imagesInFlight.resize(frameDatas.size(), VK_NULL_HANDLE);
	}
}

void PrismRenderer::updateUBOs(uint32_t curr_img)
{
	std::chrono::steady_clock::time_point currentFrameTime;

	float time = 0;
	float framedeltat = 0;

	if (time_refresh) {
		startTime = std::chrono::high_resolution_clock::now();
		time_refresh = false;
		lastFrameTime = startTime;
		currentFrameTime = startTime;
	}
	else {
		currentFrameTime = std::chrono::high_resolution_clock::now();
		time = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - startTime).count();
		framedeltat = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
		lastFrameTime = currentFrameTime;
	}

	uboUpdateCallback(framedeltat, this, curr_img);

	void* tscenedata;
	vkMapMemory(device, frameDatas[curr_img].setBuffers["scene"]._gBuffer._bufferMemory, 0, sizeof(currentScene), 0, &tscenedata);
	memcpy(tscenedata, &currentScene, sizeof(GPUSceneData));
	vkUnmapMemory(device, frameDatas[curr_img].setBuffers["scene"]._gBuffer._bufferMemory);

	void* camdata;
	vkMapMemory(device, frameDatas[curr_img].setBuffers["camera"]._gBuffer._bufferMemory, 0, sizeof(currentCamera), 0, &camdata);
	memcpy(camdata, &currentCamera, sizeof(GPUCameraData));
	vkUnmapMemory(device, frameDatas[curr_img].setBuffers["camera"]._gBuffer._bufferMemory);

	for (size_t pli = 0; pli < MAX_POINT_LIGHTS; pli++) lights[MAX_NS_LIGHTS + pli].viewproj = glm::translate(glm::mat4(1.0f), -glm::vec3(lights[MAX_NS_LIGHTS + pli].pos));
	for (size_t pli = 0; pli < MAX_DIRECTIONAL_LIGHTS; pli++) {
		lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + pli].viewproj = lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + pli].proj * glm::lookAt(
			glm::vec3(lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + pli].pos),
			glm::vec3(lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + pli].pos + glm::normalize(lights[MAX_NS_LIGHTS + MAX_POINT_LIGHTS + pli].dir)),
			glm::vec3(0, 1, 0)
		);
	}

	void* scubedata;
	vkMapMemory(device, frameDatas[curr_img].setBuffers["light"]._gBuffer._bufferMemory, 0, sizeof(GPULight) * lights.size(), 0, &scubedata);
	memcpy(scubedata, lights.data(), lights.size() * sizeof(GPULight));
	vkUnmapMemory(device, frameDatas[curr_img].setBuffers["light"]._gBuffer._bufferMemory);

	size_t objCount = renderObjects.size();
	std::vector<GPUObjectData> objubo;
	for (size_t i = 0; i < objCount; i++) {
		objubo.push_back(renderObjects[i].uboData);
	}

	if (objCount > 0) {
		void* objdata;
		vkMapMemory(device, frameDatas[curr_img].setBuffers["object"]._gBuffer._bufferMemory, 0, sizeof(objubo) * objCount, 0, &objdata);
		memcpy(objdata, objubo.data(), sizeof(GPUObjectData) * objCount);
		vkUnmapMemory(device, frameDatas[curr_img].setBuffers["object"]._gBuffer._bufferMemory);
	}

	if (refresh_cmd_buffers) {
		spawn_mut.lock();
		refreshFinalCmdBuffers();
		refresh_cmd_buffers = false;
		spawn_mut.unlock();
	}
}

void PrismRenderer::drawFrame() {
	vkWaitForFences(device, 1, &frameDatas[currentFrame].renderFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, frameDatas[currentFrame].presentSemaphore, VK_NULL_HANDLE, &imageIndex);

	imageIndex = imageIndex % MAX_FRAMES_IN_FLIGHT;

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = frameDatas[currentFrame].renderFence;

	spawn_mut.unlock();

	updateUBOs(imageIndex);
	
	//inputmgr.clearMOffset();

	spawn_mut.lock();

	genFinalCmdBuffers(currentFrame);
	//delegate_gen_final_cmd_bufs(this, currentFrame);
	//renderer_tpool->add_task(&delegate_gen_final_cmd_bufs, this, currentFrame);;
	//renderer_tpool->wait_till_done();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { frameDatas[currentFrame].presentSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frameDatas[currentFrame].commandBuffer;

	VkSemaphore signalSemaphores[] = { frameDatas[currentFrame].renderSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	

	vkResetFences(device, 1, &frameDatas[currentFrame].renderFence);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameDatas[currentFrame].renderFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void PrismRenderer::mainLoop()
{
	spawn_mut.lock();
	while (!glfwWindowShouldClose(window)) {
		drawFrame();
	}
	vkDeviceWaitIdle(device);
	spawn_mut.unlock();
}

void PrismRenderer::run()
{
	//renderer_tpool->run();
	mainLoop();
	cleanup();
}

Mesh* PrismRenderer::addMesh(std::string meshFilePath)
{
	Mesh tmesh;
	tmesh.load_from_obj(meshFilePath.c_str());
	tmesh._vertexBuffer = vkutils::createBuffer(
		device, physicalDevice,
		sizeof(Vertex) * tmesh._vertices.size(), tmesh._vertices.data(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		uploadCmdPool,
		transferQueue
	);
	tmesh._indexBuffer = vkutils::createBuffer(
		device, physicalDevice,
		sizeof(uint32_t) * tmesh._indices.size(), tmesh._indices.data(),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		uploadCmdPool,
		transferQueue
	);
	meshes[meshFilePath] = tmesh;
	return &meshes[meshFilePath];
}

void PrismRenderer::refreshMeshVB(std::string id, int fno)
{
	auto meshit = maintained_meshes.find(id);
	if (meshit != maintained_meshes.end()) {
		MaintainedMesh* tmesh = meshit->second;
		void* tmesh_vbt;
		vkMapMemory(device, tmesh->_vertexBuffer[fno]._bufferMemory, 0, sizeof(Vertex) * tmesh->_vertices.size(), 0, &tmesh_vbt);
		memcpy(tmesh_vbt, tmesh->_vertices.data(), sizeof(Vertex) * tmesh->_vertices.size());
		vkUnmapMemory(device, tmesh->_vertexBuffer[fno]._bufferMemory);

		void* tmesh_ibt;
		vkMapMemory(device, tmesh->_indexBuffer[fno]._bufferMemory, 0, sizeof(uint32_t) * tmesh->_indices.size(), 0, &tmesh_ibt);
		memcpy(tmesh_ibt, tmesh->_indices.data(), sizeof(uint32_t) * tmesh->_indices.size());
		vkUnmapMemory(device, tmesh->_indexBuffer[fno]._bufferMemory);
	}
}

GPUImage PrismRenderer::loadSingleTexture(std::string texPath, VkFormat imgFormat)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) throw std::runtime_error("failed to load texture image!");

	GPUImage tmp_gimg;
	tmp_gimg = vkutils::createGPUImage(
		device,
		physicalDevice,
		texWidth, texHeight,
		imgFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_ASPECT_COLOR_BIT
	);
	vkutils::transitionImageLayout(
		device,
		uploadCmdPool,
		transferQueue,
		tmp_gimg._image,
		imgFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);
	vkutils::copyDataToImage(
		device,
		physicalDevice,
		uploadCmdPool,
		transferQueue,
		imageSize,
		pixels,
		tmp_gimg,
		{ 0, 0, 0 },
		{ (uint32_t)texWidth, (uint32_t)texHeight, 1 }
	);
	stbi_image_free(pixels);
	vkutils::transitionImageLayout(
		device,
		frameDatas[currentFrame % MAX_FRAMES_IN_FLIGHT].commandPool,
		graphicsQueue,
		tmp_gimg._image,
		imgFormat,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	return tmp_gimg;
}

GPUTextureSet* PrismRenderer::loadObjTextures(
	std::string colorTexPath,
	std::string normalTexPath,
	std::string esTexPath,
	std::string texSamplerType
) {
	auto texit = textures.find(colorTexPath + "_" + normalTexPath + "_" + esTexPath);
	if (texit == textures.end()) {
		GPUTextureSet gts;
		gts._gImages.push_back(loadSingleTexture(colorTexPath));
		gts._gImages.push_back(loadSingleTexture(normalTexPath, VK_FORMAT_R8G8B8A8_UNORM));
		gts._gImages.push_back(loadSingleTexture(esTexPath, VK_FORMAT_R8G8B8A8_UNORM));
		gts._dSet = vkutils::createImageDSet(
			device,
			descriptorPool,
			dSetLayouts["frag_sampler_3"],
			gts._gImages,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			texSamplers["linear"]
		);

		textures[colorTexPath + "_" + normalTexPath + "_" + esTexPath] = gts;
	}
	return &textures[colorTexPath + "_" + normalTexPath + "_" + esTexPath];
}

void PrismRenderer::addRenderObj(
	std::string id,
	std::string meshFilePath,
	std::string texFilePath,
	std::string nMapFilePath,
	std::string esMapFilePath,
	std::string texSamplerType,
	glm::mat4 initTransform,
	bool include_in_final_render,
	bool include_in_shadow_map
) {
	spawn_mut.lock();
	RenderObject robj;
	robj.id = id;
	robj.renderable = include_in_final_render;
	robj.shadowcasting = include_in_shadow_map;
	robj.uboData.model = initTransform;

	auto meshit = meshes.find(meshFilePath);
	if (meshit == meshes.end()) robj.mesh = addMesh(meshFilePath);
	else robj.mesh = &(*meshit).second;

	robj.texmaps = loadObjTextures(texFilePath, nMapFilePath, esMapFilePath, texSamplerType);

	renderObjects.push_back(robj);

	spawn_mut.unlock();
}

void PrismRenderer::addRenderObj(
	std::string id,
	Mesh meshData,
	std::string texFilePath,
	std::string nMapFilePath,
	std::string esMapFilePath,
	std::string texSamplerType,
	glm::mat4 initTransform,
	bool include_in_final_render,
	bool include_in_shadow_map
) {
	spawn_mut.lock();
	RenderObject robj;
	robj.id = id;
	robj.renderable = include_in_final_render;
	robj.shadowcasting = include_in_shadow_map;
	robj.uboData.model = initTransform;

	auto meshit = meshes.find(id);
	if (meshit == meshes.end()) {
		meshData._vertexBuffer = vkutils::createBuffer(
			device, physicalDevice,
			sizeof(Vertex) * meshData._vertices.size(), meshData._vertices.data(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			uploadCmdPool,
			transferQueue
		);
		meshData._indexBuffer = vkutils::createBuffer(
			device, physicalDevice,
			sizeof(uint32_t) * meshData._indices.size(), meshData._indices.data(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			uploadCmdPool,
			transferQueue
		);
		meshes[id] = meshData;
		meshit = meshes.find(id);
	}
	robj.mesh = &(*meshit).second;

	robj.texmaps = loadObjTextures(texFilePath, nMapFilePath, esMapFilePath, texSamplerType);

	renderObjects.push_back(robj);

	spawn_mut.unlock();
}

void PrismRenderer::addMaintainedRenderObj(
	std::string id,
	MaintainedMesh* meshData,
	std::string texFilePath,
	std::string nMapFilePath,
	std::string esMapFilePath,
	std::string texSamplerType,
	glm::mat4 initTransform,
	bool include_in_final_render,
	bool include_in_shadow_map
) {
	spawn_mut.lock();
	RenderObject robj;
	robj.id = id;
	robj.maintained_mesh = true;
	robj.renderable = include_in_final_render;
	robj.shadowcasting = include_in_shadow_map;
	robj.uboData.model = initTransform;

	auto meshit = maintained_meshes.find(id);
	if (meshit == maintained_meshes.end()) {
		for (int i = 0; i < 3; i++) {
			meshData->_vertexBuffer[i] = vkutils::createBuffer(
				device, physicalDevice,
				sizeof(Vertex) * meshData->_vertices.size(), meshData->_vertices.data(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				uploadCmdPool,
				transferQueue
			);
			meshData->_indexBuffer[i] = vkutils::createBuffer(
				device, physicalDevice,
				sizeof(uint32_t) * meshData->_indices.size(), meshData->_indices.data(),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				uploadCmdPool,
				transferQueue
			);
		}
		maintained_meshes[id] = meshData;
		meshit = maintained_meshes.find(id);
	}
	robj.mmesh = meshit->second;

	robj.texmaps = loadObjTextures(texFilePath, nMapFilePath, esMapFilePath, texSamplerType);

	renderObjects.push_back(robj);

	spawn_mut.unlock();
}

void PrismRenderer::removeRenderObj(std::string id)
{
	spawn_mut.lock();
	for (auto it = renderObjects.begin(); it != renderObjects.end(); it++) {
		if (it->id == id) {
			renderObjects.erase(it);
			break;
		}
	}
	spawn_mut.unlock();
}

void PrismRenderer::removeRenderObj(size_t idx)
{
	spawn_mut.lock();
	renderObjects.erase(renderObjects.begin() + idx);
	spawn_mut.unlock();
}

void PrismRenderer::hideRenderObj(std::string id)
{
	spawn_mut.lock();
	for (auto it = renderObjects.begin(); it != renderObjects.end(); it++) {
		if (it->id == id) {
			renderObjects.erase(it);
			break;
		}
	}
	spawn_mut.unlock();
}

void PrismRenderer::removeMaintainedRenderObj(std::string id)
{
	spawn_mut.lock();
	for (auto it = renderObjects.begin(); it != renderObjects.end(); it++) {
		if (it->id == id) {
			renderObjects.erase(it);
			break;
		}
	}
	spawn_mut.unlock();
}

void PrismRenderer::cleanupSwapChain(bool destroy_only_swapchain)
{
	vkutils::destroyGPUImage(device, depthImage);
	if (destroy_only_swapchain)
		for (GPUFrameData fdata : frameDatas) {
			//vkutils::destroyGPUImage(device, fdata.swapChainImage);
			vkDestroyImageView(device, fdata.swapChainImage._imageView, NULL);
			vkDestroyFramebuffer(device, fdata.swapChainFrameBuffer, NULL);
		}
	else
		for (GPUFrameData fdata : frameDatas) {
			//vkutils::destroyGPUImage(device, fdata.swapChainImage, true);
			vkDestroyImageView(device, fdata.swapChainImage._imageView, NULL);
			vkutils::destroyGPUImage(device, fdata.pShadowMapTemp);
			vkutils::destroyGPUImage(device, fdata.dShadowMapTemp);
			vkutils::destroyGPUImage(device, fdata.pShadowDepthImage);
			vkutils::destroyGPUImage(device, fdata.dShadowDepthImage);
			vkutils::destroyGPUImage(device, fdata.colorImage);
			vkutils::destroyGPUImage(device, fdata.positionImage);
			vkutils::destroyGPUImage(device, fdata.normalImage);
			vkutils::destroyGPUImage(device, fdata.seImage);
			for (GPUImage t : fdata.shadow_cube_maps) vkutils::destroyGPUImage(device, t);
			for (GPUImage t : fdata.shadow_dir_maps) vkutils::destroyGPUImage(device, t);
			fdata.shadow_cube_maps.clear();
			fdata.shadow_dir_maps.clear();
			vkDestroyFramebuffer(device, fdata.swapChainFrameBuffer, NULL);
			vkDestroyFramebuffer(device, fdata.pShadowFrameBuffer, NULL);
			vkDestroyFramebuffer(device, fdata.dShadowFrameBuffer, NULL);
			vkFreeCommandBuffers(device, fdata.commandPool, 1, &fdata.commandBuffer);
			vkDestroySemaphore(device, fdata.renderSemaphore, NULL);
			vkDestroySemaphore(device, fdata.presentSemaphore, NULL);
			vkDestroyFence(device, fdata.renderFence, NULL);
			vkDestroyCommandPool(device, fdata.commandPool, NULL);
			for (auto t : fdata.setBuffers) vkutils::destroySetBuffer(device, descriptorPool, t.second);
			fdata.setBuffers.clear();
		}
	vkDestroySwapchainKHR(device, swapChain, NULL);
}

void PrismRenderer::cleanup()
{
	cleanupSwapChain(false);

	for (auto it : dSetLayouts) vkDestroyDescriptorSetLayout(device, it.second, NULL);
	dSetLayouts.clear();
	vkDestroyDescriptorPool(device, descriptorPool, NULL);
	for (auto it : texSamplers) vkDestroySampler(device, it.second, NULL);
	texSamplers.clear();
	for (auto it : textures) {
		for (auto jt : it.second._gImages) {
			vkutils::destroyGPUImage(device, jt);
		}
	}
	textures.clear();
	for (auto it : pipelines) {
		vkDestroyPipeline(device, it.second._pipeline, NULL);
		vkDestroyPipelineLayout(device, it.second._pipelineLayout, NULL);
	}
	pipelines.clear();
	for (auto it : meshes) {
		vkutils::destroyBuffer(device, it.second._vertexBuffer);
		vkutils::destroyBuffer(device, it.second._indexBuffer);
	}
	meshes.clear();
	for (auto it : maintained_meshes) {
		for (int i = 0; i < 3; i++) {
			vkutils::destroyBuffer(device, it.second->_vertexBuffer[i]);
			vkutils::destroyBuffer(device, it.second->_indexBuffer[i]);
		}
	}
	maintained_meshes.clear();
	vkDestroyCommandPool(device, uploadCmdPool, NULL);
	vkDestroyRenderPass(device, shadowRenderPass, NULL);
	vkDestroyRenderPass(device, ambientRenderPass, NULL);
	vkDestroyRenderPass(device, finalRenderPass , NULL);
	vkDestroyDevice(device, NULL);
	if (enableValidationLayers) vkutils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();
}

PrismRenderer::PrismRenderer(GLFWwindow* glfwWindow, void (*nextFrameCallback) (float framedeltat, PrismRenderer* renderer, uint32_t frameNo))
{
	window = glfwWindow;
	uboUpdateCallback = nextFrameCallback;
	// Layers and Extensions needed
	validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_KHR_maintenance1",
	"VK_KHR_shader_draw_parameters",
	"VK_EXT_shader_viewport_index_layer"
	};

	//renderer_tpool = new SimpleThreadPooler(RENDERER_THREADS);

	initVulkan();
}
