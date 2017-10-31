#version 330 core
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec4 fogColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 Eye;

out vec4 color;

float fogDensity = 0.1f;

uniform sampler2D texture_diffuse1;

float getFogFactor (float fogCoord)
{
	float result = 0.0;

	result = exp(-fogDensity*fogCoord);
	result = 1.0f - clamp(result, 0.0f, 1.0f);

	return result;
}

void main()
{    
    // Ambient
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;


	// Diffuse 
   	vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

	// Specular
	float specularStrength = 0.5f;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);  
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor; 

	vec3 result = (ambient + diffuse + specular) * texture(texture_diffuse1, TexCoords);
	float fogCoord = abs(Eye.z / Eye.w);
	color = mix(vec4(result, 1.0f), fogColor, getFogFactor(fogCoord));
}