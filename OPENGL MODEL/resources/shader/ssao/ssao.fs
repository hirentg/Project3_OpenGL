#version 330 core

out float FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise; 	// 4x4 texture

uniform vec3 samples[64];

// parameters
uniform int kernelSize;
float radius = 0.5f;
float bias = 0.025f;

// tile noise over screen
uniform vec2 noiseScale;
uniform mat4 projection;
uniform mat4 view;


void main()
{
	// Right now our G-buffer is storing world space position
	// since we are working with SSAO, which is hardcoded in view space, we need
	// to perform a conversion
	
	// 1. Get world space data from G-buffer
	vec3 WorldSpace = texture(gPosition, TexCoord).rgb;
	vec3 WorldNormal = texture(gNormal, TexCoord).rgb;

	// Manually convert: World -> View space
	vec3 fragPos = vec3 (view * vec4(WorldSpace, 1.0));
	vec3 normal = normalize (mat3(view) * WorldNormal);

	// 2. Get random vector
	// texCoord is 0.0-1.0
	// Multiply by noiseScale make the coordinates go from 0.0 to 480.0
	vec3 randomVec = texture (texNoise, TexCoord * noiseScale).rgb;

	// 3. Create TBN: Tangent -> View space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross (normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// 4. Loop
	float occlusion = 0.0f;
	for (int i = 0; i < kernelSize; ++i)
	{
		// get sample position 
		vec3 samplePos = TBN * samples[i];	// from Tangent to View space
		samplePos = fragPos + samplePos * radius;
	
		// project sample position to sample texture
		vec4 offset = vec4 (samplePos, 1.0);
		offset = projection * offset;	// View -> Clip
		offset.xyz /= offset.w;		// perspective division
		offset.xyz = offset.xyz * 0.5 +0.5;	// transform to 0-1 range

		// get sample depth
		// since texture contains World space position
		// we need to fetch the world position then convert it to view space 
		vec3 geometryWorldPos = texture (gPosition, offset.xy).xyz;
		float geometryViewDepth = (view * vec4(geometryWorldPos, 1.0)).z;
		
		// range check 
		// check if sample is inside geometry
		// add bias to check self-acne	
		float check = (geometryViewDepth >= samplePos.z + bias ? 1.0 : 0.0);
		// Smoothen the steps		
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs (fragPos.z - 			geometryViewDepth));
		occlusion += check * rangeCheck;
	}	

	// normalize output: 1.0: white
	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = occlusion;
}