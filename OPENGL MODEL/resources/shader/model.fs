#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;	
in vec2 TexCoord;	

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;	

//uniform sampler2D texture_diffuse1;

// for specular lighting in world space. If in viewspace the camera is always at (0,0,0)
uniform vec3 viewPos;




// For material
// dont instantiate this as sampler2D is an opaque type that cannot be instantiated

struct Material
{
	//vec3 ambient;
	//vec3 diffuse;		// removed in place for texture

	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	sampler2D texture_diffuse3;

	sampler2D texture_specular1;
	sampler2D texture_specular2;
	sampler2D texture_specular3;

	sampler2D emission;
	float shininess;
};



uniform Material material;




// Directional light function

struct DirLight
{
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;	
	vec3 specular;
};

uniform DirLight dirLight;

vec3 CalcDirLight(DirLight dirLight, vec3 normal, vec3 viewDir)
{
	vec4 textureColor = texture (material.texture_diffuse1, TexCoord);

	vec3 lightDir = normalize (-dirLight.direction);

	// diffuse shading
	float diff = max (dot (lightDir, normal), 0.0);
	
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow (max (dot (reflectDir, viewDir), 0.0), material.shininess);

	// combine result
	vec3 ambient = dirLight.ambient * vec3 (textureColor);
	vec3 diffuse = dirLight.diffuse * diff * vec3 (textureColor);
	vec3 specular = dirLight.specular * spec * vec3 (textureColor);

	return (ambient + diffuse + specular);
}



// Point light function

struct PointLight
{
	vec3 position;

	// attenuation
	float constant;
	float linear;
	float quadratic;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

// number of point lights we want
#define NR_POINT_LIGHTS 4

uniform PointLight pointLights[NR_POINT_LIGHTS];


vec3 CalcPointLight (PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec4 textureColor = texture (material.texture_diffuse1, TexCoord);

	vec3 lightDir = normalize (pointLight.position - fragPos);
	
	// diffuse
	float diff = max (dot (lightDir, normal), 0.0);
	
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow (max (dot (reflectDir, viewDir), 0.0), material.shininess);
	
	// attenuation - light drop off
	float distance = length (pointLight.position - fragPos);
	float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));

	// combine result
	vec3 ambient = pointLight.ambient * vec3 (textureColor);
	vec3 diffuse = attenuation * pointLight.diffuse * diff * vec3 (textureColor);
	vec3 specular = attenuation * pointLight.specular * spec * vec3 (textureColor);

	return (ambient + diffuse + specular);	
}



// Spotlight function (flashlight)

struct SpotLight
{
	vec3 position;
	vec3 direction;

	float cutOff;		// for inner cone
	float outerCutOff;	// for outer cone
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

uniform SpotLight spotLight;


vec3 CalcSpotLight (SpotLight spotLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{	
	vec4 textureColor = texture (material.texture_diffuse1, TexCoord);

	vec3 lightDir = normalize (spotLight.position - fragPos);

	// diffuse
	float diff = max (dot (lightDir, normal), 0.0);
	
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow (max (dot (reflectDir, viewDir), 0.0), material.shininess);
	
	// theta angle
	float theta = dot (lightDir, normalize (-spotLight.direction));
	float epsilon = spotLight.cutOff - spotLight.outerCutOff;

	float intensity = clamp ((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);


	vec3 ambient = intensity * spotLight.ambient * vec3 (textureColor);
	vec3 diffuse = intensity * spotLight.diffuse * diff * vec3 (textureColor);
	vec3 specular = intensity * spotLight.specular * spec * vec3 (textureColor);

	// still need attenuation
	// attenuation - light drop off
	float distance = length (spotLight.position - fragPos);
	float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;


	return (ambient + diffuse + specular);
}



float near = 0.1f;
float far = 100.f;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0f - 1.0f;	// back to ndc
	return (2.0 * near * far) / (far + near - z * (far - near));
}



void main()
{
	vec4 textureColor = texture (material.texture_diffuse1, TexCoord);

	vec3 norm = normalize (Normal);
	vec3 viewDir = normalize (viewPos - FragPos);
	// phase 1: Directional lighting
	vec3 result = CalcDirLight (dirLight, norm, viewDir);

	// phase 2: Point lihgts
	for (int i = 0; i < NR_POINT_LIGHTS; ++i)
	{
		result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
	}

	// phase 3: Spot light
	result += CalcSpotLight (spotLight, norm, FragPos, viewDir);
	
	if (textureColor.a < 0.0)
		discard;

	//float depth = LinearizeDepth(gl_FragCoord.z) / far;

	FragColor = vec4 (result, 1.0f);

}