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
	void parseCollDataJson(std::string cfname);
	void pushToRenderer(PrismRenderer* renderer, uint32_t frameNo);
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
	float camNP = 0.01;
	float camFP = 1000;

	float MOUSE_SENSITIVITY_X = -0.1f;
	float MOUSE_SENSITIVITY_Y = -0.1f;
	float CAM_SPEED = 1.0f;
	int MAX_GRAPPLE_TIME = 5000;
	float GRAPPLE_ACCELERATION = 35;
	float GRAPPLE_INIT_VEL = 1;
	float GRAPPLE_MIN_VEL = 1;

	glm::vec3 sunlightDir = (glm::vec3(0.0f, 0.2f, 0.0f));
	std::vector<bool> sb_visible_flags;

	collutils::PolyCollMesh* player;
	collutils::PolyCollMesh* player_future;
	MaintainedMesh* ghook;
	bool in_air = false;
	bool last_f_in_ground = false;
	bool have_double_jump = true;
	bool grappled = false;
	collutils::CollPoint grapple_point;
	int grapple_time = 0;
	float gravity = -100;

	glm::vec3 ground_normal = glm::vec3(0);
	int ground_plane = -1;

	std::unordered_map<std::string, ModelData> mobjects;
	size_t MAX_NS_LIGHTS = 30;
	size_t MAX_POINT_LIGHTS = 8;
	size_t MAX_DIRECTIONAL_LIGHTS = 8;
	std::vector<GPULight> nslights;
	std::vector<GPULight> plights;
	std::vector<GPULight> dlights;
	std::vector<std::string> newObjQueue;
	std::unordered_map<std::string, Mesh> new_bp_meshes;
	SimpleThreadPooler* thread_pool;
	
	void init();
	void give_grappled_va(glm::vec3 inp_vel, glm::vec3 grdir, bool do_jump, bool just_grappled = false);
	void give_ungrappled_va(glm::vec3 inp_vel, bool ground_touch, bool do_jump, bool just_ungrappled = false);
	void computeLogic(std::chrono::system_clock::time_point curr_time, std::chrono::milliseconds gap);
	std::chrono::system_clock::time_point lastLogicComputeTime;
	bool started = false;
	std::mutex rpush_mut;
	std::regex sbreg;
};
