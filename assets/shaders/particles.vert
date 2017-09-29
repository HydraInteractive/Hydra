#version 440 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec3 tangent;
layout (location = 5) in mat4 m;
layout (location = 11) in vec2 textureOffset1;
layout (location = 12) in vec2 textureOffset2;
layout (location = 13) in vec2 textureCoordInfo;

out vec2 textureCoords1;
out vec2 textureCoords2;
out float blend;

layout (location = 0) uniform mat4 view;
layout (location = 1) uniform mat4 proj;
layout (location = 2) uniform vec3 rightVector;
layout (location = 3) uniform vec3 upVector;

void main() {
	textureCoords1 = uv * textureCoordInfo.x + textureOffset1;
	textureCoords2 = uv * textureCoordInfo.x + textureOffset2;
	blend = textureCoordInfo.y;
	vec3 pos = (vec3(rightVector * position.x) + vec3(upVector * position.y));

	gl_Position = proj * view * m * vec4(pos, 1);
}