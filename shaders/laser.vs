#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Transform the vertex position from model space to clip space
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}