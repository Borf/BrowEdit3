#pragma once

#include <thread>
#include <vector>
#include <string>
#include <unordered_set>
#include <functional>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <browedit/gl/SSBO.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>

class Map;
class BrowEdit;
class Node;
class LightmapperShader;
namespace math { class Ray; }

class LightmapperGpu
{
public:
	#pragma pack(push, 4)
	struct GpuCube
	{
		glm::vec4 vertices[4];
		int tileUp;
		int tileFront;
		int tileSide;
		int padding1;
	};

	struct GpuCubeTile
	{
		glm::vec4 normals[4];

		float heights[4];

		uint32_t x;
		uint32_t y;
		int lightIndex;
		int padding1;
	};

	struct GpuBvhNode
	{
		glm::vec4 aabbMin;
		glm::vec4 aabbMax;
		glm::vec4 center;

		int32_t left;
		int32_t right;

		uint32_t firstTriangle;
		int32_t triangleCount;
	};

	struct GpuTriangle
	{
		glm::vec4 vertices[3];
		glm::vec4 uvs[3];
		glm::vec4 normal;

		float shadowStrength;
		int texId;
		uint64_t textureHandle;
	};

	struct LightmapResult
	{
		glm::vec4 color;
	};

	struct LightmapSample
	{
		uint32_t cubeTileIndex;
		uint32_t direction;
	};

	struct LightGridCell
	{
		uint32_t first;
		uint32_t count;
	};

	struct LightmapLight
	{
		glm::vec4 position;
		glm::vec4 color;

		int type;
		int falloffStyle;
		int falloffCount;
		float range;

		float intensity;
		float cutoff;
		int diffuseLighting;
		int affectShadowMap;

		glm::vec4 falloff[10];

		glm::vec4 direction;
		float width;
		int affectLightmap;
		int shadowTerrain;
		int noTerrainShading;

		int sunMatchRswDirection;
		float realRange;
		int padding[2];
	};
	#pragma pack(pop)

private:
	BrowEdit* browEdit;
	Gnd* gnd;
	Rsw* rsw;
	Rsw::LightmapSettings* settings;
	std::vector<bool> ignoredTextureIdx;

	// The RSW light direction.
	glm::vec3 lightDirection;

	// All the map lights and their associated RswObject (for position).
	std::vector<std::pair<RswObject*, RswLight*>> cpuLights;
	
	// The entire GND cube data.
	std::vector<GpuCube> gpuCubes;

	// The work sample associated with the current compute shader invocation.
	std::vector<GpuCubeTile> gpuCubeTiles;

	// BVH that contains all triangles (both models and ground) processed by the lightmapper.
	Bvh bvh;

	// BVH nodes for the shader.
	std::vector<GpuBvhNode> gpuBvhNodes;

	// The samples/requests sent to the GPU. There are (lightmapHeight - 2) * (lightmapWidth - 2) samples sent per tile.
	std::vector<LightmapSample> samples;

	// The triangles used by the GPU for the map.
	std::vector<GpuTriangle> gpuTriangles;

	// The lights used by the GPU.
	std::vector<LightmapLight> gpuLights;
	std::vector<LightGridCell> lightGridCells;
	std::vector<uint32_t> lightIndices;

	// Output to store the compute shader results.
	std::vector<LightmapResult> results;

	// Texture IDs to ARB bindless texture handles. Used by the GPU to access the textures directly.
	std::unordered_map<int, GLuint64> textureIds;

	std::vector<LightmapperShader*> shaders;
	gl::SSBO<GpuTriangle>* gpuTriangleSsbo = nullptr;
	gl::SSBO<LightmapSample>* sampleSsbo = nullptr;
	gl::SSBO<LightmapLight>* lightSsbo = nullptr;
	gl::SSBO<LightGridCell>* lightGridSsbo = nullptr;
	gl::SSBO<uint32_t>* lightIndicesSsbo = nullptr;
	gl::SSBO<GpuBvhNode>* gpuBvhNodeSsbo = nullptr;
	gl::SSBO<uint32_t>* triangleIndicesSsbo = nullptr;
	gl::SSBO<GpuCubeTile>* gpuCubeTileSsbo = nullptr;
	gl::SSBO<GpuCube>* gpuCubesSsbo = nullptr;
	gl::SSBO<LightmapResult>* resultSsbo = nullptr;

	// This is the same value as the lightmapper settings, but capped between [1..5]. Going higher is rather pointless and it could risk stalling the GPU.
	int quality = 1;
public:
	Map* map;
	bool dirty = true;

	LightmapperGpu(Map* map, BrowEdit* browEdit);

	static bool canRunGpu();

	void run();
	void end();
private:
	void disableWarning131186();
	void gatherLights();
	int gatherGpuCubes();
	void gatherCollisionTriangles();
	void gatherModelTriangles();
	void gatherGpuTriangles();
	void gatherGpuNodes();
	void gatherLightData(int falloffStyle);

	void calculateLightDirection();

	void initResultData();
	void initShaders();
	void initIgnoreTextures();
	void createSSBOs();
	void setDataSSBOs();
	void bindSSBOs();
	void executeComputeShaders();
	void applyLightmapResult();

	void traverseGnd(std::function<void(Gnd::Cube*, int, int, int)> visitGnd);

	glm::vec3 roPosToWorldPos(glm::vec3& p);
};

