#pragma once
#include "PrismRenderer.h"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <unordered_map>
#include <string>
#include <regex>

struct LRS {
	glm::vec3 location = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3{ 1.0f };
	glm::mat4 rotate = glm::mat4{ 1.0f };

	glm::mat4 getTMatrix();

	bool anim_finished = false;
};

struct BoneAnimStep {
	int stepduration_ms;
	int curr_time = 0;
	glm::vec3 initPos = glm::vec3(0.0f);
	glm::vec3 finalPos = glm::vec3(0.0f);
	glm::vec3 rotAxis = { 0.0f, 1.0f, 0.0f };
	float initAngle = 0;
	float finalAngle = 0;
	glm::vec3 initScale = glm::vec3(1.0f);
	glm::vec3 finalScale = glm::vec3(1.0f);
};

class BoneAnimData {
public:
	std::string name;
	std::vector<BoneAnimStep> steps;
	int curr_time = 0;
	int total_time = 0;
	int curr_step = 0;
	bool loop_anim = true;

	LRS transformAfterGap(int gap_ms);
	int getNextEventTime();
	glm::vec3 getNextVelHint();
};

class ModelData
{
public:
	std::string id;
	std::string modelFilePath;
	std::string texFilePath;
	std::string nmapFilePath;
	std::string semapFilePath;
	LRS initLRS;
	LRS objLRS;
	std::unordered_map<std::string, BoneAnimData> anims;
	std::vector<std::string> running_anims;

	void pushToRenderer(PrismRenderer* renderer);
	void updateAnim(std::string animName, int gap_ms);
};
