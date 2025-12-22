#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D specular_diffuse1;
uniform sampler2D texture_normal1;


void main()
{
	// store fragment position in gbuffer
	gPosition = FragPos;

	// store per-fragment normal 
	vec3 normal = texture (texture_normal1, TexCoord).rgb;
	normal = normalize (normal * 2.0 - 1.0);
	gNormal = normal;
	
	// store diffuse and specular
	gAlbedoSpec.rgb = texture (texture_diffuse1, TexCoord).rgb;

	gAlbedoSpec.a = texture (specular_diffuse1, TexCoord).r;
}