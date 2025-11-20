#version 330 core
out vec4 FragColor;

uniform vec4 color;

void main()
{
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    FragColor = color;
    FragColor.a *= (1.0 - (dist / 0.5)); // Fade out at the edges
}