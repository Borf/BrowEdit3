#version 420

uniform sampler2D s_texture;
uniform vec4 color = vec4(1,1,1,1);
in vec2 texCoord;
in float alpha;
out vec4 fragColor;



void main()
{
	vec4 outColor = texture2D(s_texture, texCoord);
	//output.a = (output.r + output.g + output.b) / 3;
	outColor *= color;
	outColor.rgb *= alpha;
	if(outColor.a < 0.1)
		discard;
	fragColor = outColor;
}