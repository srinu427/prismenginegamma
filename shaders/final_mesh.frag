#version 460
#extension GL_KHR_vulkan_glsl: enable
#define MAX_PLIGHTS 8
#define MAX_DLIGHTS 20
#define MAX_PLIGHT_SHADOW_DISTANCE 200.0
#define MAX_DLIGHT_SHADOW_DISTANCE 200.0
#define MAX_PLIGHT_SHADOW_FALLOFF_DISTANCE 60.0
#define MAX_DLIGHT_SHADOW_FALLOFF_DISTANCE 60.0

#define SMAP_MUL_EPSILON 1.000
#define SMAP_ADD_EPSILON -0.001
#define SMAP_BLUR_EPSILON 0.0005
#define SMAP_BLUR_SCALING 3
#define SMAP_BLUR_FALLOFF 0.5
#define SMAP_BLUR_SAMPLES 12

#define SCUBE_MUL_EPSILON 1.05
#define SCUBE_ADD_EPSILON 0.01
#define SCUBE_BLUR_EPSILON 0.007
#define SCUBE_BLUR_SCALING 20
#define SCUBE_BLUR_FALLOFF 0.6
#define SCUBE_BLUR_SAMPLES 45

#define SCUBE_SOFT_MUL_EPSILON 1.00
#define SCUBE_SOFT_ADD_EPSILON 0.01
#define SCUBE_SOFT_BLUR_EPSILON 0.5
#define SCUBE_SOFT_BLUR_SCALING 10
#define SCUBE_SOFT_BLUR_FALLOFF 0.8
#define SCUBE_SOFT_BLUR_SAMPLES 45

#define SCUBE_ANTIBLUR_EPSILON 0.001
#define PI 3.1415926538

#define SHADOW_DARKNESS 0.2
#define SHADOW_BRIGHTNESS 0.80

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat4 mvMatrix;


layout(set = 2, binding = 0) uniform GPUSceneData {
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightPosition;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
} sceneData;

struct GPULight {
	vec4 pos;
	vec4 color;
	vec4 dir;
	mat4 proj;
	mat4 viewproj;
	ivec4 flags;
};

layout(set = 3, binding = 0) uniform sampler2D texSampler;
layout(std140, set = 4, binding = 0) uniform LightBuffer {GPULight lights[MAX_PLIGHTS + MAX_DLIGHTS];} lightBuffer;
layout(set = 5, binding = 0) uniform samplerCube shadowCubes[MAX_PLIGHTS];
layout(set = 6, binding = 0) uniform sampler2D shadowMaps[MAX_DLIGHTS];
layout(set = 7, binding = 0) uniform sampler2D normalSampler;

layout(location = 0) out vec4 outColor;

//Pre Calculated sin and cos values for smap blur
//float smap_sines[] = {0.0, 0.06540312923014306, 0.13052619222005157, 0.19509032201612825, 0.25881904510252074, 0.3214394653031616, 0.3826834323650898, 0.44228869021900125, 0.49999999999999994, 0.5555702330196022, 0.6087614290087207, 0.6593458151000688, 0.7071067811865476, 0.7518398074789774, 0.7933533402912352, 0.8314696123025451, 0.8660254037844386, 0.8968727415326884, 0.9238795325112867, 0.9469301294951056, 0.9659258262890683, 0.9807852804032304, 0.9914448613738104, 0.9978589232386035};
float smap_sines[] = {0.0, 0.13052619222005157, 0.25881904510252074, 0.3826834323650898, 0.49999999999999994, 0.60876142900872078, 0.7071067811865476, 0.7933533402912352, 0.8660254037844386, 0.9238795325112867, 0.9659258262890683, 0.9914448613738104};
//float smap_cosines[] = {1.0, 0.9978589232386035, 0.9914448613738104, 0.9807852804032304, 0.9659258262890683, 0.9469301294951057, 0.9238795325112867, 0.8968727415326884, 0.8660254037844387, 0.8314696123025452, 0.7933533402912352, 0.7518398074789774, 0.7071067811865476, 0.6593458151000688, 0.6087614290087207, 0.5555702330196024, 0.5000000000000001, 0.44228869021900125, 0.38268343236508984, 0.3214394653031617, 0.25881904510252074, 0.19509032201612833, 0.1305261922200517, 0.06540312923014327};
float smap_cosines[] = {1.0, 0.9914448613738104, 0.9659258262890683, 0.9238795325112867, 0.8660254037844387, 0.7933533402912352, 0.7071067811865476, 0.6087614290087207, 0.5000000000000001, 0.38268343236508984, 0.25881904510252074, 0.1305261922200517};

vec3 norm_vec_in_dir(vec3 v, vec3 dir, float val){
	if (length(dir) == 0) return v;
	vec3 ndir = normalize(dir);
	vec3 tmpo =  v + (val-dot(ndir, v)) * ndir;
	tmpo = tmpo + (val-dot(ndir, tmpo)) * ndir;
	return tmpo;
}

vec4 norm_vec_in_dir(vec4 v, vec4 dir, float val){
	if (length(dir) == 0) return v;
	vec4 ndir = normalize(dir);
	vec4 tmpo =  v + (val-dot(ndir, v)) * ndir;
	tmpo = tmpo + (val-dot(ndir, tmpo)) * ndir;
	return tmpo;
}

float compute_soft_shadow(int lidx, vec3 light_vec){
	vec3 v1, v2;

	float lvl = length(light_vec);
	if (lvl > MAX_PLIGHT_SHADOW_DISTANCE) return 1;
	vec3 lvn = normalize(light_vec);

	if (lvn.y == 0 && lvn.z == 0) {v1=vec3(0, 15, 0);}
	else {v1 = vec3(15,0,0);}

	v2 = cross(lvn, v1);
	//v1 = cross(lvn, v2);
	float light_samples = 0;
	float sharp_depth = texture(shadowCubes[lidx], light_vec).r;
	if(sharp_depth*SCUBE_SOFT_MUL_EPSILON >= lvl){light_samples += 1;}
	vec3 sample_vec;
	float brad = max(SCUBE_SOFT_BLUR_EPSILON*(1-sharp_depth/lvl), SCUBE_BLUR_EPSILON*lvl);
	float sample_weight = SCUBE_SOFT_BLUR_FALLOFF/(SCUBE_SOFT_BLUR_SCALING*brad+1);
	float theta = PI/SCUBE_SOFT_BLUR_SAMPLES;
	for (int i = 0; i < SCUBE_SOFT_BLUR_SAMPLES; i++){
		sample_vec = v1*cos(i*theta) + v2*sin(i*theta);
		sample_vec *= brad;
		if(texture(shadowCubes[lidx], light_vec + sample_vec).r*SCUBE_SOFT_MUL_EPSILON + SCUBE_SOFT_ADD_EPSILON >= lvl){light_samples += sample_weight;}
		if(texture(shadowCubes[lidx], light_vec - sample_vec).r*SCUBE_SOFT_MUL_EPSILON + SCUBE_SOFT_ADD_EPSILON >= lvl){light_samples += sample_weight;}
	}
	return SHADOW_DARKNESS + SHADOW_BRIGHTNESS*(light_samples/(1 + 2*SCUBE_SOFT_BLUR_SAMPLES*sample_weight));
}

float compute_shadow(int lidx, vec3 light_vec){
	vec3 v1, v2;

	float lvl = length(light_vec);
	if (lvl > MAX_PLIGHT_SHADOW_DISTANCE) return 1;
	vec3 lvn = normalize(light_vec);

	if (lvn.y == 0 && lvn.z == 0) {v1=vec3(0, 1, 0);}
	else {v1 = vec3(1,0,0);}

	v2 = cross(lvn, v1);
	//v1 = cross(lvn, v2);
	float light_samples = 0;
	if(texture(shadowCubes[lidx], light_vec).r*SCUBE_MUL_EPSILON >= lvl){light_samples += 1;}
	vec3 sample_vec;
	float brad = SCUBE_BLUR_EPSILON*lvl;
	float sample_weight = SCUBE_BLUR_FALLOFF/(SCUBE_BLUR_SCALING+1);
	float theta = PI/SCUBE_BLUR_SAMPLES;
	for (int i = 0; i < SCUBE_BLUR_SAMPLES; i++){
		sample_vec = v1*cos(i*theta) + v2*sin(i*theta);
		sample_vec *= brad;
		if(texture(shadowCubes[lidx], light_vec + sample_vec).r*SCUBE_MUL_EPSILON + SCUBE_ADD_EPSILON >= lvl){light_samples += sample_weight;}
		if(texture(shadowCubes[lidx], light_vec - sample_vec).r*SCUBE_MUL_EPSILON + SCUBE_ADD_EPSILON >= lvl){light_samples += sample_weight;}
	}
	return SHADOW_DARKNESS + SHADOW_BRIGHTNESS*(light_samples/(1 + 2*SCUBE_BLUR_SAMPLES*sample_weight));
}

float compute_map_shadow(int lidx, vec3 light_vec, vec4 ppos, vec4 normal){
	int midx = lidx - MAX_PLIGHTS;
	

	float lvl = length(light_vec);
	if (lvl > MAX_DLIGHT_SHADOW_DISTANCE) return 1;
	vec3 lvn = normalize(light_vec);

	vec4 smuv = lightBuffer.lights[lidx].viewproj * vec4(ppos.xyz, 1.0f);
	vec4 smuvn = smuv/smuv.w;

	float fader = 0;
	if (lvl > MAX_DLIGHT_SHADOW_FALLOFF_DISTANCE) fader = (SHADOW_BRIGHTNESS)*(lvl - MAX_DLIGHT_SHADOW_FALLOFF_DISTANCE)/(MAX_DLIGHT_SHADOW_DISTANCE - MAX_DLIGHT_SHADOW_FALLOFF_DISTANCE);
	if (smuvn.z <-1.0 || smuvn.z > 1.0 || smuvn.w < 1.0) return SHADOW_DARKNESS + fader;

	int light_samples = 0;
	int center_samples = 0;
	vec2 csample = vec2(smuvn.x/2.0 + 0.5f, -smuvn.y/2.0 + 0.5f);
	float cdist = texture(shadowMaps[midx], csample).r;
	if(cdist*SMAP_MUL_EPSILON + SMAP_ADD_EPSILON >= lvl){center_samples = 1;}
	vec2 sample_vec;
	float brad = max(0.00008, SMAP_BLUR_EPSILON*(1 - cdist/lvl));
	//vec4 v1 = brad*norm_vec_in_dir(vec4(10, 0, 0, 0), normal, 1), v2 = brad*norm_vec_in_dir(vec4(0, 10, 0, 0), normal, 1);
	vec4 v1 = brad*vec4(10, 0, 0, 0), v2 = brad*vec4(0, 10, 0, 0);
	float sample_weight = SMAP_BLUR_FALLOFF/(SMAP_BLUR_SCALING*brad+1);
	for (int i = 0; i < SMAP_BLUR_SAMPLES; i++){
		sample_vec = v1.xy*smap_cosines[i] + v2.xy*smap_sines[i];
		if(texture(shadowMaps[midx], csample + sample_vec).r*SMAP_MUL_EPSILON + SMAP_ADD_EPSILON >= lvl){light_samples += 1;}
		if(texture(shadowMaps[midx], csample - sample_vec).r*SMAP_MUL_EPSILON + SMAP_ADD_EPSILON >= lvl){light_samples += 1;}
		sample_vec = v1.xy*smap_cosines[i] - v2.xy*smap_sines[i];
		if(texture(shadowMaps[midx], csample + sample_vec).r*SMAP_MUL_EPSILON + SMAP_ADD_EPSILON >= lvl){light_samples += 1;}
		if(texture(shadowMaps[midx], csample - sample_vec).r*SMAP_MUL_EPSILON + SMAP_ADD_EPSILON >= lvl){light_samples += 1;}
	}
	float sprob = ((center_samples + light_samples*sample_weight)/(1 + 4*SMAP_BLUR_SAMPLES*sample_weight));
	
	return (SHADOW_DARKNESS + fader) + (SHADOW_BRIGHTNESS - fader)*sprob;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec4 normVal = normalize(mvMatrix * texture(normalSampler, fragTexCoord));
	float shadow = 1;
	vec4 diffuse = vec4(0);
	for (int i = 0; i < MAX_PLIGHTS + MAX_DLIGHTS; i++){
		vec3 lightVec = lightBuffer.lights[i].pos.xyz - fragPosition;
		if (bool(lightBuffer.lights[i].flags.x & 1)){
			if (i < MAX_PLIGHTS){
				shadow *= compute_shadow(i, lightVec);
			}
			else{
				shadow *= compute_map_shadow(i, lightVec, vec4(fragPosition, 1.0f), normVal);
			}
		}
		if (bool((lightBuffer.lights[i].flags.x >> 1) & 1)){
			float lcomp = 40 * dot(normVal.xyz, normalize(lightVec));
			if (length(lightVec) > 1) lcomp = lcomp/(length(lightVec)* length(lightVec));
			diffuse.x = max(lcomp * lightBuffer.lights[i].color.x, diffuse.x);
			diffuse.y = max(lcomp * lightBuffer.lights[i].color.y, diffuse.y);
			diffuse.z = max(lcomp * lightBuffer.lights[i].color.z, diffuse.z);
		}
	}
    outColor = texColor * ((SHADOW_DARKNESS * vec4(1,1,1,1)) + (SHADOW_BRIGHTNESS * diffuse)) * shadow;
}