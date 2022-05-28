#version 420
out vec4 fragColor;
in vec3 normal;
in vec2 texCoord;

uniform float lightMin = 0.5;
uniform sampler2D s_texture;
uniform float textureFac = 0.0f;
uniform vec4 color;
uniform vec4 colorMult = vec4(1,1,1,1);

void main()
{
    vec4 c = mix(color, texture2D(s_texture, texCoord), textureFac) * colorMult;

    fragColor = vec4(c.rgb * clamp(dot(normal, normalize(vec3(1,1,1))), lightMin, 1.0), c.a);
}