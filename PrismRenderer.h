#pragma once

#include "vkutils.h"
#include "SimpleThreadPooler.h"

#include <mutex>
#include <vector>

class PrismRenderer
{
public:
	const size_t MAX_OBJECTS = 1000;
	const size_t MAX_NS_LIGHTS = 30;
	const size_t MAX_POINT_LIGHTS = 8;
	const size_t MAX_DIRECTIONAL_LIGHTS = 8;

	bool framebufferResized = false;
	VkExtent2D swapChainExtent = { 1280, 720 };
	VkExtent2D plight_smap_extent = { 1024, 1024 };
	VkExtent2D dlight_smap_extent = { 1 << 11, 1 << 11};

	GPUSceneData currentScene;
	GPUCameraData currentCamera;
	std::vector<RenderObject> renderObjects;
	std::vector<GPULight> lights;

	VkRenderPass finalRenderPass;
	VkRenderPass ambientRenderPass;
	VkRenderPass gbufferRenderPass;
	VkRenderPass shadowRenderPass;
	std::mutex spawn_mut;

	bool refresh_cmd_buffers = false;

	PrismRenderer(GLFWwindow* glfwWindow, void(*nextFrameCallback)(float framedeltat, PrismRenderer* renderer, uint32_t frameNo));
	void (*uboUpdateCallback) (float framedeltat, PrismRenderer* renderer, uint32_t frameNo);
	void run();
	void addRenderObj(
		std::string id,
		std::string meshFilePath,
		std::string texFilePath,
		std::string nMapFilePath,
		std::string esMapFilePath,
		std::string texSamplerType,
		glm::mat4 initTransform,
		bool include_in_final_render = true,
		bool include_in_shadow_map = true
	);
	void addRenderObj(
		std::string id,
		Mesh meshData,
		std::string texFilePath,
		std::string nMapFilePath,
		std::string esMapFilePath,
		std::string texSamplerType,
		glm::mat4 initTransform,
		bool include_in_final_render = true,
		bool include_in_shadow_map = true
	);
	void addMaintainedRenderObj(
		std::string id,
		MaintainedMesh* meshData,
		std::string texFilePath,
		std::string nMapFilePath,
		std::string esMapFilePath,
		std::string texSamplerType,
		glm::mat4 initTransform,
		bool include_in_final_render = true,
		bool include_in_shadow_map = true
	);
	void refreshMeshVB(std::string id, int fno);
	void removeRenderObj(std::string id);
	void removeRenderObj(size_t idx);
	void hideRenderObj(std::string id);
	void removeMaintainedRenderObj(std::string id);
	void genFinalCmdBuffers(size_t frameNo);
private:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;

	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilyIndices queueFamilyIndices;
	VkQueue graphicsQueue, presentQueue, transferQueue;
	VkSurfaceKHR surface;

	size_t MAX_FRAMES_IN_FLIGHT = 3;

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;

	VkDescriptorPool descriptorPool;
	std::unordered_map<std::string, VkDescriptorSetLayout> dSetLayouts;
	std::unordered_map<std::string, GPUPipeline> pipelines;
	std::unordered_map<std::string, Mesh> meshes;
	std::unordered_map<std::string, MaintainedMesh*> maintained_meshes;
	std::unordered_map<std::string, GPUTextureSet> textures;
	std::unordered_map<std::string, VkSampler> texSamplers;

	std::vector<GPUBuffer> uniformBuffers;

	std::vector<GPUFrameData> frameDatas;
	size_t currentFrame = 0;
	bool time_refresh = true;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point lastFrameTime;

	VkFormat depthFormat;
	GPUImage depthImage;

	VkCommandPool uploadCmdPool;
	std::vector<VkFence> imagesInFlight;

	uint32_t RENDERER_THREADS = 3;
	SimpleThreadPooler* renderer_tpool;
	std::mutex cpool_mtx;

	void getVkInstance();
	void createSurface();
	void getVkLogicalDevice();
	void createSwapChain(SwapChainSupportDetails swapChainSupport);
	void makeBasicCmdPools();
	void makeDLightMaps();
	void makePLightMaps();
	void createDepthImage();
	void createShadowRenderPass();
	void createGbufferRenderPass();
	void createAmbientRenderPass();
	void createFinalRenderPass();
	void createDescriptorPool();
	void addDsetLayout(std::string name, uint32_t binding, VkDescriptorType dType, uint32_t dCount, VkShaderStageFlags stageFlag);
	void makeBasicDSetLayouts();
	void makeBasicDSets();
	void addSimplePipeline(
		std::string name,
		VkRenderPass rPass,
		std::unordered_map<VkShaderStageFlagBits, std::string> stage_shader_map,
		std::vector<std::string> reqDSetLayouts,
		VkOffset2D scissorOffset,
		VkExtent2D scissorExtent,
		uint32_t VPWidth, uint32_t VPHeight,
		bool invert_VP_Y = true,
		std::vector<VkPushConstantRange> pushConstantRanges = {}
	);
	void makeFinalPipeline();
	void makeAmbientPipeline();
	void makeGbufferPipeline();
	void makeDLightShadowPipeline();
	void makePLightShadowPipeline();
	void createFinalFrameBuffers();
	void createAmbientFrameBuffers();
	void createGbufferFrameBuffers();
	void createShadowFrameBuffers();

	void makeIndirectCmdBuffer();
	void addDLightCmds(VkCommandBuffer cmdBuffer, size_t frameNo);
	void addPLightCmds(VkCommandBuffer cmdBuffer, size_t frameNo);
	void addGbufferCmds(VkCommandBuffer cmdBuffer, size_t frameNo);
	void addAmbientCmds(VkCommandBuffer cmdBuffer, size_t frameNo);
	void addFinalMeshCmds(VkCommandBuffer cmdBuffer, size_t frameNo);
	void createFinalCmdBuffers();
	void refreshFinalCmdBuffers();
	void createSyncObjects();

	void initVulkan();
	void recreateSwapChain();
	void updateUBOs(uint32_t curr_img);
	void drawFrame();
	void mainLoop();

	void createBasicSamplers();
	Mesh* addMesh(std::string meshFilePath);
	GPUImage loadSingleTexture(std::string texPath, VkFormat imgFormat=VK_FORMAT_R8G8B8A8_SRGB);
	GPUTextureSet* loadObjTextures(
		std::string colorTexPath,
		std::string normalTexPath,
		std::string esTexPath,
		std::string texSamplerType
	);

	void cleanupSwapChain(bool destroy_only_swapchain);
	void cleanup();
};

