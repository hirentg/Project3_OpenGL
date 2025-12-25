#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
};
uniform Material material;


void main()
{
	// store fragment position in gbuffer
	gPosition = FragPos;

	// store per-fragment normal 
	vec3 normal = texture (material.texture_normal1, TexCoord).rgb;
	// Transform range [0,1] -> [-1,1]
	normal = normalize (normal * 2.0 - 1.0);
	normal = normalize(TBN * normal);
	gNormal = normal;
	
	// store diffuse and specular
	gAlbedoSpec.rgb = texture (material.texture_diffuse1, TexCoord).rgb;

	gAlbedoSpec.a = texture (material.texture_specular1, TexCoord).r;
}