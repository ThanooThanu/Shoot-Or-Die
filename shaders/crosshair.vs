#version 330 core
layout (location = 0) in vec2 aPos;

void main()
{
    // The crosshair vertices are already in screen space,
    // so we just pass them directly to gl_Position.
    // The z-coordinate is 0.0, and w is 1.0.
    gl_Position = vec4(aPos, 0.0, 1.0);
}