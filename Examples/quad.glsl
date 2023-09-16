#section vertex

#version 330

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TextureCoords;

uniform mat4 u_ViewProjection;

out vec2 v_TextureCoords;

void main() {
	v_TextureCoords = a_TextureCoords;
	gl_Position = vec4(a_Position, 1.0) * u_ViewProjection;
}

#section fragment

#version 330

out vec4 o_Color;

uniform sampler2D u_Tex;

in vec2 v_TextureCoords;
/*
void main() {
	float d = texture(u_Tex, v_TextureCoords).r;
    float aaf = fwidth(d);

	float step = 0.4;

    float alpha = smoothstep(step - aaf, step + aaf, d);

    o_Color = vec4(texture(u_Tex, v_TextureCoords).rgb, 1.0);
}
*/

const float pxRange = 5.0f;

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(ivec2 texsize, vec2 textureCoord) {
	vec2 unitRange = vec2(pxRange) / vec2(texsize);
	vec2 screenTexSize = vec2(1.0) / fwidth(v_TextureCoords);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main() {

	ivec2 TextureSize = textureSize(u_Tex, 0);

	vec4 texColor = texture(u_Tex, v_TextureCoords);

	float sd = median(texColor.r, texColor.g, texColor.b);
	float screenPxDistance = screenPxRange(TextureSize, v_TextureCoords) * (sd - 0.5);

    float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	if (alpha == 0.0)
		discard;

	vec4 color = vec4(1.0, 1.0, 1.0, alpha);

	if (color.a == 0.0)
		discard;

	o_Color = color;
}