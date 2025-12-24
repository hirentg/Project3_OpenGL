#version 330 core
out vec4 FragColor;

in VS_OUT {
    	vec2 TexCoord;
    	vec3 FragPos;
    	vec4 FragPosLightSpace; // Required for Directional Shadows
	mat3 TBN;
} fs_in;

// --- TEXTURES & MATERIALS ---
struct Material {
    	sampler2D texture_diffuse1;
    	sampler2D texture_specular1;
	sampler2D texture_normal1;
    	float shininess;
};
uniform Material material;

// --- LIGHT STRUCTS ---
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

// --- UNIFORMS ---
#define NR_POINT_LIGHTS 100
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

// control actual number of lights
uniform int nr_lights;

uniform vec3 viewPos;
uniform bool blinn;


// SHADOW UNIFORMS
uniform sampler2D shadowMapDir;     // Directional Shadow Map (2D)
uniform samplerCube shadowMapPoint; // Point Shadow Map (Cube)
uniform float far_plane;            // Far plane for point shadows

uniform vec3 shadowCasterPos; 


// --- FUNCTIONS ---

// 1. DIRECTIONAL SHADOW CALCULATION 
float CalcDirShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoord = projCoord * 0.5 + 0.5;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoord.z > 1.0)
        return 0.0;

    float currentDepth = projCoord.z;
    
    // Bias to prevent shadow acne
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF (Percentage-closer filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapDir, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMapDir, projCoord.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

// 2. POINT SHADOW CALCULATION 
float CalcPointShadow(vec3 fragPos)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = fragPos - shadowCasterPos; 
    
    // Use the fragment to light vector to sample from the depth map    
    float closestDepth = texture(shadowMapPoint, fragToLight).r;
    
    // Currently in linear range between [0,1], retransform it back to original depth value
    closestDepth *= far_plane;
    
    // Now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    
    // Bias
    float bias = 0.05; 
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;        

    return shadow;
}

// 3. LIGHTING CALCULATIONS

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(-light.direction);
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0); // Hardcoded 32 or use material.shininess
    }
    else
    {
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoord));
    
    // Apply Shadow
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoord));
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    // Apply Shadow
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoord));
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}



void main()
{
	vec3 normal = texture (material.texture_normal1, fs_in.TexCoord).rgb;
	// transform normal to range [-1,1]
	normal = normalize(normal * 2.0 - 1.0);

	// transform normal from tangent to world space (for ease of intergration)
	normal = normalize (fs_in.TBN * normal);

    	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    // 1. DIRECTIONAL LIGHT
    // ----------------------------------------
    float dirShadow = CalcDirShadow(fs_in.FragPosLightSpace, normal, normalize(-dirLight.direction));
    vec3 result = CalcDirLight(dirLight, normal, viewDir, dirShadow);
    
    // 2. POINT LIGHTS
    // ----------------------------------------
    // Calculate point shadow ONCE for the caster (assumed to be shadowCasterPos)
    float pointShadow = CalcPointShadow(fs_in.FragPos);

    for(int i = 0; i < nr_lights; i++)
    {
        // Check if this specific light is the one casting shadows
        // We use a small epsilon check to see if this light is the shadow caster
        float distanceToCaster = length(pointLights[i].position - shadowCasterPos);
        
        float shadowToApply = 0.0;
        if(distanceToCaster < 0.1) // This is the shadow casting light
        {
            shadowToApply = pointShadow;
        }

        result += CalcPointLight(pointLights[i], normal, fs_in.FragPos, viewDir, shadowToApply);
    }
    
    // 3. SPOT LIGHT (No shadow map yet)
    // ----------------------------------------
    result += CalcSpotLight(spotLight, normal, fs_in.FragPos, viewDir);    
    
    FragColor = vec4(result, 1.0);
}