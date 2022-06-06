#pragma once
#include "PrismInputs.h"
#include "PrismPhysics.h"
#include "PrismRenderer.h"
#include "PrismAudioManager.h"
#include "ModelStructs.h"
#include "CollisionStructs.h"
#include "SimpleThreadPooler.h"

#include <regex>


class LogicManager
{
public:
	LogicManager(PrismInputs* ipmgr, PrismAudioManager* audman, int logicpolltime_ms=1);
	~LogicManager();
	void run();
	void stop();
	void parseCollDataFile(std::string cfname);
	void pushToRenderer(PrismRenderer* renderer);
private:
	PrismInputs* inputmgr;
	PrismPhysics* physicsmgr;
	PrismAudioManager* audiomgr;
	int logicPollTime = 1;
	bool shouldStop = false;
	float langle = 0;

	glm::vec3 currentCamEye = glm::vec3(15.0f, 15.0f, 15.0f);
	glm::vec3 currentCamDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - currentCamEye);
	glm::vec3 currentCamUp = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));

	float MOUSE_SENSITIVITY_X = -0.1f;
	float MOUSE_SENSITIVITY_Y = -0.1f;
	float CAM_SPEED = 1.0f;

	glm::vec3 sunlightDir = (glm::vec3(0.0f, 0.2f, 0.0f));
	std::vector<bool> sb_visible_flags;

	collutils::PolyCollMesh* player;
	collutils::PolyCollMesh* player_future;
	bool in_air = false;
	bool last_f_in_ground = false;
	bool have_double_jump = true;
	float gravity = -100;

	glm::vec3 ground_normal = glm::vec3(0);
	int ground_plane = -1;

	std::unordered_map<std::string, ModelData> mobjects;
	std::vector<GPULight> plights;
	std::vector<GPULight> dlights;
	std::vector<std::string> newObjQueue;
	std::unordered_map<std::string, Mesh> new_bp_meshes;
	SimpleThreadPooler* thread_pool;
	
	void init();
	void computeLogic(std::chrono::system_clock::time_point curr_time, std::chrono::milliseconds gap);
	std::chrono::system_clock::time_point lastLogicComputeTime;
	bool started = false;
	std::mutex rpush_mut;
	std::regex sbreg;
};
