#version 420

uniform sampler2D s_texture;
uniform float selection;
varying vec2 texCoord;

out vec4 fragColor;



void main()
{
	vec4 color = texture2D(s_texture, texCoord);
	if(color.a < 0.1)
		discard;
	fragColor = color;
}