#version 460
//#extension GL_KHR_vulkan_glsl: enable

layout(set = 0, binding = 0) uniform GPUCameraData {
	vec4 camPos;
	vec4 camDir;
	vec4 projprops;
	mat4 viewproj;
} camData;

struct ObjectData{
	mat4 model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer{ ObjectData objects[];} objectBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec3 inColor;
layout(location = 5) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec2 fragTexCoord;

void main() {
	mat4 transformMatrix = camData.viewproj * objectBuffer.objects[gl_BaseInstance].model;
	gl_Position =  transformMatrix * vec4(inPosition, 1.0);
	fragPosition = (objectBuffer.objects[gl_BaseInstance].model * vec4(inPosition, 1.0)).xyz;
	fragColor = inColor;
	fragNormal = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(inNormal, 0.0)));
	fragTangent = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(inTangent, 0.0)));
	fragBitangent = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(inBitangent, 0.0)));
	//fragBitangent = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(cross(inNormal, inTangent), 0.0)));
	fragTexCoord = inTexCoord;
}