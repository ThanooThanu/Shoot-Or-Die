#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
};
uniform Light light;

uniform sampler2D texture_diffuse1; 

void main()
{    
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if(texColor.a < 0.1) discard; 
    
    vec3 ambient = light.ambient * texColor.rgb;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texColor.rgb;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}