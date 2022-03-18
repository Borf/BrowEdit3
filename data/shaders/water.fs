#version 420
out vec4 fragColor;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D s_texture;

void main()
{
    vec4 color = texture2D(s_texture, texCoord); 
    color.a = 0.25;
    fragColor = color;
}