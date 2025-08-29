#version 420

uniform sampler2D s_texture;
uniform sampler2D s_lighting;

//uniform float lightIntensity;
uniform float lightToggle = 1.0f;
uniform float colorToggle = 1.0f;
uniform float lightColorToggle = 1.0f;
uniform float shadowMapToggle = 1.0f;
uniform float viewTextures = 1.0f;
uniform int lightCount = 0;
uniform bool hideOtherLights = false;

uniform bool fogEnabled;
uniform float fogNear = 0;
uniform float fogFar = 1;
//uniform float fogExp = 0.5;
uniform vec4 fogColor = vec4(1,1,1,1);

in vec2 texCoord;
in vec2 texCoord2;
in vec3 normal;
in vec4 color;
in vec3 mult;

out vec4 fragColor;
in vec3 fragPos;

#define MAX_FALLOFF 10
#define NR_LIGHTS 1

struct Light {
    vec3 position;
    vec3 color;
	int type;
	int falloff_style;
	vec2 falloff[MAX_FALLOFF];
	int falloff_count;
	float range;
	float intensity;
	float cutoff;
	bool diffuseLighting;
	vec3 direction;
	float width;
};

uniform Light lights[NR_LIGHTS];

vec3 CalcPointLight(Light light, vec3 normal, vec3 inFragPos);

void main()
{
	vec4 texture = vec4(1,1,1,1);

	vec4 texColor = texture2D(s_texture, texCoord);
	texture = mix(vec4(1,1,1,texColor.a), texColor, viewTextures);
	if(texture.a < 0.1)
		discard;
	
	vec3 light = vec3(0, 0, 0);
	
	for (int i = 0; i < lightCount; i++)
        light.rgb += CalcPointLight(lights[i], normalize(normal), fragPos);
	
	texture.rgb *= mult;
	texture.rgb *= max(color, colorToggle).rgb;
	texture.rgb *= max(texture2D(s_lighting, texCoord2).a, shadowMapToggle);
	
	if (!hideOtherLights) {
		texture.rgb += clamp(texture2D(s_lighting, texCoord2).rgb, 0.0, 1.0) * lightColorToggle;
		texture.rgb += light.rgb;
	}
	else {		
		if (lightCount > 0 && light.rgb != vec3(0, 0, 0))
			texture.rgb += light.rgb;
		else
			texture.rgb += clamp(texture2D(s_lighting, texCoord2).rgb, 0.0, 1.0) * lightColorToggle;
	}
	
	if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		texture = mix(texture, fogColor, fogAmount);
	}	
	
	fragColor = texture;
}

// calculates the color when using a point light.
vec3 CalcPointLight(Light light, vec3 normal, vec3 inFragPos)
{
	vec3 lightDir = normalize(light.position - inFragPos);
    float distance = length(light.position - inFragPos);
	
	if (light.type == 2)	// sun, ignored
		return vec3(0, 0, 0);
	
	float attenuation = 0.0f;
	float dotProduct = abs(dot(normalize(normal), lightDir));
	
	if (light.falloff_style == 4) {
		float kC = 1;
		float kL = 2.0f / light.range;
		float kQ = 1.0f / (light.range * light.range);
		float maxChannel = max(max(light.color.r, light.color.g), light.color.b);
		float realRange = (-kL + sqrt(kL * kL - 4 * kQ * (kC - 128.0f * maxChannel * light.intensity))) / (2 * kQ);
		
		if (distance > realRange)
			return vec3(0, 0, 0);
		
		float d = max(distance - light.range, 0.0f);
		float denom = d / light.range + 1;
		attenuation = light.intensity / (denom * denom);
		if (light.cutoff > 0)
			attenuation = max(0.0f, (attenuation - light.cutoff) / (1 - light.cutoff));
	}
	else {
		if (distance > light.range)
			return vec3(0, 0, 0);
		
		float d = distance / light.range;
		
		if (light.falloff_style == 0) {
			attenuation = clamp(1.0f - pow(d, light.cutoff), 0.0f, 1.0f);
		}
		else if (light.falloff_style == 1) {	// Spline
			int n = light.falloff_count;
			float h[MAX_FALLOFF];
			float F[MAX_FALLOFF];
			float s[MAX_FALLOFF];
			float m[MAX_FALLOFF * MAX_FALLOFF];
			
			for (int i = n - 1; i > 0; i--) {
				h[i] = 0.0f;
				F[i] = 0.0f;
				s[i] = 0.0f;
			}
			
			for (int i = n - 1; i > 0; i--)
			{
				F[i] = (light.falloff[i].y - light.falloff[i - 1].y) / (light.falloff[i].x - light.falloff[i - 1].x);
				h[i - 1] = light.falloff[i].x - light.falloff[i - 1].x;
			}
			
			for (int i = 1; i < n - 1; i++)
			{
				m[i * MAX_FALLOFF + i] = 2 * (h[i - 1] + h[i]);
				if (i != 1)
				{
					m[i * MAX_FALLOFF + i - 1] = h[i - 1];
					m[(i - 1) * MAX_FALLOFF + i] = h[i - 1];
				}
				m[i * MAX_FALLOFF + n - 1] = 6 * (F[i + 1] - F[i]);
			}
			
			for (int i = 1; i < n - 2; i++)
			{
				float temp = (m[(i + 1) * MAX_FALLOFF + i] / m[i * MAX_FALLOFF + i]);
				for (int j = 1; j <= n - 1; j++)
					m[(i + 1) * MAX_FALLOFF + j] -= temp * m[i * MAX_FALLOFF + j];
			}
			float sum;
			for (int i = n - 2; i > 0; i--)
			{
				sum = 0;
				for (int j = i; j <= n - 2; j++)
					sum += m[i * MAX_FALLOFF + j] * s[j];
				s[i] = (m[i * MAX_FALLOFF + n - 1] - sum) / m[i * MAX_FALLOFF + i];
			}
			for (int i = 0; i < n - 1; i++) {
				if (light.falloff[i].x <= d && d <= light.falloff[i + 1].x)
				{
					float a = (s[i + 1] - s[i]) / (6 * h[i]);
					float b = s[i] / 2;
					float c = (light.falloff[i + 1].y - light.falloff[i].y) / h[i] - (2 * h[i] * s[i] + s[i + 1] * h[i]) / 6;
					float e = light.falloff[i].y;
					sum = a * pow((d - light.falloff[i].x), 3.0f) + b * pow((d - light.falloff[i].x), 2.0f) + c * (d - light.falloff[i].x) + e;
				}
			}
			attenuation = clamp(sum, 0.0f, 1.0f);
		}
		else if (light.falloff_style == 2) {	// Lagrange
			for (int i = 0; i < light.falloff_count; i++) {
				float term = light.falloff[i].y;
				
				for (int j = 1; j < light.falloff_count; j++) {
					if (j != i)
						term = term * (d - light.falloff[j].x) / (light.falloff[i].x - light.falloff[j].x);
				}
				
				attenuation += term;
			}
			
			attenuation = clamp(attenuation, 0.0f, 1.0f);
		}
		else if (light.falloff_style == 3) {	// Linear
			vec2 before = vec2(-9999, -9999);
			vec2 after = vec2(9999, 9999);
			
			for (int i = 0; i < light.falloff_count; i++) {
				vec2 p = light.falloff[i];
				
				if (p.x <= d && d - p.x < d - before.x)
					before = p;
				if (p.x >= d && p.x - d < after.x - d)
					after = p;
			}
			
			float diff = (d - before.x) / (after.x - before.x);
			attenuation = before.y + diff * (after.y - before.y);
			attenuation = clamp(attenuation, 0.0f, 1.0f);
		}
		else if (light.falloff_style == 5) {	// S Curve
			d = 2.0f * d - 1.0f;
			
			if (d <= 0.0f)
				attenuation = -1.0f / (1.0f + 1.5f * - d + 1.5f * d * d) * (1.0f + d) + 2.0f;
			else
				attenuation = 1.0f / (1.0f + 1.5f * d + 1.5f * d * d) * (1.0f - d);
		}
		else {
			attenuation = 0.0f;
		}
	}
	
	if (light.type == 1) {	// spot
		float dp = dot(lightDir, -light.direction);
		
		if (dp < (1 - light.width))
			attenuation = 0.0f;
		else {
			float fac = 1.0f - ((1.0f - abs(dp)) / light.width);
			attenuation *= fac;
		}
	}
	
	if (light.diffuseLighting) {
		attenuation *= dotProduct;
	}
	
	attenuation *= light.intensity;
	
	return light.color * attenuation;
}