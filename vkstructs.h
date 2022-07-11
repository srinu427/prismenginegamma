#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>
#include <optional>

#define PRISM_LIGHT_SHADOW_FLAG 0x01
#define PRISM_LIGHT_DIFFUSE_FLAG 0x02
#define PRISM_LIGHT_SPECULAR_FLAG 0x04
#define PRISM_LIGHT_EMISSIVE_FLAG 0x06


struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct MeshPushConstants {
	int tidx;
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, bitangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, color);

		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct GPUPushConstant {
	VkPushConstantRange PCRange;
	void* PCData;
};

struct GPUPipeline {
	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;

	void bindPipeline(VkCommandBuffer cmdBuffer);
	void bindPipelineDSets(VkCommandBuffer cmdBuffer, std::vector<VkDescriptorSet> dSets, std::vector<GPUPushConstant> pushConstants);
};

struct RunnableGPUPipeline {
	GPUPipeline gPipeline;
	std::vector<VkDescriptorSet> dSets;
	std::vector<GPUPushConstant> pushConstants;
};

struct GPUBuffer {
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;
};

struct GPUSetBuffer {
	GPUBuffer _gBuffer;
	VkDescriptorSet _dSet;
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;

	GPUBuffer _vertexBuffer;
	GPUBuffer _indexBuffer;
	VkSampler _textureSampler;

	void add_vertices(std::vector<Vertex> verts);
	void make_cuboid(glm::vec3 center, glm::vec3 u, glm::vec3 v, float ulen, float vlen, float tlen);
	bool load_from_obj(const char* filename);
};

struct MaintainedMesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;

	GPUBuffer _vertexBuffer[3];
	GPUBuffer _indexBuffer[3];
	VkSampler _textureSampler;

	void add_vertices(std::vector<Vertex> verts);
	bool load_from_obj(const char* filename);
};

struct GPUImage {
	VkImage _image;
	VkDeviceMemory _imageMemory;
	VkImageViewCreateInfo _imageViewInfo;
	VkImageView _imageView;
};

struct GPUTextureSet {
	std::vector<GPUImage> _gImages;
	VkDescriptorSet _dSet;
};

struct GPUCameraData {
	glm::vec4 camPos;
	glm::vec4 camDir;
	glm::vec4 projprops;
	glm::mat4 viewproj;
};

struct GPUObjectData {
	glm::mat4 model = glm::mat4{ 1.0f };
};

struct GPUSceneData {
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightPosition;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};

struct GPULight{
	glm::vec4 pos = glm::vec4(0);
	glm::vec4 color = glm::vec4(0);
	glm::vec4 dir = glm::vec4(0);
	glm::vec4 props = glm::vec4(0);
	glm::mat4 proj = glm::mat4(1);
	glm::mat4 viewproj = glm::mat4(1);
	glm::ivec4 flags = glm::ivec4(0);

	void set_vp_mat(float fov = glm::radians(90.0f), float aspect = 1.0f, float near_plane = 1.0f, float far_plane = 1000.0f);
};

struct GPULightPC {
	glm::ivec4 idx = glm::ivec4(0);
	glm::mat4 viewproj = glm::mat4(1);
};

class RenderObject {
public:
	std::string id;
	Mesh* mesh;
	MaintainedMesh* mmesh;
	GPUTextureSet* texmaps;
	GPUObjectData uboData;
	bool renderable = true;
	bool shadowcasting = true;
	bool maintained_mesh = false;

	void drawMesh(VkCommandBuffer cmdBuffer, size_t obj_idx = 0);
};

struct GPUFrameData {
	std::unordered_map<std::string, GPUSetBuffer> setBuffers;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	GPUImage swapChainImage;
	GPUImage colorImage;
	GPUImage positionImage;
	GPUImage normalImage;
	GPUImage seImage;
	GPUImage ambientImage;
	GPUImage pShadowMapTemp;
	GPUImage dShadowMapTemp;
	GPUImage pShadowDepthImage;
	GPUImage dShadowDepthImage;
	std::vector<GPUImage> shadow_cube_maps;
	std::vector<GPUImage> shadow_dir_maps;

	VkFramebuffer swapChainFrameBuffer;
	VkFramebuffer gbufferFrameBuffer;
	VkFramebuffer ambientFrameBuffer;
	VkFramebuffer pShadowFrameBuffer;
	VkFramebuffer dShadowFrameBuffer;
	
	VkDescriptorSet shadow_cube_dset;
	VkDescriptorSet shadow_dir_dset;
	VkDescriptorSet positionDset;
	VkDescriptorSet normalImageDset;
	VkDescriptorSet finalComposeDset;

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;
};