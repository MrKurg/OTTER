#version 440

#include "../fragments/vs_common.glsl"

//Lecture 10
uniform float delta;

void main() {
	
	outColor = inColor;
	outUV = inUV;

	//LECTURE 10b
	vec3 vert = inPosition;
	//vert.y = texture(myTextureSampler, vertex_uv).r;

	//sin animation
	vert.y = sin(vert.x * 5 + delta) * 0.2;
	vert.z = 0.0;
	// Pass vertex pos in world space to frag shader
	outWorldPos = (u_Model * vec4(inPosition, 1.0)).xyz;

	// Normals
	outNormal = mat3(u_NormalMatrix) * inNormal;

	gl_Position = u_ModelViewProjection * vec4(vert, 1.0);
	//gl_Position = MVP * vec4(vertex_pos, 1.0);
}
	