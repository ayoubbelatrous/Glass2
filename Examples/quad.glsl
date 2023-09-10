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

void main() {
	float d = texture(u_Tex, v_TextureCoords).r;
    float aaf = fwidth(d);

	float step = 0.4;

    float alpha = smoothstep(step - aaf, step + aaf, d);
    o_Color = vec4(1,0.9,0.9, alpha);
}