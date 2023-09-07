#section vertex

#version 330

layout(location = 0) in vec3 v_Position;

uniform mat4 u_ViewProjection;

void main() {
	gl_Position = vec4(v_Position, 1.0) * u_ViewProjection ;
}

#section fragment

#version 330

out vec4 o_Color;

void main() {
	o_Color = vec4(1,1,1,1);
}