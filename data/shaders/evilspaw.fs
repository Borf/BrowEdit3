#version 420

uniform sampler2DArray s_texture;
in vec2 texCoord;
out vec4 fragColor;
uniform float layerIndex;
uniform bool selection;
uniform vec4 selectionColor = vec4(1,0,0,1);

void main()
{
	if (selection) {
		fragColor = selectionColor;
	}
	else {
		fragColor = texture(s_texture, vec3(texCoord, floor(layerIndex)));
		
		if (fragColor.a <= 0)
			discard;
	}
}