#include "LogicManager.h"

#include <math.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace collutils;

static inline glm::vec3 jlist_to_vec3(rapidjson::Value& jval) {
    return glm::vec3(jval[0].GetFloat(), jval[1].GetFloat(), jval[2].GetFloat());
};

void gen_grapple_verts(glm::vec3 pcen, glm::vec3 gpoint, MaintainedMesh* ghookm, bool never_filled) {
    
    if (never_filled) {
        uint32_t tmpints[] = {1, 2, 3, 1, 3, 4, 5, 6, 7, 5, 7, 8, 9, 10, 11, 9, 11, 12, 13, 14, 15, 13, 15, 16, 17, 18, 19, 17, 19, 20, 21, 22, 23, 21, 23, 24};
        for (int i = 0; i < 36; i++) {
            tmpints[i]--;
        }
        ghookm->_indices.assign(tmpints, tmpints + 36);
        ghookm->_vertices.resize(24);
    }
    glm::vec3 gdir = glm::normalize(gpoint - pcen);
    float glen = glm::length(gpoint - pcen);
    if (glen < 0.0000001) return;
    glm::vec3 i = (gdir.y == 0 && gdir.z == 0) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    i = 0.1f * glm::normalize(normalize_vec_in_dir(i, gdir));
    glm::vec3 j = glm::cross(i, gdir);
    //glm::vec3 j = glm::vec3(0, 0.5, 0);

    ghookm->_vertices[0].pos = gpoint + i + j;
    ghookm->_vertices[0].normal = glm::normalize(j);
    ghookm->_vertices[0].texCoord = glm::vec2(0.25, glen);
    ghookm->_vertices[0].tangent = gdir;

    ghookm->_vertices[1].pos = gpoint - i + j;
    ghookm->_vertices[1].normal = ghookm->_vertices[0].normal;
    ghookm->_vertices[1].texCoord = glm::vec2(0, glen);
    ghookm->_vertices[1].tangent = gdir;

    ghookm->_vertices[2].pos = pcen - i + j;
    ghookm->_vertices[2].normal = ghookm->_vertices[0].normal;
    ghookm->_vertices[2].texCoord = glm::vec2(0, 0);
    ghookm->_vertices[2].tangent = gdir;

    ghookm->_vertices[3].pos = pcen + i + j;
    ghookm->_vertices[3].normal = ghookm->_vertices[0].normal;
    ghookm->_vertices[3].texCoord = glm::vec2(0.25, 0);
    ghookm->_vertices[3].tangent = gdir;

    ghookm->_vertices[4].pos = gpoint + i - j;
    ghookm->_vertices[4].normal = glm::normalize(i);
    ghookm->_vertices[4].texCoord = glm::vec2(0.5, glen);

    ghookm->_vertices[5].pos = gpoint + i + j;
    ghookm->_vertices[5].normal = ghookm->_vertices[4].normal;
    ghookm->_vertices[5].texCoord = glm::vec2(0.25, glen);

    ghookm->_vertices[6].pos = pcen + i + j;
    ghookm->_vertices[6].normal = ghookm->_vertices[4].normal;
    ghookm->_vertices[6].texCoord = glm::vec2(0.25, 0);

    ghookm->_vertices[7].pos = pcen + i - j;
    ghookm->_vertices[7].normal = ghookm->_vertices[4].normal;
    ghookm->_vertices[7].texCoord = glm::vec2(0.5, 0);

    ghookm->_vertices[8].pos = gpoint - i - j;
    ghookm->_vertices[8].normal = -glm::normalize(j);
    ghookm->_vertices[8].texCoord = glm::vec2(0.75, glen);

    ghookm->_vertices[9].pos = gpoint + i - j;
    ghookm->_vertices[9].normal = ghookm->_vertices[8].normal;
    ghookm->_vertices[9].texCoord = glm::vec2(0.5, glen);

    ghookm->_vertices[10].pos = pcen + i - j;
    ghookm->_vertices[10].normal = ghookm->_vertices[8].normal;
    ghookm->_vertices[10].texCoord = glm::vec2(0.5, 0);

    ghookm->_vertices[11].pos = pcen - i - j;
    ghookm->_vertices[11].normal = ghookm->_vertices[8].normal;
    ghookm->_vertices[11].texCoord = glm::vec2(0.75, 0);

    ghookm->_vertices[12].pos = gpoint - i + j;
    ghookm->_vertices[12].normal = -glm::normalize(i);
    ghookm->_vertices[12].texCoord = glm::vec2(1, glen);

    ghookm->_vertices[13].pos = gpoint - i - j;
    ghookm->_vertices[13].normal = ghookm->_vertices[12].normal;
    ghookm->_vertices[13].texCoord = glm::vec2(0.75, glen);

    ghookm->_vertices[14].pos = pcen - i - j;
    ghookm->_vertices[14].normal = ghookm->_vertices[12].normal;
    ghookm->_vertices[14].texCoord = glm::vec2(0.75, 0);

    ghookm->_vertices[15].pos = pcen - i + j;
    ghookm->_vertices[15].normal = ghookm->_vertices[12].normal;
    ghookm->_vertices[15].texCoord = glm::vec2(1, 0);

    ghookm->_vertices[16].pos = pcen + i + j;
    ghookm->_vertices[16].normal = -gdir;
    ghookm->_vertices[16].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[17].pos = pcen - i + j;
    ghookm->_vertices[17].normal = ghookm->_vertices[16].normal;
    ghookm->_vertices[17].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[18].pos = pcen - i - j;
    ghookm->_vertices[18].normal = ghookm->_vertices[16].normal;
    ghookm->_vertices[18].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[19].pos = pcen + i - j;
    ghookm->_vertices[19].normal = ghookm->_vertices[16].normal;
    ghookm->_vertices[19].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[20].pos = gpoint - i + j;
    ghookm->_vertices[20].normal = gdir;
    ghookm->_vertices[20].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[21].pos = gpoint + i + j;
    ghookm->_vertices[21].normal = ghookm->_vertices[20].normal;
    ghookm->_vertices[21].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[22].pos = gpoint + i - j;
    ghookm->_vertices[22].normal = ghookm->_vertices[20].normal;
    ghookm->_vertices[22].texCoord = glm::vec2(0, 0);

    ghookm->_vertices[23].pos = gpoint - i - j;
    ghookm->_vertices[23].normal = ghookm->_vertices[20].normal;
    ghookm->_vertices[23].texCoord = glm::vec2(0, 0);
    
}

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

        lss.read(itype, 4);
        itype[4] = '\0';

        if (itype[0] != '#') {
            try {
                lss.seekg(4);
                if (strcmp(itype, "PUVL") == 0) {
                    float plane_thickness;
                    float plane_friction;
                    lss >> plane_thickness >> plane_friction;
                    glm::vec3 u, v, rcenter;
                    float ulen, vlen;
                    lss >> rcenter.x >> rcenter.y >> rcenter.z >> u.x >> u.y >> u.z >> v.x >> v.y >> v.z >> ulen >> vlen;
                    physicsmgr->gen_and_add_pcmesh(rcenter, u, v, ulen, vlen, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "PNSP") == 0) {
                    float plane_thickness;
                    float plane_friction;
                    lss >> plane_thickness >> plane_friction;
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
                    float plane_thickness;
                    float plane_friction;
                    lss >> plane_thickness >> plane_friction;
                    glm::vec3 u, v, rcenter;
                    float ulen, vlen, tlen;
                    lss >> rcenter.x >> rcenter.y >> rcenter.z >> u.x >> u.y >> u.z >> v.x >> v.y >> v.z >> ulen >> vlen >> tlen;
                    physicsmgr->gen_and_add_pcmesh(rcenter, u, v, ulen, vlen, tlen, plane_thickness, plane_friction);
                    sb_visible_flags.push_back(true);
                    continue;
                }
                if (strcmp(itype, "CNPH") == 0) {
                    float plane_thickness;
                    float plane_friction;
                    lss >> plane_thickness >> plane_friction;
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
                    int n;
                    lss >> n;

                    BoneAnimData tmp_bad;
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

                    bool loop_state;
                    std::string loop_str, animname;
                    lss >> loop_str >> animname;
                    loop_state = (loop_str == "LOOP");

                    tmp_bad.loop_anim = loop_state;
                    tmp_bad.name = animname;

                    PolyCollMesh* lastmesh = physicsmgr->lmeshes->back();
                    lastmesh->anims[animname] = tmp_bad;
                    if (loop_state) {
                        lastmesh->running_anims.push_back(animname);
                    }

                    lastmesh = physicsmgr->lmeshes_future->back();
                    lastmesh->anims[animname] = tmp_bad;
                    if (loop_state) {
                        lastmesh->running_anims.push_back(animname);
                    }
                    continue;
                }

                if (strcmp(itype, "HIDE") == 0) {
                    sb_visible_flags[physicsmgr->lmeshes->size() - 1] = false;
                    continue;
                }
                if (strcmp(itype, "KILL") == 0) {
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav = "kill";
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav = "kill";
                    continue;
                }

                if (strcmp(itype, "RAOT") == 0) {
                    std::string animname;
                    lss >> animname;
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav = "animself";
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav_args.push_back(animname);
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->running_anims.clear();

                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav = "animself";
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav_args.push_back(animname);
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->running_anims.clear();
                    continue;
                }
                if (strcmp(itype, "RRAT") == 0) {
                    int n;
                    std::string animname;
                    lss >> n >> animname;
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav = "animremote";
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav_args.push_back(animname);
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->coll_behav_args.push_back(std::to_string(n));
                    physicsmgr->lmeshes->at(physicsmgr->lmeshes->size() - 1)->running_anims.clear();

                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav = "animremote";
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav_args.push_back(animname);
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->coll_behav_args.push_back(std::to_string(n));
                    physicsmgr->lmeshes_future->at(physicsmgr->lmeshes_future->size() - 1)->running_anims.clear();
                    continue;
                }

                if (strcmp(itype, "MDLO") == 0) {
                    lss.seekg(4);
                    std::string objPath, texPath, nmapPath, semapPath;
                    glm::vec3 init_loc, init_scale;

                    lss >> init_loc.x >> init_loc.y >> init_loc.z >> init_scale.x >> init_scale.y >> init_scale.z >> objPath >> texPath >> nmapPath >> semapPath;

                    ModelData tmpold;
                    tmpold.id = "sbound-" + std::to_string(physicsmgr->lmeshes->size() - 1);
                    tmpold.modelFilePath = objPath;
                    tmpold.texFilePath = texPath;
                    tmpold.nmapFilePath = nmapPath;
                    tmpold.semapFilePath = semapPath;
                    tmpold.initLRS.location = init_loc;
                    tmpold.initLRS.scale = init_scale;
                    tmpold.objLRS.location = init_loc;
                    tmpold.objLRS.scale = init_scale;

                    mobjects[tmpold.id] = tmpold;
                    continue;
                }
                if (strcmp(itype, "DLES") == 0) {
                    if (dlights.size() < MAX_DIRECTIONAL_LIGHTS) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_dir, light_col;
                        float fov, aspect, ldist;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_dir.x >> light_dir.y >> light_dir.z >> light_col.x >> light_col.y >> light_col.z >> ldist  >> fov >> aspect;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.dir = glm::vec4(light_dir, 0);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.props.y = ldist;
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG | PRISM_LIGHT_SHADOW_FLAG);
                        tmpl.set_vp_mat(glm::radians(fov), aspect, 0.01f, 1000.0f);
                        dlights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "DLEN") == 0) {
                    if (nslights.size() < MAX_NS_LIGHTS) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_dir, light_col;
                        float fov, aspect, ldist;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_dir.x >> light_dir.y >> light_dir.z >> light_col.x >> light_col.y >> light_col.z >> ldist >> fov >> aspect;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.dir = glm::vec4(light_dir, 0);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.props.y = ldist;
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG);
                        tmpl.set_vp_mat(glm::radians(fov), aspect, 0.01f, 1000.0f);
                        nslights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "PLEN") == 0) {
                    if (nslights.size() < MAX_NS_LIGHTS) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_col;
                        float ldist;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_col.x >> light_col.y >> light_col.z >> ldist;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.props.y = ldist;
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG);
                        nslights.push_back(tmpl);
                    }
                    continue;
                }
                if (strcmp(itype, "PLES") == 0) {
                    if (plights.size() < MAX_POINT_LIGHTS) {
                        lss.seekg(4);
                        glm::vec3 light_pos, light_col;
                        float ldist;
                        lss >> light_pos.x >> light_pos.y >> light_pos.z >> light_col.x >> light_col.y >> light_col.z >> ldist;
                        GPULight tmpl;
                        tmpl.pos = glm::vec4(light_pos, 1);
                        tmpl.color = glm::vec4(light_col, 1);
                        tmpl.props.y = ldist;
                        tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG | PRISM_LIGHT_SHADOW_FLAG);
                        plights.push_back(tmpl);
                    }
                    continue;
                }
            }
            catch (int eno) {
                std::cout << eno << std::endl;
                continue;
            }
        }
    }
}

void LogicManager::parseCollDataJson(std::string cfname)
{
    std::ifstream fr(cfname);
    std::stringstream frss;
    frss << fr.rdbuf();
    rapidjson::Document frjson;
    frjson.Parse(frss.str().c_str());
    for (uint32_t i = 0; i < frjson.Size(); i++) {
        rapidjson::Value& tmpgd = frjson[i];
        if (tmpgd["type"].GetString() == "geometry") {
            if (tmpgd["geo_type"].GetString() == "plane_uv") {
                physicsmgr->gen_and_add_pcmesh(
                    jlist_to_vec3(tmpgd["center"]),
                    jlist_to_vec3(tmpgd["u"]),
                    jlist_to_vec3(tmpgd["v"]),
                    tmpgd["ulen"].GetFloat(),
                    tmpgd["vlen"].GetFloat(),
                    tmpgd["epsilon"].GetFloat(),
                    tmpgd["friction"].GetFloat()
                );
            }
            else if (tmpgd["geo_type"].GetString() == "plane_np") {
                std::vector<glm::vec3> tfverts(tmpgd["tf_verts"].Size());
                for (uint32_t j = 0; j < tmpgd["tf_verts"].Size(); j++) {
                    tfverts[j] = jlist_to_vec3(tmpgd["tf_verts"][j]);
                }
                physicsmgr->gen_and_add_pcmesh(
                    tfverts,
                    tmpgd["epsilon"].GetFloat(),
                    tmpgd["friction"].GetFloat()
                );
            }
            else if (tmpgd["geo_type"].GetString() == "cylinder_uv") {
                physicsmgr->gen_and_add_pcmesh(
                    jlist_to_vec3(tmpgd["center"]),
                    jlist_to_vec3(tmpgd["u"]),
                    jlist_to_vec3(tmpgd["v"]),
                    tmpgd["ulen"].GetFloat(),
                    tmpgd["vlen"].GetFloat(),
                    tmpgd["tlen"].GetFloat(),
                    tmpgd["epsilon"].GetFloat(),
                    tmpgd["friction"].GetFloat()
                );
            }
            else if (tmpgd["geo_type"].GetString() == "cylinder_np") {
                std::vector<glm::vec3> tfverts(tmpgd["tf_verts"].Size());
                for (uint32_t j = 0; j < tmpgd["tf_verts"].Size(); j++) {
                    tfverts[j] = jlist_to_vec3(tmpgd["tf_verts"][j]);
                }
                physicsmgr->gen_and_add_pcmesh(
                    tfverts,
                    glm::cross(tfverts[2] - tfverts[1], tfverts[1] - tfverts[0]),
                    tmpgd["tlen"].GetFloat(),
                    tmpgd["epsilon"].GetFloat(),
                    tmpgd["friction"].GetFloat()
                );
            }

            PolyCollMesh* gen_pcm = physicsmgr->lmeshes->back();
            PolyCollMesh* gen_pcm_future = physicsmgr->lmeshes->back();

            if (tmpgd.HasMember("collision_behaviour")) {
                if (tmpgd["collision_behaviour"] == "kill") {
                    gen_pcm->coll_behav = "kill";
                    gen_pcm_future->coll_behav = "kill";
                }
                else if (tmpgd["collision_behaviour"] == "animate") {
                    gen_pcm->coll_behav = "animremote";
                    gen_pcm_future->coll_behav = "animremote";
                    for (uint32_t cai = 0; cai < tmpgd["collision_behaviour_args"].Size(); cai++) {
                        gen_pcm->coll_behav_args.push_back(tmpgd["collision_behaviour_args"][cai].GetString());
                        gen_pcm_future->coll_behav_args.push_back(tmpgd["collision_behaviour_args"][cai].GetString());
                    }
                }
            }
            if (tmpgd.HasMember("hide")) {
                sb_visible_flags[physicsmgr->lmeshes->size() - 1] = false;
            }
            if (tmpgd.HasMember("anims")) {
                for (uint32_t k = 0; k < tmpgd["anims"].Size(); k++) {
                    rapidjson::Value& tmpad = tmpgd["anims"][k];
                    BoneAnimData tmp_bad;
                    tmp_bad.name = tmpad["name"].GetString();
                    if (tmpad.HasMember("loop") && tmpad["loop"].GetBool()) tmp_bad.loop_anim = true;
                    tmp_bad.loop_anim = 
                    tmp_bad.total_time = 0;
                    for (uint32_t sti = 0; sti < tmpad["steps"].Size(); sti++) {
                        rapidjson::Value& tmpst = tmpad["steps"][sti];
                        BoneAnimStep tmp_bas;
                        tmp_bas.stepduration_ms = tmpst["anim_time"].GetInt();
                        tmp_bas.initPos = jlist_to_vec3(tmpst["init_pos"]);
                        tmp_bas.finalPos = jlist_to_vec3(tmpst["final_pos"]);
                        tmp_bad.steps.push_back(tmp_bas);
                        tmp_bad.total_time += tmp_bas.stepduration_ms;
                    }

                    gen_pcm->anims[tmp_bad.name] = tmp_bad;
                    gen_pcm_future->anims[tmp_bad.name] = tmp_bad;
                    if (tmp_bad.loop_anim) {
                        gen_pcm->running_anims.push_back(tmp_bad.name);
                        gen_pcm_future->running_anims.push_back(tmp_bad.name);
                    }
                }
                
            }
            if (tmpgd.HasMember("display_model")) {
                sb_visible_flags[physicsmgr->lmeshes->size() - 1] = false;

                rapidjson::Value& jmd = tmpgd["display_model"];

                ModelData tmpold;
                tmpold.id = "sbound-" + std::to_string(physicsmgr->lmeshes->size() - 1);
                tmpold.modelFilePath = jmd["model_path"].GetString();
                tmpold.texFilePath = jmd["texture_path"].GetString();
                tmpold.nmapFilePath = jmd["normal_map_path"].GetString();
                tmpold.semapFilePath = jmd["se_map_path"].GetString();
                tmpold.initLRS.location = jlist_to_vec3(jmd["init_loc"]);
                tmpold.initLRS.scale = jlist_to_vec3(jmd["init_scale"]);
                tmpold.objLRS = tmpold.initLRS;

                mobjects[tmpold.id] = tmpold;
            }
        }
        else if (tmpgd["type"].GetString() == "light") {
            if (!tmpgd["shadowcasting"].GetBool()) {
                GPULight tmpl;
                tmpl.pos = glm::vec4(jlist_to_vec3(tmpgd["position"]), 1);
                tmpl.color = glm::vec4(jlist_to_vec3(tmpgd["color"]), 1);
                tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG);
                nslights.push_back(tmpl);
            }
            else {
                GPULight tmpl;
                tmpl.pos = glm::vec4(jlist_to_vec3(tmpgd["position"]), 1);
                tmpl.color = glm::vec4(jlist_to_vec3(tmpgd["color"]), 1);
                tmpl.flags.x = (PRISM_LIGHT_EMISSIVE_FLAG | PRISM_LIGHT_SHADOW_FLAG);
                if (tmpgd["shadow_type"] == "directional") {
                    tmpl.dir = glm::vec4(jlist_to_vec3(tmpgd["direction"]), 0);
                    tmpl.set_vp_mat(glm::radians(tmpgd["fov"].GetFloat()), tmpgd["aspect"].GetFloat(), 0.01f, 1000.0f);
                    if (dlights.size() < 20) {
                        dlights.push_back(tmpl);
                    }
                }
                else if (tmpgd["shadow_type"] == "point") {
                    if (plights.size() < 8) {
                        plights.push_back(tmpl);
                    }
                }
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
    obama.semapFilePath = "textures/ptex1_se.png";
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
    light.semapFilePath = "textures/ptex1_se.png";
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

    ghook = new MaintainedMesh();
    gen_grapple_verts(player->_center, glm::vec3(0, 0, 0), ghook, true);

    audiomgr->add_aud_buffer("jump", "sounds/jump1.wav");
    audiomgr->add_aud_buffer("fall", "sounds/fall1.wav");
    audiomgr->add_aud_source("player");
    audiomgr->add_aud_source("1");
    audiomgr->update_listener(player->_center, player->_vel, currentCamDir, currentCamUp);

    //thread_pool = new SimpleThreadPooler(2);
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

void LogicManager::pushToRenderer(PrismRenderer* renderer, uint32_t frameNo)
{
    rpush_mut.lock();
    renderer->currentScene.sunlightPosition = glm::vec4(sunlightDir, 1.0f);
    for (int i = 0; i < nslights.size(); i++) {
        renderer->lights[i] = nslights[i];
    }
    for (int i=0; i < plights.size(); i++) {
        renderer->lights[renderer->MAX_NS_LIGHTS + i] = plights[i];
    }
    for (int i=0; i < dlights.size(); i++) {
        renderer->lights[i + renderer->MAX_NS_LIGHTS + renderer->MAX_POINT_LIGHTS] = dlights[i];
    }

    for (std::string it : newObjQueue) {
        mobjects[it].pushToRenderer(renderer);
    }
    for (auto it = new_bp_meshes.begin(); it != new_bp_meshes.end(); it++) {
        renderer->addRenderObj(
            it->first,
            it->second,
            "textures/basic_tile.png",
            "textures/flat_nmap.png",
            "textures/basic_tile_se.png",
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
        camNP,
        camFP);
    renderer->currentCamera.projprops.x = camNP;
    renderer->currentCamera.projprops.y = camFP;
    renderer->currentCamera.viewproj = proj * view;
    renderer->currentCamera.camPos = { currentCamEye, 0.0 };
    renderer->currentCamera.camDir = { currentCamDir, 0.0 };

    bool ghir = false;
    size_t ghidx = 0;
    
    size_t robjCount = renderer->renderObjects.size();
    for (size_t it = 0; it < robjCount; it++) {
        std::string objid = renderer->renderObjects[it].id;
        if (objid == "ghook") {
            ghir = true;
            ghidx = it;
        }
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

    if (grappled) {
        if (!ghir) {
            renderer->addMaintainedRenderObj("ghook", ghook, "textures/wall_tex1.png", "textures/wall_tex1_n.png", "textures/wall_tex1_se.png", "linear", glm::mat4(1));
        }
        //renderer->renderObjects[ghidx].shadowcasting = false;
        renderer->renderObjects[ghidx].renderable = true;
        renderer->refreshMeshVB("ghook", frameNo);
    }
    else {
        if (ghir) {
            renderer->renderObjects[ghidx].renderable = false;
        }
    }
    rpush_mut.unlock();
}

void LogicManager::give_grappled_va(glm::vec3 inp_vel, glm::vec3 grdir, bool do_jump, bool just_grappled)
{
    player->controlled = true;
    player_future->controlled = true;
    if (glm::length(inp_vel) > 0) {
        if (!do_jump) {
            if (glm::dot(player->_vel, glm::normalize(inp_vel)) < 15.0f) {
                player->_acc = 20.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z));
                glm::vec3 tmp = glm::normalize(glm::vec3(player->_vel.x, 0, player->_vel.z));
                if (glm::length(tmp) > 15.0f) {
                    tmp = glm::normalize(tmp);
                    player->_vel = 15.0f * (glm::vec3(tmp.x, 0, tmp.z)) + glm::vec3(0, player->_vel.y, 0);
                }
            }
        }
        else {
            player->_vel = 15.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z)) + glm::vec3(0, 50, 0);
        }
    }
    else {
        if (do_jump) {
            player->_vel.y = 50;
        }
        player->_acc.x = 0;
        player->_acc.z = 0;
    }

    if (just_grappled) {
        if (glm::dot(player->_vel, grdir) < GRAPPLE_INIT_VEL) {
            player->_vel = normalize_vec_in_dir(player->_vel, grdir, GRAPPLE_INIT_VEL);
        }
        player->_acc.y = 0;
    }
    else {
        if (glm::dot(player->_vel, grdir) < GRAPPLE_MIN_VEL) {
            player->_vel = normalize_vec_in_dir(player->_vel, grdir, GRAPPLE_MIN_VEL);
        }
    }

    player->_acc = normalize_vec_in_dir(player->_acc, glm::normalize(grdir), GRAPPLE_ACCELERATION);
}

void LogicManager::give_ungrappled_va(glm::vec3 inp_vel, bool ground_touch, bool do_jump, bool just_ungrappled)
{
    if (glm::length(inp_vel) > 0) {
        player->controlled = true;
        player_future->controlled = true;
        if (ground_touch) {
            player->_vel = 22.0f * glm::normalize(normalize_vec_in_dir(glm::vec3(inp_vel.x, 0, inp_vel.z), ground_normal, 0)) + glm::dot((*(physicsmgr->lmeshes))[ground_plane]->_vel, ground_normal) * ground_normal;
            if (do_jump) {
                player->_vel.y = (*(physicsmgr->lmeshes))[ground_plane]->_vel.y + 50;
                player->_acc.x = 0;
                player->_acc.z = 0;
            }
        }
        else {
            if (!do_jump) {
                if (glm::dot(player->_vel, glm::normalize(inp_vel)) < 15.0f) {
                    player->_acc = 20.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z));
                    glm::vec3 tmp = glm::normalize(glm::vec3(player->_vel.x, 0, player->_vel.z));
                    if (glm::length(tmp) > 15.0f) {
                        tmp = glm::normalize(tmp);
                        player->_vel = 15.0f * (glm::vec3(tmp.x, 0, tmp.z)) + glm::vec3(0, player->_vel.y, 0);
                    }
                }
            }
            else {
                player->_vel = 15.0f * glm::normalize(glm::vec3(inp_vel.x, 0, inp_vel.z)) + glm::vec3(0, 50, 0);
                player->_acc.x = 0;
                player->_acc.z = 0;
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
    }
    player->_acc.y = gravity;
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
    for (int pli = 0; pli < physicsmgr->dl_ccache[0].size(); pli++) {
        CollCache tmp_cc = physicsmgr->dl_ccache[0][pli];
        if ((!tmp_cc.m1side && !tmp_cc.m2side) && 
            (abs(glm::dot(glm::normalize(glm::vec3(tmp_cc.sep_plane)), glm::vec3(0, 1, 0))) > 0.1)) {
            float vt = glm::dot(tmp_cc.sep_plane, glm::vec4(player->_center, 1)) / glm::dot(tmp_cc.sep_plane, glm::vec4(0, 1, 0, 0));
            if (vt >= 0) {
                ground_touch = true;
                ground_plane = pli;
                ground_normal = (glm::dot(player->_center, glm::vec3(tmp_cc.sep_plane)) > 0) ? glm::vec3(tmp_cc.sep_plane) : -glm::vec3(tmp_cc.sep_plane);
                ground_normal = glm::normalize(ground_normal);
                break;
            }
        }
    }

    glm::vec3 inp_vel = glm::vec3(0);

    if (inputmgr->wasKeyPressed(GLFW_KEY_W)) inp_vel -= crelz;
    if (inputmgr->wasKeyPressed(GLFW_KEY_S)) inp_vel += crelz;
    if (inputmgr->wasKeyPressed(GLFW_KEY_A)) inp_vel -= crelx;
    if (inputmgr->wasKeyPressed(GLFW_KEY_D)) inp_vel += crelx;

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
    if (!do_jump) {
        if (last_f_in_ground) {
            if (!ground_touch) {
                ground_touch = true;
                last_f_in_ground = false;
            }
        }
        else {
            if (ground_touch) {
                last_f_in_ground = true;
            }
        }
    }
    else {
        last_f_in_ground = false;
    }

    if (ground_touch) {
        have_double_jump = true;
        if (in_air) {
            audiomgr->play_aud_buffer_from_source("player", "fall");
        }
    }

    in_air = !ground_touch;

    bool qpressed = inputmgr->wasKeyPressed(GLFW_KEY_Q);
    if (grappled) {
        glm::vec3 grdir = grapple_point.displacement1 - player->_center;
        CollPoint loltmp = physicsmgr->find_ray_first_coll(player->_center, grdir);
        if (qpressed || glm::length(grdir) <= 1 || grapple_time >= MAX_GRAPPLE_TIME || !loltmp.will_collide || glm::length(loltmp.displacement1 - grapple_point.displacement1) > 1) {
            inputmgr->clearKeyPressState(GLFW_KEY_Q);
            std::cout << "ungrappled\n";
            grappled = false;
            grapple_time = 0;
            give_ungrappled_va(inp_vel, ground_touch, do_jump, true);
        }
        else {
            give_grappled_va(inp_vel, glm::normalize(grdir), do_jump);
            grapple_time += int(logicDeltaT * 1000);
            gen_grapple_verts(player->_center, grapple_point.displacement1, ghook, false);
        }
        
    }
    else {
        if (qpressed) {
            inputmgr->clearKeyPressState(GLFW_KEY_Q);
            grapple_point = physicsmgr->find_ray_first_coll(player->_center, currentCamDir);
            if (grapple_point.will_collide) {
                glm::vec3 grdir = grapple_point.displacement1 - player->_center;
                if (glm::length(grdir) > 1) {
                    grappled = true;
                    std::cout << "grappled\n";
                    give_grappled_va(inp_vel, glm::normalize(grdir), do_jump, true);
                    grapple_time = int(logicDeltaT * 1000);
                    gen_grapple_verts(player->_center, grapple_point.displacement1, ghook, false);
                }
            }
        }
        if (!grappled) {
            give_ungrappled_va(inp_vel, ground_touch, do_jump, false);
        }
    }


    player_future->_vel = player->_vel;
    player_future->_acc = player->_acc;

    physicsmgr->run_physics(int(logicDeltaT * 1000));
    //physicsmgr->run_physics(5);

    for (auto it = mobjects.begin(); it != mobjects.end(); it++) {
        std::smatch m;
        if (std::regex_search(it->first, m, sbreg)) {
            int sbi = std::stoi(m[1].str());
            mobjects[it->second.id].objLRS.location = it->second.initLRS.location + ((*(physicsmgr->lmeshes))[sbi]->_center - (*(physicsmgr->lmeshes))[sbi]->_init_center);
        }
    }

    if (std::isnan(player->_center.x)) {
        std::cout << "center nan\n";
    }

    if (inputmgr->wasKeyPressed(GLFW_MOUSE_BUTTON_1)) {
        print_vec(player->_center - (*(physicsmgr->lmeshes))[ground_plane]->_center);
        std::cout << int(logicDeltaT * 1000) << ',' << ground_touch << ',' << last_f_in_ground << ',' << in_air << '\n';
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
