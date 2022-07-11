#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (set = 0, binding = 0) uniform sampler2D samplerPositionDepth;
layout (set = 1, binding = 0) uniform sampler2D samplerNormal;
layout (set = 2, binding = 0) uniform GPUCameraData {
	vec4 camPos;
    vec4 camDir;
	vec4 projprops;
    mat4 viewproj;
} camData;

//Pre Calculated sin and cos values for smap blur
//float smap_sines[] = {0.0, 0.06540312923014306, 0.13052619222005157, 0.19509032201612825, 0.25881904510252074, 0.3214394653031616, 0.3826834323650898, 0.44228869021900125, 0.49999999999999994, 0.5555702330196022, 0.6087614290087207, 0.6593458151000688, 0.7071067811865476, 0.7518398074789774, 0.7933533402912352, 0.8314696123025451, 0.8660254037844386, 0.8968727415326884, 0.9238795325112867, 0.9469301294951056, 0.9659258262890683, 0.9807852804032304, 0.9914448613738104, 0.9978589232386035};
//float smap_sines[] = {0.0, 0.13052619222005157, 0.25881904510252074, 0.3826834323650898, 0.49999999999999994, 0.60876142900872078, 0.7071067811865476, 0.7933533402912352, 0.8660254037844386, 0.9238795325112867, 0.9659258262890683, 0.9914448613738104};
//float smap_cosines[] = {1.0, 0.9978589232386035, 0.9914448613738104, 0.9807852804032304, 0.9659258262890683, 0.9469301294951057, 0.9238795325112867, 0.8968727415326884, 0.8660254037844387, 0.8314696123025452, 0.7933533402912352, 0.7518398074789774, 0.7071067811865476, 0.6593458151000688, 0.6087614290087207, 0.5555702330196024, 0.5000000000000001, 0.44228869021900125, 0.38268343236508984, 0.3214394653031617, 0.25881904510252074, 0.19509032201612833, 0.1305261922200517, 0.06540312923014327};
//float smap_cosines[] = {1.0, 0.9914448613738104, 0.9659258262890683, 0.9238795325112867, 0.8660254037844387, 0.7933533402912352, 0.7071067811865476, 0.6087614290087207, 0.5000000000000001, 0.38268343236508984, 0.25881904510252074, 0.1305261922200517};
vec3 rands[] = {normalize(vec3(1,0,0)),normalize(vec3(1,0,0.5)), normalize(vec3(1,0,0.8)),
				normalize(vec3(0,1,0)), normalize(vec3(0,1,0.5)),normalize(vec3(0,1,0.8)),
				normalize(vec3(-1,0,0)), normalize(vec3(-1,0,0.5)), normalize(vec3(-1,0,0.8)),
				normalize(vec3(0,-1,0)), normalize(vec3(0,-1,0.5)), normalize(vec3(0,-1,0.8)),
				normalize(vec3(1,1,0)), normalize(vec3(1,1,0.5)), normalize(vec3(1,1,0.8)),
				normalize(vec3(1,-1,0)), normalize(vec3(1,-1,0.5)), normalize(vec3(1,-1,0.8)), 
				normalize(vec3(-1,1,0)), normalize(vec3(-1,1,0.5)), normalize(vec3(-1,1,0.8)),
				normalize(vec3(-1,-1,0)), normalize(vec3(-1,-1,0.5)), normalize(vec3(-1,-1,0.8)),};

float aobrs[] = { 0.3, 0.5, 1, 1.5, 2};
float brws[] = { 2, 1, 0.5, 0.5, 0.4};

layout (location = 0) in vec2 inUV;

layout (location = 0) out float ambientShade;

void main() 
{
    vec2 mUV = inUV;
	//mUV = mUV * 0.5f + 0.5f;
	//mUV.y = 1 - mUV.y;
	vec3 fragPos = texture(samplerPositionDepth, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	normal = normal - vec3(0.5);
	normal = 2 * normal;

	vec3 tangent;
	if (normal.y ==0 && normal.z ==0){
		tangent = vec3(0, 1, 0);
	}
	else{
		tangent = normal + vec3(1, 0, 0);
		tangent = (tangent - (dot(tangent, normal) * normal));
		tangent = normalize(tangent - (dot(tangent, normal) * normal));
	}
	vec3 bitangent = cross(normal, tangent);

	mat3 TBN = mat3(tangent, bitangent, normal);

	float light_prob = 0;
	float total_prob = 0;
	//total_prob = (2 + 1 + 0.8)*12.0;
	float bias = 0.3;
	float cfragdist = length(fragPos - camData.camPos.xyz);

	for (int si = 0; si < 9; si++){
		for (int ri = 0; ri < 3; ri++){
			vec3 sample_pos;
			if (cfragdist < 10){
				sample_pos = fragPos + TBN * (rands[si]*aobrs[ri])*(((cfragdist-0.2)*(cfragdist-0.2))/100.0);
			}
			else{
				sample_pos = fragPos + TBN * (rands[si]*aobrs[ri]);
			}

			sample_pos = fragPos + TBN * (rands[si]*aobrs[ri]);
			
			vec4 sample_frag = camData.viewproj * vec4(sample_pos, 1);
			sample_frag.xyz /= sample_frag.w;
			sample_frag.xyz = sample_frag.xyz * 0.5f + 0.5f;
			sample_frag.y = 1 - sample_frag.y;

			vec2 pixdiff = sample_frag.xy - inUV;
			if (pixdiff.x > 0.025 || pixdiff.y > 0.025){
				pixdiff = 0.0001*normalize(pixdiff);
				sample_frag.xy = inUV + pixdiff;
			}

			float sample_depth = length(sample_pos - camData.camPos.xyz);
			sample_depth *= 1.00;

			if (sample_frag.x >= 0 && sample_frag.x <= 1 || sample_frag.y >= 0 && sample_frag.y <= 1 || sample_frag.z >= 0 && sample_frag.z <= 1 && sample_frag.w > 0){

				float map_depth = texture(samplerPositionDepth, sample_frag.xy).w;
				//map_depth = length(texture(samplerPositionDepth, sample_frag.xy).xyz - camData.camPos.xyz);

				float depth_diff = sample_depth - map_depth;

				if (depth_diff <= 2 && ((depth_diff) >=  bias)) {
					light_prob += brws[ri];
				}
				total_prob += brws[ri];
			}
			else{
				//light_prob += brws[ri];
				total_prob += brws[ri];
			}
		}
	}
	if (total_prob > 0){
		ambientShade = pow(1 - (light_prob/total_prob), 2);
	}
	else{
		ambientShade = 1;
	}
}