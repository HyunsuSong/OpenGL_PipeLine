#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

uniform mat4 MVP2;

out vec2 UV;

void main() {
	gl_Position = MVP2 * vec4(vertexPosition_modelspace, 1);
	UV = (vertexPosition_modelspace.xy + vec2(1, 1)) / 2.0;
}