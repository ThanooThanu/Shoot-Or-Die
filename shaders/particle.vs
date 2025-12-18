#version 330 core
layout (location = 0) in float aDummy; // We don't use vertex attributes

uniform vec3 offset;
uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * vec4(offset, 1.0);
    gl_PointSize = 10.0; // Size of the particle
}