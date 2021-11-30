#version 440

#include "../fragments/vs_common.glsl"

struct vertMat
{
	sampler2D waterHeight;
	float timeMultiplier;
	float sinMultiplier;
	float zPosMultiplier;
};

uniform vertMat u_VertexMaterial;

void main() {
	vec3 vertPos = inPosition;
	vertPos.z = texture(u_VertexMaterial.waterHeight, inUV).b;
	vertPos.z = sin(vertPos.z * u_VertexMaterial.zPosMultiplier 
		+ (u_Time * u_VertexMaterial.timeMultiplier)) 
		* u_VertexMaterial.sinMultiplier;

	outWorldPos = (u_Model * vec4(vertPos, 1.0)).xyz;
	
	gl_Position = u_ViewProjection * vec4(outWorldPos, 1);

	// Passing
	outUV = inUV;
	outColor = inColor;
	outNormal = mat3(u_NormalMatrix) * inNormal;
}

