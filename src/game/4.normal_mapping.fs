#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight light;
uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;

void main() {
    // ---------------------------------------------------------
    // 1. THEME SETTINGS
    // ---------------------------------------------------------
    // Sci-Fi Brighter Atmosphere (Increased Theme Brightness)
    vec3 sciFiAmbientColor = vec3(0.5, 0.55, 0.6); 
    vec3 flashlightColor = vec3(1.0, 1.0, 1.0);     

    // Sample Texture
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 objectColor = texColor.rgb;

    // Hack: Tint untextured objects (grey/black) to look like concrete
    if(length(objectColor) < 0.1) { 
         objectColor = vec3(0.6, 0.6, 0.65); 
    }

    vec3 norm = normalize(Normal);
    
    // ---------------------------------------------------------
    // 2. FLASHLIGHT (High Power / Long Range)
    // ---------------------------------------------------------
    vec3 lightDir = normalize(viewPos - FragPos);
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    
    // Attenuation Logic
    float distance = length(viewPos - FragPos);
    
    // Low attenuation for long range flashlight (Map Scale x10 friendly)
    float attenuation = 1.0 / (1.0 + 0.007 * distance + 0.0002 * distance * distance);    

    // Combine flashlight
    vec3 flashlight = (diff * flashlightColor + spec * flashlightColor) * attenuation * 2.5; 

    // ---------------------------------------------------------
    // 3. AMBIENT LIGHT
    // ---------------------------------------------------------
    // Combine C++ ambient with Theme color
    vec3 ambient = light.ambient * sciFiAmbientColor; 
    
    // Final Result
    vec3 finalLight = ambient + flashlight;
    vec3 result = finalLight * objectColor;

    FragColor = vec4(result, 1.0);
}