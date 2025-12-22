#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;


out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	TexCoord = aTexCoord;

	FragPos = vec3 (model * vec4(aPos, 1.0));

	// Normal mapping
	mat3 normalMatrix = mat3(transpose(inverse(model)));
	vec3 T = normalize (normalMatrix * aTangent);
	vec3 B = normalize (normalMatrix * aBitangent);
	vec3 N = normalize (normalMatrix * aNormal);

	TBN = mat3(T, B, N);

}