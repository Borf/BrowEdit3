
#extension GL_ARB_bindless_texture : require
layout(local_size_x = 256) in;

struct GpuTriangle
{
	vec4 vertices[3];
	vec4 uvs[3];
	vec4 normal;

	float shadowStrength;
	int texId;
	uvec2 textureHandle;
};

struct BvhNode
{
	vec4 aabbMin;
	vec4 aabbMax;
	vec4 center;

	int left;
	int right;

	uint firstTriangle;
	int triangleCount;
};

struct GpuCubeTile
{
	vec4 normals[4];
	
	float heights[4];
	
	uint x;
	uint y;
	int lightIndex;
	int padding1;
};

struct GpuCube
{
	vec4 h1;
	vec4 h2;
	vec4 h3;
	vec4 h4;
	int tileUp;
	int tileFront;
	int tileSide;
	int padding1;
};

struct LightmapSample
{
	uint cubeTileIndex;
	uint direction;
};

#define MAX_FALLOFF 10

struct Light
{
	vec4 position;
	vec4 color;

	int type;
	int falloff_style;
	int falloff_count;
	float range;

	float intensity;
	float cutoff;
	int diffuseLighting;
	int affectShadowMap;

	vec4 falloff[MAX_FALLOFF];

	vec4 direction;
	float width;
	int affectLightmap;
	int shadowTerrain;
	int noTerrainShading;
	
	int sunMatchRswDirection;
	float realRange;
	int padding[2];
};

struct LightGridCell
{
	uint first;
	uint count;
};

struct LightmapResult
{
	vec4 color;
};

layout(std430, binding = 0) readonly buffer TriangleBuffer
{
	GpuTriangle triangles[];
};

layout(std430, binding = 1) readonly buffer SampleBuffer
{
	LightmapSample samples[];
};

layout(std430, binding = 2) readonly buffer LightBuffer
{
	Light lights[];
};

layout(std430, binding = 3) readonly buffer LightGridBuffer
{
	LightGridCell cells[];
};

layout(std430, binding = 4) readonly buffer LightIndexBuffer
{
	uint lightIndices[];
};

layout(std430, binding = 5) readonly buffer BvhNodeBuffer
{
	BvhNode nodes[];
};

layout(std430, binding = 6) readonly buffer TriangleIndexBuffer
{
	uint triangleIndices[];
};

layout(std430, binding = 7) readonly buffer CubeBuffer
{
	GpuCubeTile cubes[];
};

layout(std430, binding = 8) readonly buffer GpuCubeBuffer
{
	GpuCube gnd_cubes[];
};

layout(std430, binding = 9) buffer ResultBuffer
{
	LightmapResult results[];
};

uniform bool additiveShadow;
uniform bool shadows;
uniform float lightmapAmbient;
uniform int quality;
uniform int lightmapWidth;
uniform int lightmapHeight;
uniform int gndHeight;
uniform int gndWidth;

#ifdef SUN
uniform vec3 lightDirection;
#endif

bool intersectTriangle(vec3 origin, vec3 direction, vec3 v0, vec3 v1, vec3 v2, out float t, out float u, out float v) {
	const float epsilon = 0.000001;

	vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;

	vec3 p = cross(direction, edge2);
	float det = dot(edge1, p);

	if (abs(det) < epsilon)
		return false;

	float invDet = 1.0 / det;

	vec3 s = origin - v0;

	u = dot(s, p) * invDet;

	if (u < 0.0 || u > 1.0)
		return false;

	vec3 q = cross(s, edge1);

	v = dot(direction, q) * invDet;

	if (v < 0.0 || u + v > 1.0)
		return false;

	t = dot(edge2, q) * invDet;

	return t > epsilon;
}

bool intersectAabb(
	vec3 origin,
	vec3 direction,
	vec3 boundsMin,
	vec3 boundsMax,
	float minDistance,
	float maxDistance)
{
	vec3 invDir = 1.0 / direction;

	vec3 t0 = (boundsMin - origin) * invDir;
	vec3 t1 = (boundsMax - origin) * invDir;

	vec3 tMin = min(t0, t1);
	vec3 tMax = max(t0, t1);

	float entry = max(max(tMin.x, tMin.y), tMin.z);
	float exit = min(min(tMax.x, tMax.y), tMax.z);

	return exit >= max(entry, minDistance) &&
		   entry <= maxDistance;
}

float traceShadow(vec3 origin, vec3 direction, float minDistance, float maxDistance, bool shadowTerrain)
{
	if (nodes.length() == 0)
		return 0.0;

	int stack[64];
	int stackPtr = 0;
	float shadowStrength = 0.0;
	stack[stackPtr++] = 0;

	while (stackPtr > 0)
	{
		BvhNode node = nodes[stack[--stackPtr]];

		if (!intersectAabb(origin, direction, node.aabbMin.xyz, node.aabbMax.xyz, minDistance, maxDistance)) {
			continue;
		}

		if (node.triangleCount != 0)
		{
			for (int i = 0; i < node.triangleCount; i++)
			{
				GpuTriangle triangle = triangles[triangleIndices[node.firstTriangle + uint(i)]];
				
				if (!shadowTerrain && triangle.texId == -1)
					continue;
				
				float t;
				float u;
				float v;
				
				if (intersectTriangle(
					origin,
					direction,
					triangle.vertices[0].xyz,
					triangle.vertices[1].xyz,
					triangle.vertices[2].xyz,
					t, u, v))
				{
					if (t >= minDistance && t <= maxDistance)
					{
						if (triangle.texId < 0)
							return 1.0;
						
						vec2 uv = triangle.uvs[0].xy * (1.0 - u - v) + triangle.uvs[1].xy * u + triangle.uvs[2].xy * v;
						
						uv = fract(uv);
						
						vec4 tColor = texture(sampler2D(triangle.textureHandle), uv);
						
						if (tColor.r > 0.968 && tColor.g < 0.0313 && tColor.b > 0.968)
							continue;
						
						shadowStrength += tColor.a;
						
						if (shadowStrength >= 1.0)
							return 1.0;
						
						continue;
					}
				}
			}

			continue;
		}

		// Defensive overflow check. A balanced BVH should never reach this.
		if (stackPtr + 2 > 64)
			return 0.0;
		
		if (node.right != -1)
			stack[stackPtr++] = node.right;
		if (node.left != -1)
			stack[stackPtr++] = node.left;
	}

	return shadowStrength;
}

vec3 TriangleBarycentricCoords(vec2 a, vec2 b, vec2 c, vec2 p)
{
    vec2 v0 = b - a;
    vec2 v1 = c - a;
    vec2 v2 = p - a;
    float denom = v0.x * v1.y - v1.x * v0.y;
    
    float v = (v2.x * v1.y - v1.x * v2.y) / denom;
    float w = (v0.x * v2.y - v2.x * v0.y) / denom;
    float u = 1.0 - v - w;
    
    return vec3(u, v, w);
}

bool TriangleContainsPoint(vec2 a, vec2 b, vec2 c, vec2 p) {
	bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) <= 0.0f;
	bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) <= 0.0f;
	bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) <= 0.0f;
	return ((b1 == b2) && (b2 == b3));
}

vec4 CalcLight_Local(uint lightIndex, vec3 normal, vec3 origin);

#ifdef SUN
vec4 CalcLight_Sun(uint lightIndex, vec3 normal, vec3 origin);
#endif

void main()
{
	uint sampleIndex = gl_GlobalInvocationID.x;
	
	if (sampleIndex >= samples.length())
		return;
	
	LightmapSample lSample = samples[sampleIndex];
	
	GpuCubeTile cube = cubes[lSample.cubeTileIndex];
	uint direction = lSample.direction;
	
	uint sampleWidth = uint(lightmapWidth - 2);
	uint sampleHeight = uint(lightmapHeight - 2);
	
	float sampleWidth_inv = 1.0 / float(lightmapWidth - 2);
	float sampleHeight_inv = 1.0 / float(lightmapHeight - 2);
	
	uint xx = (sampleIndex / sampleHeight) % sampleWidth + 1u;
	uint yy = sampleIndex % sampleHeight + 1u;
	
	int count = quality * quality;
	uint x = cube.x;
	uint y = cube.y;
	
	if (cube.lightIndex < 0)
		return;
	
	LightGridCell cell = cells[cube.lightIndex];
	
	if (cell.count == 0)
		return;
	
	vec4 totalColor = vec4(0.0);
	
	float sx = 10.0 / (lightmapWidth - 2);
	float sy = 10.0 / (lightmapHeight - 2);
	float invQuality = 1.0 / float(quality);
	
	vec3 norm3 = vec3(0);
	vec3 position = vec3(0);
	
	vec2 v1;
	vec2 v2;
	vec2 v3;
	vec2 v4;
	
	if (direction == 0) {
		v1 = vec2((x + 0) * 10, 10 * gndHeight - (y + 1) * 10 + 10);
		v2 = vec2((x + 0) * 10, 10 * gndHeight - (y + 0) * 10 + 10);
		v3 = vec2((x + 1) * 10, 10 * gndHeight - (y + 0) * 10 + 10);
		v4 = vec2((x + 1) * 10, 10 * gndHeight - (y + 1) * 10 + 10);
	}
	
	for (int qx = 0; qx < quality; qx++) {
		for (int qy = 0; qy < quality; qy++) {
			float xxx = (float(qx) + 0.5) * invQuality;
			float yyy = (float(qy) + 0.5) * invQuality;
			
			//float xxx = float(qx) * invQuality + invQuality * 0.5;
			//float yyy = float(qy) * invQuality + invQuality * 0.5;
			
			if (direction == 0) {
				vec2 p = vec2(10 * x + sx * ((xx + xxx) - 1), 10 * gndHeight + 10 - 10 * y - sy * ((yy + yyy) - 1));
				
				float h = 0.0;
				vec4 normal = vec4(0);
				
				if (TriangleContainsPoint(v1, v2, v3, p)) {
					vec3 uvw = TriangleBarycentricCoords(v1, v2, v3, p);
					h = uvw.x * -cube.heights[2] + uvw.y * -cube.heights[0] + uvw.z * -cube.heights[1];
					normal = uvw.x * cube.normals[2] + uvw.y * cube.normals[0] + uvw.z * cube.normals[1];
				}
				else if (TriangleContainsPoint(v3, v4, v1, p)) {
					vec3 uvw = TriangleBarycentricCoords(v3, v4, v1, p);
					h = uvw.x * -cube.heights[1] + uvw.y * -cube.heights[3] + uvw.z * -cube.heights[2];
					normal = uvw.x * cube.normals[1] + uvw.y * cube.normals[3] + uvw.z * cube.normals[2];
				}
				
				normal.y *= -1;
				normal.z *= -1;
				normal = normalize(normal);
				
				norm3 = normal.xyz; 
				position = vec3(p.x, h, p.y);
			}
			else if (direction == 1) {
				float h1 = mix(cube.heights[0], cube.heights[1], ((xx + xxx) - 1) * sampleWidth_inv);
				float h2 = mix(cube.heights[2], cube.heights[3], ((xx + xxx) - 1) * sampleWidth_inv);
				float h = mix(h1, h2, ((yy + yyy) - 1) * sampleHeight_inv);
	
				position = vec3(10 * x + sx * ((xx + xxx) - 1), -h, 10 * gndHeight - 10 * y);
				norm3 = vec3(0, 0, 1);
	
				if (h1 < h2)
					norm3 = -norm3;
			}
			else if (direction == 2) {
				float h1 = mix(cube.heights[0], cube.heights[1], ((xx + xxx) - 1) * sampleWidth_inv);
				float h2 = mix(cube.heights[2], cube.heights[3], ((xx + xxx) - 1) * sampleWidth_inv);
				float h = mix(h1, h2, ((yy + yyy) - 1) * sampleHeight_inv);
	
				position = vec3(10 * x + 10, -h, 10 * gndHeight - 10 * y + sy * ((xx + xxx) - 1));
				norm3 = vec3(-1, 0, 0);
	
				if (h1 < h2)
					norm3 = -norm3;
			}
			
			float intensity = 0;
			
			vec4 accumColor = vec4(0.0);
			
			for (uint i = 0; i < cell.count; i++) {
				vec4 lightColor;			
#ifdef SUN
				lightColor = CalcLight_Sun(lightIndices[cell.first + i], norm3, position);
#else
				lightColor = CalcLight_Local(lightIndices[cell.first + i], norm3, position);
#endif
				accumColor += lightColor;
			}
			
			totalColor += accumColor;
		}
	}
	
	results[sampleIndex].color += totalColor;
}

float calcAttenuation_default(float d, float cutoff)
{
	return clamp(1.0f - pow(d, cutoff), 0.0f, 1.0f);
}

float calcAttenuation_linear(float d, uint lightIndex)
{
	Light light = lights[lightIndex];
	vec2 before = vec2(-9999, -9999);
	vec2 after = vec2(9999, 9999);
	
	for (int i = 0; i < light.falloff_count; i++) {
		vec2 p = light.falloff[i].xy;
		
		if (p.x <= d && d - p.x < d - before.x)
			before = p;
		if (p.x >= d && p.x - d < after.x - d)
			after = p;
	}
	
	float diff = (d - before.x) / (after.x - before.x);
	return clamp(before.y + diff * (after.y - before.y), 0.0f, 1.0f);
}

float calcAttenuation_lagrange(float d, uint lightIndex)
{
	Light light = lights[lightIndex];
	float attenuation = 0.0;
	
	for (int i = 0; i < light.falloff_count; i++) {
		float term = light.falloff[i].y;
		
		for (int j = 1; j < light.falloff_count; j++) {
			if (j != i)
				term = term * (d - light.falloff[j].x) / (light.falloff[i].x - light.falloff[j].x);
		}
		
		attenuation += term;
	}
	
	return clamp(attenuation, 0.0f, 1.0f);
}

float calcAttenuation_magic(float distance, uint lightIndex)
{
	Light light = lights[lightIndex];
	float d = max(distance - light.range, 0.0f);
	float denom = d / light.range + 1;
	float attenuation = light.intensity / (denom * denom);
	if (light.cutoff > 0)
		attenuation = max(0.0f, (attenuation - light.cutoff) / (1 - light.cutoff));
	return attenuation;
}

float calcAttenuation_spline(float d, uint lightIndex)
{
	Light light = lights[lightIndex];
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
	return clamp(sum, 0.0f, 1.0f);
}

vec4 CalcLight_Post(uint lightIndex, vec3 lightDir, vec3 origin, float attenuation, float dotProduct, float distance)
{
	Light light = lights[lightIndex];
	float shadowStrength = 0.0;
	
	// Collision check
	if (shadows) {
		if (shadowStrength < 1 && light.shadowTerrain != 0) {
			shadowStrength = traceShadow(origin, lightDir, 0.001, distance, light.shadowTerrain != 0);
		}
	}
	
	shadowStrength = min(shadowStrength, 1.0);
	
	if (light.diffuseLighting != 0) {
		if (light.type != 2 || additiveShadow)
			attenuation = attenuation * dotProduct;
		else
			attenuation = attenuation * (1.0 - dotProduct);
	}
	
	vec4 color = vec4(0.0);
	if (light.affectShadowMap != 0) {
		if (additiveShadow)
			color.w += ((1 - shadowStrength) * attenuation * light.intensity);
		else
			color.w -= shadowStrength * attenuation * light.intensity;
	}
	if (light.affectLightmap != 0)
		color.xyz += (1 - shadowStrength) * attenuation * light.color.xyz * light.intensity;
	
	return color;
}

#ifdef SUN
vec4 CalcLight_Sun(uint lightIndex, vec3 normal, vec3 origin)
{
	Light light = lights[lightIndex];
	vec3 lightDir;
	
	if (light.sunMatchRswDirection != 0)
		lightDir = lightDirection;
	else
		lightDir = light.direction.xyz;
	
	float dotProduct = light.noTerrainShading != 0 ? 1.0 : dot(normal, lightDir);
	
	if (dotProduct <= 0.0) {
		if (!additiveShadow && light.affectShadowMap != 0) {
			return vec4(0.0, 0.0, 0.0, -light.intensity);
		}
		
		return vec4(0.0);
	}
	
	return CalcLight_Post(lightIndex, lightDir, origin, 1.0, dotProduct, 999999);
}
#endif

vec4 CalcLight_Local(uint lightIndex, vec3 normal, vec3 origin)
{
	Light light = lights[lightIndex];
	vec3 lightDir = normalize(light.position.xyz - origin);
	
	float distance = length(light.position.xyz - origin);
	float attenuation = 0.0f;
	float dotProduct = light.noTerrainShading != 0 ? 1.0 : dot(normal, lightDir);
	
	if (dotProduct <= 0.0) {
		return vec4(0.0);
	}
	
	if (distance > light.realRange)
		return vec4(0.0);
	
	float d = distance / light.range;
	
#if FALLOFF_STYLE == 0
	attenuation = calcAttenuation_default(d, light.cutoff);
#elif FALLOFF_STYLE == 1
	attenuation = calcAttenuation_spline(d, lightIndex);
#elif FALLOFF_STYLE == 2
	attenuation = calcAttenuation_lagrange(d, lightIndex);
#elif FALLOFF_STYLE == 3
	attenuation = calcAttenuation_linear(d, lightIndex);
#elif FALLOFF_STYLE == 4
	attenuation = calcAttenuation_magic(distance, lightIndex);
#elif FALLOFF_STYLE == 5
	d = 2.0f * d - 1.0f;

	if (d <= 0.0f)
		attenuation = -1.0f / (1.0f + 1.5f * - d + 1.5f * d * d) * (1.0f + d) + 2.0f;
	else
		attenuation = 1.0f / (1.0f + 1.5f * d + 1.5f * d * d) * (1.0f - d);
#endif
	
	if (light.type == 1) {	// spot
		float dp = dot(lightDir, -light.direction.xyz);
		
		if (dp < (1 - light.width))
			attenuation = 0.0f;
		else {
			float fac = 1.0f - ((1.0f - abs(dp)) / light.width);
			attenuation *= fac;
		}
	}
	
	return CalcLight_Post(lightIndex, lightDir, origin, attenuation, dotProduct, distance);
}
