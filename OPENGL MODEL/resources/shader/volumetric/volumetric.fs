#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

// 1. Inputs
uniform sampler2D gPosition;	// Where the rays end (wall/object)
uniform sampler2D shadowMapDir;
uniform mat4 lightSpaceMatrix;	// World->light space
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;


uniform int NB_STEPS;
// 2. Settings:
// const int NB_STEPS = 50;	// higher = smoother (camera)
const float G_SCATTERING = 0.5;		// 0.0: uniform fog, 0.9: dictinct sun beam

const float PI = 3.1415689;

// 3. MIE scattering 
// calculate how much light bounces torward the camera based on angle
float computeScattering (float lightDotView)
{
	float result = 1.0 - G_SCATTERING * G_SCATTERING;
	result /= (4.0 + PI * pow (1.0 + G_SCATTERING * G_SCATTERING - (2.0 * G_SCATTERING) * lightDotView, 1.5));

	return result;
}

void main()
{
	// A. retrieve ray info
	vec3 fragPos = texture (gPosition, TexCoord).rgb;
	vec3 startPosition = viewPos;

	vec3 rayVector = fragPos - startPosition;	// from camera to object
	float rayLength = length (rayVector);
	vec3 rayDirection = rayVector / rayLength;	// normalized

	float stepLength = rayLength / NB_STEPS;	// size of each slice
	vec3 step = rayDirection * stepLength;

	vec3 currentPosition = startPosition;

	// Calculating light direction for scattering
	vec3 lightDir = normalize (lightPos);
	
	float accFog = 0.0;

	// B. Raymarch loop
	for (int i = 0; i < NB_STEPS; ++i)
	{
		// 1. transform current point to shadow map coordinate
		vec4 fragPosLightSpace = lightSpaceMatrix * vec4(currentPosition, 1.0);
		vec3 projCoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
		
		// transform to 0,1 range
		projCoord = projCoord * 0.5 + 0.5;
		
		// 2. Check if we are inside the shadow map texture range
		if(projCoord.z < 1.0 && projCoord.x > 0.0 && projCoord.x < 1.0 && 			projCoord.y > 0.0 && projCoord.y < 1.0)	
		{
			float closestDepth = texture (shadowMapDir, projCoord.xy).r;
			float currentDepth = projCoord.z;

			if (currentDepth - 0.005 < closestDepth)
			{
				// if the light isnt block, then it is lit
				accFog += computeScattering (dot (rayDirection, 					lightDir));
			}
		}
	
	// 4. move to next slice
	currentPosition += step;

	}

	// C. Average and output
	accFog /= NB_STEPS;
	
	// make the fog visible (gods ray)
	accFog *= 2.0;
	
	vec3 finalFog = vec3(accFog) * lightColor;
	FragColor = vec4 (finalFog, 1.0);
	
}