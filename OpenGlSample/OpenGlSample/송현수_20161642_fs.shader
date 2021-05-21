// fs - rtt
#version 330 core

in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 LightDirection_cameraspace2;

out vec4 color;

uniform sampler2D myTextureSampler;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;
uniform vec3 LightPosition_worldspace2;

uniform float uvoffset;

void main() {

	vec3 LightColor = vec3(1, 1, 1);
	vec3 LightColor_2 = vec3(1, 0, 1);

	float LightPower = 10000.0f;
	float LightPower_2 = 30.0f;

	vec3 MaterialDiffuseColor = texture(myTextureSampler, vec2(UV.x + uvoffset, UV.y)).rgb;
	vec3 MaterialAmbientColor = vec3(0.3f, 0.3f, 0.3f) * MaterialDiffuseColor;

	float distance = length(LightPosition_worldspace - Position_worldspace);
	float distance2 = length(LightPosition_worldspace2 - Position_worldspace);

	vec3 n = normalize(Normal_cameraspace);
	vec3 l = normalize(LightDirection_cameraspace);

	vec3 l_2 = normalize(LightDirection_cameraspace2);

	float cosTheta = clamp(dot(n, l), 0, 1);
	float cosTheta_2 = clamp(dot(n, l_2), 0, 1);

	vec3 E = normalize(EyeDirection_cameraspace);
	vec3 R = reflect(-l, n);

	float cosAlpha = clamp(dot(E, R), 0, 1);

	color.rgb = MaterialAmbientColor
		+ MaterialDiffuseColor * LightColor * LightPower * cosTheta / (distance * distance)
		+ MaterialDiffuseColor * LightColor_2 * LightPower_2 * cosTheta_2 / (distance2 * distance2);		//기본광(기본예제)

	color.a = 1;
}