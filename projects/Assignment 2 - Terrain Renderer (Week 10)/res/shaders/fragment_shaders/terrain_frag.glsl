#version 430

#include "../fragments/fs_common_inputs.glsl"

layout(location = 0) out vec4 frag_color;

struct Material {
	sampler2D sand;
	sampler2D grass;
	sampler2D rock;
	sampler2D snow;

	float sandPos;
	float grassPos;
	float rockPos;
	float snowPos;
};

uniform Material u_Material;

#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/frame_uniforms.glsl"

void main() {
	vec4 sandTex = texture(u_Material.sand, inUV);
	vec4 grassTex = texture(u_Material.grass, inUV);
	vec4 rockTex = texture(u_Material.rock, inUV);
	vec4 snowTex = texture(u_Material.snow, inUV);

	vec3 vertPos = inWorldPos;

	if(vertPos.z > u_Material.sandPos)
	{
		frag_color = mix(
			sandTex,
			grassTex,
			smoothstep(u_Material.sandPos + 2.0, u_Material.grassPos, vertPos.z) // SandPos Modified
		);
	}

	if(vertPos.z > u_Material.grassPos)
	{
		frag_color = mix(
			grassTex,
			rockTex,
			smoothstep(u_Material.grassPos, u_Material.rockPos, vertPos.z)
		);
	}

	if(vertPos.z > u_Material.rockPos)
	{
		frag_color = mix(
			rockTex,
			snowTex,
			smoothstep(u_Material.rockPos, u_Material.snowPos, vertPos.z)
		);
	}

	if(vertPos.z > u_Material.snowPos)
	{
		frag_color = snowTex;
	}
}