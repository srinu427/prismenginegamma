#include "LogicManager.h"

#include <math.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace collutils;

LogicManager::LogicManager(PrismInputs* ipmgr, PrismAudioManager* audman, int logicpolltime_ms)
{
	inputmgr = ipmgr;
    audiomgr = audman;
    if (logicpolltime_ms != 0) logicPollTime = logicpolltime_ms;
    init();
}

void LogicManager::parseCollDataFile(std::string cfname)
{
    std::ifstream fr(cfname);
    std::string line;
    while (std::getline(fr, line)) {
        if (line.length() < 4) continue;

        std::istringstream lss(line);

        char itype[5];
        float plane_thickness;
        float plane_friction;

        lss.read(itype, 4);
        itype[4] = '\0';

        if (itype[0] != '#') {
            try {
                lss.seekg(4);
                lss >> plane_thickness >> plane_friction;
                if (strcmp(itype, "PUVL") == 0) {
                    glm::vec3 u, v, rcenter;
                    float ulen, vlen;
                    lss >> rcenter.x >> rcenter.y >> rcenter.z >> u.x >> u.y >> u.z >> v.x >> v.y >> v.z >> ulen >> vlen;
                    physicsmgr->gen_and_add_pcmesh(rcenter, u, v, ulen, vlen, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "PNSP") == 0) {
                    int n;
                    lss >> n;
                    std::vector<glm::vec3> points(n);
                    for (int i = 0; i < n; i++) {
                        lss >> points[i].x >> points[i].y >> points[i].z;
                    }
                    physicsmgr->gen_and_add_pcmesh(points, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "CUVH") == 0) {
                    glm::vec3 u, v, rcenter;
                    float ulen, vlen, tlen;
                    lss >> rcenter.x >> rcenter.y >> rcenter.z >> u.x >> u.y >> u.z >> v.x >> v.y >> v.z >> ulen >> vlen >> tlen;
                    physicsmgr->gen_and_add_pcmesh(rcenter, u, v, ulen, vlen, tlen, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "CNPH") == 0) {
                    int n;
                    float h;
                    lss >> n;
                    std::vector<glm::vec3> points(n);
                    for (int i = 0; i < n; i++) {
                        lss >> points[i].x >> points[i].y >> points[i].z;
                    }
                    lss >> h;
                    physicsmgr->gen_and_add_pcmesh(points,glm::cross(points[2] - points[1], points[1] - points[0]), h, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "LANI") == 0) {
                    lss.seekg(4);
                    int n;
                    lss >> n;
                    BoneAnimData tmp_bad;
                    tmp_bad.name = "fanim";
                    tmp_bad.total_time = 0;
                    for (int i = 0; i < n; i++) {
                        int step_dur;
                        glm::vec3 inip, finp;
                        lss >> step_dur >> inip.x >> inip.y >> inip.z >> finp.x >> finp.y >> finp.z;

                        BoneAnimStep tmp_bas;
                        tmp_bas.stepduration_ms = step_dur;
                        tmp_bas.initPos = inip;
                        tmp_bas.finalPos = finp;
                        tmp_bas.rotAxis = { 0,1,0 };
                        tmp_bas.initAngle = 0;
                        tmp_bas.finalAngle = 0;

                        tmp_bad.steps.push_back(tmp_bas);
                        tmp_bad.total_time += step_dur;
                    }
                    PolyCollMesh* lastmesh = physicsmgr->lmeshes->back();
                    lastmesh->anims["fanim"] = tmp_bad;
                    lastmesh->running_anims.push_back("fanim");

                    lastmesh = physicsmgr->lmeshes_future->back();
                    lastmesh->anims["fanim"] = tmp_bad;
                    lastmesh->running_anims.push_back("fanim");
                    continue;
                }
                if (strcmp(itype, "HIDE") == 0) {
                    sb_visible_flags[physicsmgr->lmeshes->size() - 1] = false;
                    continue;
                }
                if (strcmp(itype, "MDLO") == 0) {
                    lss.seekg(4);
                    std::string objPath, texPath, nmapPath;
                    glm::vec3 init_loc, init_scale;

                    lss >> init_loc.x >> init_loc.y >> init_loc.z >> init_scale.x >> init_scale.y >> init_scale.z >> objPath >> texPath >> nmapPath;

                    ModelData tmpold;
                    tmpold.id = "sbound-" + std::to_string(physicsmgr->lmeshes->size() - 1);
                    tmpold.modelFilePath = objPath;
                    tmpold.texFilePath = texPath;
                    tmpold.nmapFilePath = nmapPath;
                    tmpold.initLRS.location = init_loc;
                    tmpold.initLRS.scale = init_scale;
                    tmpold.objLRS.location = init_loc;
                    tmpold.objLRS.scale = init_scale;

                    mobjects[tmpold.id] = tmpold;
                    continue;
                }
                if (strcmp(itype, "DLES") == 0) {
                    if (dlights.size() < 20) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_dir, light_col;
                        float fov, aspect;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_dir.x >> light_dir.y >> light_dir.z >> light_col.x >> light_col.y >> light_col.z >> fov >> aspect;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.dir = glm::vec4(light_dir, 0);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG | PRISM_LIGHT_SHADOW_FLAG);
                        tmpl.set_vp_mat(glm::radians(fov), aspect, 0.01f, 1000.0f);
                        dlights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "DLEN") == 0) {
                    if (dlights.size() < 20) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_dir, light_col;
                        float fov, aspect;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_dir.x >> light_dir.y >> light_dir.z >> light_col.x >> light_col.y >> light_col.z >> fov >> aspect;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.dir = glm::vec4(light_dir, 0);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG);
                        tmpl.set_vp_mat(glm::radians(fov), aspect, 0.01f, 1000.0f);
                        dlights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "PLEN") == 0) {
                    if (plights.size() < 8) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_col;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_col.x >> light_col.y >> light_col.z;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG);
                        plights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "PLES") == 0) {
                    if (plights.size() < 8) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_col;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_col.x >> light_col.y >> light_col.z;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG | PRISM_LIGHT_SHADOW_FLAG);
                        plights.push_back(tmpl);
                    }
                    continue;
                }
            }
            catch (int eno) {
                continue;
            }
        }
    }
}

void LogicManager::init()
{
    physicsmgr = new PrismPhysics();
    sunlightDir = currentCamEye;

    ModelData obama;
    obama.id = "obama";
    obama.modelFilePath = "models/obamaprisme.obj";
    obama.texFilePath = "textures/obama_prime.jpg";
    obama.nmapFilePath = "textures/flat_nmap.png";
    obama.objLRS.location = glm::vec3(0.0f, 0.5f, 0.1f);
    obama.objLRS.scale = glm::vec3{ 1.0f };

    BoneAnimStep rotateprism;
    rotateprism.stepduration_ms = 2000;
    rotateprism.rotAxis = { 0,1,0 };
    rotateprism.initAngle = 0;
    rotateprism.finalAngle = 2 * glm::pi<float>();
    BoneAnimData rotateanim;
    rotateanim.name = "rotateprism";
    rotateanim.total_time = 1000;
    rotateanim.steps.push_back(rotateprism);
    obama.anims["rotateprism"] = rotateanim;

    //ObjectLogicData floor;
    //floor.id = "floor";
    //floor.modelFilePath = "models/floor.obj";
    //floor.texFilePath = "textures/floor_tile_2.png";

    ModelData light;
    light.id = "light";
    light.modelFilePath = "models/obamaprisme_rn.obj";
    light.texFilePath = "textures/obama_prime.jpg";
    light.nmapFilePath = "textures/flat_nmap.png";
    light.objLRS.location = currentCamEye;
    light.objLRS.scale = glm::vec3{ 0.04f };

    mobjects["obama"] = obama;
    newObjQueue.push_back("obama");

    //lObjects["room"] = room;
    //newObjQueue.push_back("room");

    mobjects["light"] = light;

    parseCollDataFile("levels/1.txt");

    for (int sbi = 0; sbi < physicsmgr->lmeshes->size(); sbi++) {
        if (mobjects.find("sbound-" + std::to_string(sbi)) == mobjects.end()) {
            if (sb_visible_flags[sbi]) {
                new_bp_meshes["sbound-" + std::to_string(sbi)] = (*(physicsmgr->lmeshes))[sbi]->gen_mesh();
            }
        }
        else {
            newObjQueue.push_back("sbound-" + std::to_string(sbi));
        }
    }
    sbreg = std::regex("sbound-([0-9]+)");
    
    //add player
    physicsmgr->gen_and_add_pcmesh(glm::vec3(10, 2.7, 10), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), 2, 2, 5, 0.1f, 100, true);
    physicsmgr->dmeshes->at(0)->_acc = glm::vec3(0, gravity, 0);
    physicsmgr->dmeshes_future->at(0)->_acc = glm::vec3(0, gravity, 0);
    player = physicsmgr->dmeshes->at(0);
    player_future = physicsmgr->dmeshes_future->at(0);
    in_air = true;

    audiomgr->add_aud_buffer("jump", "sounds/jump1.wav");
    audiomgr->add_aud_buffer("fall", "sounds/fall1.wav");
    audiomgr->add_aud_source("player");
    audiomgr->add_aud_source("1");
    audiomgr->update_listener(player->_center, player->_vel, currentCamDir, currentCamUp);

    thread_pool = new SimpleThreadPooler(2);
}

void LogicManager::run()
{
	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	std::chrono::milliseconds interval = std::chrono::milliseconds(logicPollTime);
	std::chrono::system_clock::time_point wait_until = start + interval;

	while (!shouldStop) {
		std::chrono::milliseconds gap = interval;
		std::chrono::system_clock::time_point curr_time = std::chrono::system_clock::now();
		if (wait_until < curr_time) {
			std::chrono::milliseconds offset = ((std::chrono::ceil<std::chrono::milliseconds>(curr_time - wait_until).count() / std::chrono::ceil<std::chrono::milliseconds>(interval).count()) + 1) * interval;
			wait_until += offset;
			gap += offset;
		}
		std::this_thread::sleep_until(wait_until);
		computeLogic(wait_until, gap);
		wait_until += interval;
	}
}

void LogicManager::pushToRenderer(PrismRenderer* renderer)
{
    rpush_mut.lock();
    renderer->currentScene.sunlightPosition = glm::vec4(sunlightDir, 1.0f);
    for (int i=0; i < plights.size(); i++) {
        renderer->lights[i] = plights[i];
    }
    for (int i=0; i < dlights.size(); i++) {
        renderer->lights[i + renderer->MAX_POINT_LIGHTS] = dlights[i];
    }

    for (std::string it : newObjQueue) {
        mobjects[it].pushToRenderer(renderer);
    }
    for (auto it = new_bp_meshes.begin(); it != new_bp_meshes.end(); it++) {
        renderer->addRenderObj(
            it->first,
            it->second,
            "textures/basic_tile.jpg",
            "textures/flat_nmap.png",
            "linear",
            glm::mat4(1)
        );
    }
    newObjQueue.clear();
    new_bp_meshes.clear();

    glm::mat4 view = glm::lookAt(
        currentCamEye,
        currentCamEye + currentCamDir,
        currentCamUp
    );
    glm::mat4 proj = glm::perspective(
        glm::radians(90.0f),
        renderer->swapChainExtent.width / (float)renderer->swapChainExtent.height,
        0.01f,
        1000.0f);
    renderer->currentCamera.viewproj = proj * view;
    renderer->currentCamera.camPos = { currentCamEye, 0.0 };
    renderer->currentCamera.camDir = { currentCamDir, 0.0 };

    
    size_t robjCount = renderer->renderObjects.size();
    for (size_t it = 0; it < robjCount; it++) {
        std::string objid = renderer->renderObjects[it].id;
        if (mobjects.find(objid) == mobjects.end()) {
            std::smatch m;
            if (std::regex_search(objid, m, sbreg)) {
                int sbi = std::stoi(m[1].str());
                if (sb_visible_flags[sbi]) {
                    renderer->renderObjects[it].uboData.model = glm::translate(glm::mat4(1.0f), physicsmgr->lmeshes->at(sbi)->_center - physicsmgr->lmeshes->at(sbi)->_init_center);
                }
            }
        }
        else {
            renderer->renderObjects[it].uboData.model = mobjects[objid].objLRS.getTMatrix();
        }
    }
    rpush_mut.unlock();
}

void LogicManager::computeLogic(std::chrono::system_clock::time_point curr_time, std::chrono::milliseconds gap)
{
    rpush_mut.lock();
    float logicDeltaT = std::chrono::duration<float, std::chrono::seconds::period>(gap).count();
    
    langle = (langle + (logicDeltaT * 0.5f));
    glm::vec3 crelx = glm::normalize(glm::cross(currentCamDir, currentCamUp));
    glm::vec3 crely = currentCamUp;
    glm::vec3 crelz = glm::cross(crelx, crely);

    if (inputmgr->wasKeyPressed(GLFW_KEY_F)) {
        dlights[0].pos = glm::vec4(currentCamEye, 1.0);
    }
    if (inputmgr->wasKeyPressed(GLFW_KEY_G)) {
        dlights[1].pos = glm::vec4(currentCamEye, 1.0);
    }
    if (inputmgr->wasKeyPressed(GLFW_KEY_H)) {
        dlights[2].pos = glm::vec4(currentCamEye, 1.0);
        dlights[2].dir = glm::vec4(currentCamDir, 1.0);
    }

    bool ground_touch = false;
    int ground_plane = -1;
    glm::vec3 ground_normal = glm::vec3(0);
    for (int pli = 0; pli < physicsmgr->dl_ccache[0].size(); pli++) {
        CollCache tmp_cc = physicsmgr->dl_ccache[0][pli];
        if ((!tmp_cc.m1side && !tmp_cc.m2side) && abs(glm::dot(glm::normalize(glm::vec3(tmp_cc.sep_plane)), glm::vec3(0, 1, 0))) > 0.1) {
            ground_touch = true;
            ground_plane = pli;
            ground_normal = (glm::dot(player->_center, glm::vec3(tmp_cc.sep_plane)) > 0) ? glm::vec3(tmp_cc.sep_plane) : -glm::vec3(tmp_cc.sep_plane);
            ground_normal = glm::normalize(ground_normal);
            break;
        }
    }

    glm::vec3 inp_vel = glm::vec3(0);

    if (inputmgr->wasKeyPressed(GLFW_KEY_W)) inp_vel -= crelz;
    if (inputmgr->wasKeyPressed(GLFW_KEY_S)) inp_vel += crelz;
    if (inputmgr->wasKeyPressed(GLFW_KEY_A)) inp_vel -= crelx;
    if (inputmgr->wasKeyPressed(GLFW_KEY_D)) inp_vel += crelx;
    if (ground_touch) {
        have_double_jump = true;
        if (in_air) {
            audiomgr->play_aud_buffer_from_source("player", "fall");
        }
    }

    bool do_jump = false;

    if (inputmgr->wasKeyPressed(GLFW_KEY_SPACE)) {
        
        if (ground_touch) {
            inputmgr->clearKeyPressState(GLFW_KEY_SPACE);
            audiomgr->play_aud_buffer_from_source("player", "jump");
            do_jump = true;
        }
        else {
            if (have_double_jump) {
                inputmgr->clearKeyPressState(GLFW_KEY_SPACE);
                audiomgr->play_aud_buffer_from_source("player", "jump");
                have_double_jump = false;
                do_jump = true;
            }
        }
    }

    in_air = !ground_touch;

    if (glm::length(inp_vel) > 0) {
        player->controlled = true;
        player_future->controlled = true;
        if (ground_touch) {
            player->_vel = 20.0f * glm::normalize(normalize_vec_in_dir(glm::vec3(inp_vel.x, 0, inp_vel.z), ground_normal, 0)) + glm::dot((*(physicsmgr->lmeshes))[ground_plane]->_vel, ground_normal) * ground_normal;
            if (do_jump) {
                player->_vel.y = (*(physicsmgr->lmeshes))[ground_plane]->_vel.y + 50;
                player->_acc.x = 0;
                player->_acc.z = 0;
                player->_acc.y = gravity;
            }
            else {
                //player->_acc = (*(physicsmgr->lmeshes))[ground_plane]->friction * glm::normalize(normalize_vec_in_dir(glm::vec3(inp_vel.x, 0, inp_vel.z), ground_normal, 0)) + glm::dot((*(physicsmgr->lmeshes))[ground_plane]->_vel, ground_normal) * ground_normal;
                player->_acc.y = gravity;
            }
            
        }
        else {
            if (!do_jump) {
                if (glm::dot(player->_vel, glm::normalize(inp_vel)) < 15.0f) {
                    player->_acc = glm::vec3(0, gravity, 0) + 40.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z));
                    glm::vec3 tmp = glm::normalize(glm::vec3(player->_vel.x, 0, player->_vel.z));
                    if (glm::length(tmp) > 15.0f) {
                        tmp = glm::normalize(tmp);
                        player->_vel = 15.0f * (glm::vec3(tmp.x, 0, tmp.z)) + glm::vec3(0, player->_vel.y, 0);
                    }
                }
            }
            else {
                std::cout << "djump";
                player->_vel = 20.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z)) + glm::vec3(0, 50, 0);
                player->_acc.x = 0;
                player->_acc.z = 0;
                player->_acc.y = gravity;
            }
        }
    }
    else {
        player->controlled = false;
        player_future->controlled = false;
        if (do_jump) {
            player->_vel.y = 50;
        }
        player->_acc.x = 0;
        player->_acc.z = 0;
        player->_acc.y = gravity;
    }

    player_future->_vel = player->_vel;
    player_future->_acc = player->_acc;

    physicsmgr->run_physics(int(logicDeltaT * 1000));
    //physicsmgr->run_physics(5);

    /*
    for (auto it : mobjects) {
        std::smatch m;
        if (std::regex_search(it.first, m, sbreg)) {
            int sbi = std::stoi(m[1].str());
            mobjects[it.second.id].objLRS.location = it.second.initLRS.location + ((*(physicsmgr->lmeshes))[sbi]->_center - (*(physicsmgr->lmeshes))[sbi]->_init_center);
        }
    }
    */

    if (inputmgr->wasKeyPressed(GLFW_MOUSE_BUTTON_1)) {
        print_vec(player->_center);
    }
    currentCamEye = player->_center + glm::vec3(0, 2.5, 0);
    currentCamDir = glm::vec3(glm::rotate(glm::mat4(1.0f),
        glm::radians(MOUSE_SENSITIVITY_X * float(inputmgr->dmx)),
        crely) * glm::vec4(currentCamDir, 0.0f));

    float nrb = glm::dot(glm::normalize(currentCamUp), glm::normalize(currentCamDir));
    if (!(abs(nrb) > 0.9 && nrb*inputmgr->dmy < 0)) {
        currentCamDir = glm::vec3(glm::rotate(glm::mat4(1.0f),
            glm::radians(MOUSE_SENSITIVITY_Y * float(inputmgr->dmy)),
            crelx) * glm::vec4(currentCamDir, 0.0f));
    }
    audiomgr->update_aud_source("player", player->_center, player->_vel);
    audiomgr->update_listener(player->_center, player->_vel, currentCamDir, currentCamUp);

    inputmgr->clearMOffset();
    rpush_mut.unlock();
}

void LogicManager::stop()
{
    rpush_mut.lock();
    shouldStop = true;
    rpush_mut.unlock();
}

LogicManager::~LogicManager()
{
    delete physicsmgr;
}
