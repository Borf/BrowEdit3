#version 420
out vec4 fragColor;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D s_texture;
uniform sampler2D s_textureNext;
uniform float frameTime;

void main()
{
    vec4 color = texture2D(s_texture, texCoord); 
    vec4 color2 = texture2D(s_textureNext, texCoord); 
    color.a = 0.564;
    color2.a = 0.564;
    fragColor = mix(color, color2, frameTime);
}