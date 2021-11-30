#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_common.glsl"

uniform sampler2D heightMap;

void main() {
	vec3 vertPos = inPosition;
	vertPos.z = texture(heightMap, inUV).r * 10.0f;
	outWorldPos = (u_Model * vec4(vertPos, 1.0)).xyz;
	
	gl_Position = u_ViewProjection * vec4(outWorldPos, 1);

	// Pass to Frag
	outUV = inUV;
	outColor = inColor;
	outNormal = mat3(u_NormalMatrix) * inNormal;
}

