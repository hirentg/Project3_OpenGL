#version 330 core
layout (location =0) in vec3 aPos;
layout (location =1) in vec3 aNormal;
layout (location =2) in vec2 aTexCoord;
layout (location =3) in vec3 aTangent;
layout (location =4) in vec3 aBitangent;

out VS_OUT
{	
	vec2 TexCoord;
	vec3 FragPos;
	vec4 FragPosLightSpace;
	mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 lightSpaceMatrix;
uniform vec3 lightPos;
uniform vec3 viewPos;


void main()
{
	vs_out.TexCoord = aTexCoord;

	vs_out.FragPos = vec3 (model * vec4(aPos, 1.0f));
	vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
	gl_Position = projection * view * vec4(vs_out.FragPos, 1.0f);

	// Normal mapping
	mat3 normalMatrix = mat3(transpose(inverse(model)));
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 B = normalize(normalMatrix * aBitangent);
	vec3 N = normalize(normalMatrix * aNormal);

	vs_out.TBN = mat3 (T, B, N);

	
}