#version 420

layout (location = 0) out vec4 frag;

uniform sampler2D s_texture;
uniform sampler2D s_depth_fbo;
uniform sampler2D s_depth_outline;

uniform vec2 iResolution;
uniform vec4 selectionOverlay;
uniform vec4 selectionOutline;
uniform vec4 selectionOccludedOverlay;
uniform vec4 selectionOccludedOutline;

in vec2 uvTex;

void main()
{
	float step_u = 1.0 / iResolution.x;
	float step_v = 1.0 / iResolution.y;
	vec2 texelSize = 1.0 / vec2(iResolution.x, iResolution.y);
	
	vec4 current_texel = texture(s_texture, uvTex);
	
	// 0 depth = close to camera
	// 1 depth = furthest possible from camera
	float depth_fbo = texture(s_depth_fbo, uvTex).r;
	float depth_outline = texture(s_depth_outline, uvTex).r;
	
	bool current_hasColor = false;
	bool current_occluded = false;
	
	frag = vec4(0, 0, 0, 0);
	
	if ((current_texel.r >= 0 || current_texel.g >= 0) && current_texel.a >= 1.0) {
		current_hasColor = true;
		frag = selectionOverlay;
		
		if (depth_fbo < depth_outline) {
			frag = selectionOccludedOverlay;
		}
	}
	
	if (depth_fbo < depth_outline) {
		current_occluded = true;
	}
	
	int thickness = 1;
	int result = 0;
	
	for (int x = -thickness; x <= thickness; x++) {
		for (int y = -thickness; y <= thickness; y++) {
			if (x == 0 && y == 0)
				continue;
			
			vec2 target_uv = uvTex + vec2(x, y) * texelSize;
			vec4 target_texel = texture(s_texture, target_uv);
			
			// Nearby texel has color
			if ((target_texel.r >= 0.0 || target_texel.g >= 0.0) && target_texel.a >= 1.0) {
				float target_depth_fbo = texture(s_depth_fbo, target_uv).r;
				float target_depth_outline = texture(s_depth_outline, target_uv).r;
				
				bool target_occluded = target_depth_fbo < target_depth_outline;
				
				// Generic outline
				if (!current_hasColor) {
					result = max(result, target_occluded ? 1 : 2);
				}
				
				// Outline for meshes intersection
				if (current_texel != target_texel &&
					depth_outline > target_depth_outline) {
					result = max(result, (target_occluded && current_occluded) ? 1 : 2);
				}
			}
		}
		
		// The "red" visible outline should take priority over the occluded outline
		if (result == 2)
			frag = selectionOutline;
		else if (result == 1)
			frag = selectionOccludedOutline;
	}
}