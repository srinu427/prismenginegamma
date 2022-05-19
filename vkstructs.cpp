#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "vkstructs.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>


void GPULight::set_vp_mat(float fov, float aspect, float near_plane, float far_plane)
{
	proj = glm::perspective(fov, aspect, near_plane, far_plane);
}

void Mesh::add_vertices(std::vector<Vertex> verts)
{
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (int i = 0; i < verts.size(); i++) {
		Vertex v = verts[i];
		if (uniqueVertices.count(v) == 0) {
			uniqueVertices[v] = static_cast<uint32_t>(_vertices.size());
			_vertices.push_back(v);
		}
		_indices.push_back(uniqueVertices[v]);
	}
}

bool Mesh::load_from_obj(const char* filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
		std::cout << (warn + err) << std::endl;
		return false;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	Vertex tri_vert[3];

	for (const auto& shape : shapes) {
		int tvi = 0;
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			tri_vert[tvi] = vertex;

			if (tvi == 2) {
				glm::vec3 e1 = tri_vert[1].pos - tri_vert[0].pos;
				glm::vec3 e2 = tri_vert[2].pos - tri_vert[0].pos;
				glm::vec2 duv1 = tri_vert[1].texCoord - tri_vert[0].texCoord;
				glm::vec2 duv2 = tri_vert[2].texCoord - tri_vert[0].texCoord;
				glm::vec3 tang;
				float denom = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);
				tang.x = denom * (duv2.y * e1.x - duv1.y * e2.x);
				tang.y = denom * (duv2.y * e1.y - duv1.y * e2.y);
				tang.z = denom * (duv2.y * e1.z - duv1.y * e2.z);
				for (int i = 0; i < 3; i++) {
					tri_vert[i].tangent = tang;
					if (uniqueVertices.count(tri_vert[i]) == 0) {
						uniqueVertices[tri_vert[i]] = static_cast<uint32_t>(_vertices.size());
						_vertices.push_back(tri_vert[i]);
					}
					_indices.push_back(uniqueVertices[tri_vert[i]]);
				}
			}
			tvi = (tvi + 1) % 3;
		}

	}
	return true;
}

void GPUPipeline::bindPipeline(VkCommandBuffer cmdBuffer)
{
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

void GPUPipeline::bindPipelineDSets(VkCommandBuffer cmdBuffer, std::vector<VkDescriptorSet> dSets, std::vector<GPUPushConstant> pushConstants)
{
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, dSets.size(), dSets.data(), 0, NULL);
	for (int i = 0; i < pushConstants.size(); i++) vkCmdPushConstants(cmdBuffer, _pipelineLayout, pushConstants[i].PCRange.stageFlags, pushConstants[i].PCRange.offset, pushConstants[i].PCRange.size, pushConstants[i].PCData);
}

void RenderObject::drawMesh(VkCommandBuffer cmdBuffer, uint32_t obj_idx)
{
	vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(mesh->_indices.size()), 1, 0, 0, obj_idx);
}
