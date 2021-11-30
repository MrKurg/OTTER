#version 430

#include "../fragments/fs_common_inputs.glsl"

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;


uniform sampler2D waterTexture;

uniform vec2 waterMoveUV;

#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/frame_uniforms.glsl"

void main() {
	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(waterTexture, inUV + (waterMoveUV * u_Time));

	textureColor = vec4(textureColor.xyz, textureColor.b * 0.8);

	frag_color = textureColor;

	
}