#version 420
out vec4 fragColor;
in vec3 normal;

uniform sampler2D s_texture;
uniform float textureFac = 0.0f;
uniform vec4 color;

void main()
{
    fragColor = vec4(color.rgb * clamp(dot(normal, normalize(vec3(1,1,1))), 0.5, 1.0), color.a);
}