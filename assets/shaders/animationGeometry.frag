#version 440 core

in GeometryData {
	vec3 position;
	vec3 normal;
	vec3 color;
	vec2 uv;
	mat3 tbn;
} inData;

layout (early_fragment_tests) in;

layout (location = 0) out vec3 position;
layout (location = 1) out vec4 diffuse;
layout (location = 2) out vec3 normal;
layout (location = 3) out float glow;

layout (location = 7) uniform sampler2D diffuseTexture;
layout (location = 8) uniform sampler2D normalTexture;
layout (location = 9) uniform sampler2D specularTexture;
layout (location = 10) uniform sampler2D glowTexture;

void main() {
	vec3 materialDiffuse = texture(diffuseTexture, inData.uv).rgb;
	float specular = texture(specularTexture, inData.uv).r;
	diffuse = vec4(materialDiffuse, 0);

	vec3 tempNormal = normalize(texture(normalTexture, inData.uv).rgb * 2 - 1);
	normal = normalize(inData.tbn * tempNormal);

	glow = texture(glowTexture, inData.uv).r;

	position = inData.position;
}