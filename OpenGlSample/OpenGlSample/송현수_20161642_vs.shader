#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;
out vec3 LightDirection_cameraspace2;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform mat4 L;
uniform vec3 LightPosition_worldspace;
uniform vec3 LightPosition_worldspace2;

void main() {

	gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);

	Position_worldspace = (M * vec4(vertexPosition_modelspace, 1)).xyz;

	vec3 vertexPosition_cameraspace = (V * M * vec4(vertexPosition_modelspace, 1)).xyz;
	EyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

	vec3 LightPosition_cameraspace = (V * vec4(LightPosition_worldspace, 1)).xyz;
	LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;

	vec3 LightPosition_cameraspace2 = (V * vec4(LightPosition_worldspace2, 1 * L)).xyz;
	LightDirection_cameraspace2 = LightPosition_cameraspace2 + EyeDirection_cameraspace;

	Normal_cameraspace = (V * M * vec4(vertexNormal_modelspace, 0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.

	UV = vertexUV;
}