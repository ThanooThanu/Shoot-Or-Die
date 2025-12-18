#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aTangent;
layout (location = 4) in vec4 aBitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const int MAX_BONES = 200; 
uniform mat4 finalBonesMatrices[MAX_BONES];

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    
    float totalWeight = 0.0; // ตัวแปรเช็คน้ำหนักรวม

    for(int i = 0 ; i < 4 ; i++) 
    {
        // 1. ตัดกระดูกเสีย (-1) และกระดูกเกินทิ้ง
        if(boneIds[i] < 0 || boneIds[i] >= MAX_BONES) 
        {
            continue; 
        }

        // 2. ตัดน้ำหนัก 0 ทิ้ง
        if(weights[i] <= 0.0)
        {
            continue;
        }

        // คำนวณปกติ
        vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos,1.0f);
        totalPosition += localPosition * weights[i];
        
        vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
        totalNormal += localNormal * weights[i];

        totalWeight += weights[i]; // สะสมน้ำหนัก
    }

    // --- จุดสำคัญที่สุด (แก้รูดำ) ---
    // ถ้าน้ำหนักรวมเป็น 0 (แปลว่าจุดนี้ไม่มีกระดูกคุมเลย)
    // ให้ใช้ค่าเดิมๆ (aPos) แทนที่จะยุบหายไป
    if (totalWeight <= 0.001) {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }
    // ----------------------------
    
    gl_Position =  projection * view * model * totalPosition;
    FragPos = vec3(model * totalPosition);
    TexCoords = aTexCoords;
    Normal = mat3(transpose(inverse(model))) * totalNormal; 
}