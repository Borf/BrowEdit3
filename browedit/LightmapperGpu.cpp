#include <Windows.h>
#include "LightmapperGpu.h"
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/shaders/LightmapperShader.h>
#include <browedit/gl/Texture.h>

#include <glm/glm.hpp>
#include <iostream>
#include <imgui_internal.h>

std::chrono::steady_clock::time_point debug_startTime;

LightmapperGpu::LightmapperGpu(Map* map, BrowEdit* browEdit) : map(map), browEdit(browEdit)
{
	gnd = map->rootNode->getComponent<Gnd>();
	rsw = map->rootNode->getComponent<Rsw>();

	settings = &rsw->lightmapSettings;
	quality = glm::min(5, settings->quality);
}

void startDebugTimer()
{
	debug_startTime = std::chrono::steady_clock::now();
}

void stopDebugTimer(std::string message)
{
	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - debug_startTime);

	std::cout << "Lightmapper: " << message << std::to_string(duration.count()) + "ms" << std::endl;
}

bool LightmapperGpu::canRunGpu()
{
	bool runOnGpu = true;

	if (!glfwExtensionSupported("GL_ARB_bindless_texture")) {
		std::cout << "The GPU lightmapper requires GL_ARB_bindless_texture which is not supported." << std::endl;
		runOnGpu = false;
	}

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	if (major < 4 || (major == 4 && minor < 3)) {
		std::cout << "The GPU lightmapper requires OpenGL 4.3" << std::endl;
		runOnGpu = false;
	}

	return runOnGpu;
}

void LightmapperGpu::run()
{
	disableWarning131186();
	initIgnoreTextures();
	calculateLightDirection();

	startDebugTimer();
	gatherLights();
	gatherCollisionTriangles();
	stopDebugTimer("Gathered map data: ");

	initResultData();
	initShaders();

	createSSBOs();
	setDataSSBOs();
	bindSSBOs();

	executeComputeShaders();

	// Copy results and apply them to the lightmap tile data.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultSsbo->getId());
	auto gpuResults = (LightmapResult*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->results.size() * sizeof(LightmapResult), GL_MAP_READ_BIT);

	if (gpuResults)
	{
		std::copy(gpuResults, gpuResults + results.size(), results.begin());
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		startDebugTimer();
		applyLightmapResult();
		stopDebugTimer("Applied GPU data to lightmap: ");
	}

	end();
}

void LightmapperGpu::end()
{
	delete gpuTriangleSsbo;
	delete sampleSsbo;
	delete lightSsbo;
	delete lightGridSsbo;
	delete lightIndicesSsbo;
	delete gpuBvhNodeSsbo;
	delete triangleIndicesSsbo;
	delete gpuCubeTileSsbo;
	delete gpuCubesSsbo;
	delete resultSsbo;

	for (int i = 0; i < shaders.size(); i++)
		delete shaders[i];

	shaders.clear();

	for (auto& textureIt : textureIds)
		glMakeTextureHandleNonResidentARB(textureIt.second);
}

void LightmapperGpu::disableWarning131186()
{
	GLuint disableIds[] = { 131186 };
	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 1, disableIds, GL_FALSE);
}

void LightmapperGpu::gatherLights()
{
	map->rootNode->traverse([&](Node* n) {
		if (n->getComponent<RswLight>()) {
			auto rswLight = n->getComponent<RswLight>();
			auto rswObject = n->getComponent<RswObject>();

			if (!rswLight->enabled)
				return;

			cpuLights.push_back(std::pair<RswObject*, RswLight*>(rswObject, rswLight));
		}
	});
}

int LightmapperGpu::gatherGpuCubes()
{
	gpuCubes.resize(gnd->height * gnd->width);

	int gpuCubeIdx = 0;
	int cubeTileProcessed = 0;

	for (int y = 0; y < gnd->height; y++) {
		for (int x = 0; x < gnd->width; x++) {
			Gnd::Cube* cube = gnd->cubes[x][y];

			GpuCube& gpuCube = gpuCubes[gpuCubeIdx++];
			gpuCube.vertices[0] = glm::vec4(10 * x, -cube->h1, 10 * gnd->height - 10 * y + 10, 0);
			gpuCube.vertices[1] = glm::vec4(10 * x + 10, -cube->h2, 10 * gnd->height - 10 * y + 10, 0);
			gpuCube.vertices[2] = glm::vec4(10 * x, -cube->h3, 10 * gnd->height - 10 * y, 0);
			gpuCube.vertices[3] = glm::vec4(10 * x + 10, -cube->h4, 10 * gnd->height - 10 * y, 0);
			gpuCube.tileUp = cube->tileUp;
			gpuCube.tileFront = cube->tileFront;
			gpuCube.tileSide = cube->tileSide;
		}
	}

	traverseGnd([&](Gnd::Cube* cube, int x, int y, int tileDir) {
		cubeTileProcessed++;
	});
	
	return cubeTileProcessed;
}

void LightmapperGpu::gatherCollisionTriangles()
{
	int height = gnd->height;
	float qualityStep = 1.0f / quality;
	const float sx = 10.0f / (gnd->lightmapWidth - 2);
	const float sy = 10.0f / (gnd->lightmapHeight - 2);
	int sampleCount = (gnd->lightmapWidth - 2) * (gnd->lightmapHeight - 2);

	int cubeTileProcessed = gatherGpuCubes();
	
	gpuCubeTiles.resize(cubeTileProcessed);
	bvh.triangles.resize(2 * cubeTileProcessed);
	samples.resize(cubeTileProcessed * (gnd->lightmapWidth - 2) * (gnd->lightmapHeight - 2));
	
	int cubeIdx = 0;
	int triangleIdx = 0;
	int sampleIdx = 0;
	
	// Gather GND triangles and process cube tiles
	traverseGnd([&](Gnd::Cube* cube, int x, int y, int tileDir) {
		Triangle& triangle0 = bvh.triangles[triangleIdx++];
		Triangle& triangle1 = bvh.triangles[triangleIdx++];
		triangle0.texId = -1;
		triangle1.texId = -1;

		if (tileDir == 0) {
			glm::vec4 v1(10 * x, -cube->h3, 10 * height - 10 * y, 0);
			glm::vec4 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y, 0);
			glm::vec4 v3(10 * x, -cube->h1, 10 * height - 10 * y + 10, 0);
			glm::vec4 v4(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10, 0);

			triangle0.vertices[0] = v3;
			triangle0.vertices[1] = v4;
			triangle0.vertices[2] = v1;
			triangle0.normal = glm::vec4(cube->normalsForCalc[0] * glm::vec3(1, -1, 1), 0);

			triangle1.vertices[0] = v2;
			triangle1.vertices[1] = v1;
			triangle1.vertices[2] = v4;
			triangle1.normal = glm::vec4(cube->normalsForCalc[3] * glm::vec3(1, -1, 1), 0);
		}
		else if (tileDir == 1) {
			auto otherCube = gnd->cubes[x][y + 1];

			glm::vec3 v1(10 * x, -cube->h3, 10 * gnd->height - 10 * y);
			glm::vec3 v2(10 * x + 10, -cube->h4, 10 * gnd->height - 10 * y);
			glm::vec3 v4(10 * x + 10, -otherCube->h2, 10 * gnd->height - 10 * y);
			glm::vec3 v3(10 * x, -otherCube->h1, 10 * gnd->height - 10 * y);

			glm::vec3 normal(0, 0, -1);

			if (cube->h3 < otherCube->h1)
				normal *= -1;

			triangle0.vertices[0] = v1;
			triangle0.vertices[1] = v2;
			triangle0.vertices[2] = v3;
			triangle0.normal = normal;

			triangle1.vertices[0] = v3;
			triangle1.vertices[1] = v2;
			triangle1.vertices[2] = v4;
			triangle1.normal = normal;
		}
		else if (tileDir == 2) {
			auto otherCube = gnd->cubes[x + 1][y];

			glm::vec3 v1(10 * x + 10, -cube->h2, 10 * gnd->height - 10 * y + 10);
			glm::vec3 v2(10 * x + 10, -cube->h4, 10 * gnd->height - 10 * y);
			glm::vec3 v3(10 * x + 10, -otherCube->h1, 10 * gnd->height - 10 * y + 10);
			glm::vec3 v4(10 * x + 10, -otherCube->h3, 10 * gnd->height - 10 * y);

			glm::vec3 normal(-1, 0, 0);

			if (cube->h3 < otherCube->h1)
				normal *= -1;

			triangle0.vertices[0] = v1;
			triangle0.vertices[1] = v3;
			triangle0.vertices[2] = v4;
			triangle0.normal = normal;

			triangle1.vertices[0] = v4;
			triangle1.vertices[1] = v2;
			triangle1.vertices[2] = v1;
			triangle1.normal = normal;
		}

		int currentCubeTile = cubeIdx;
		GpuCubeTile& cubeTile = gpuCubeTiles[cubeIdx++];
		cubeTile.x = x;
		cubeTile.y = y;
		cubeTile.lightIndex = (height - y - 1) * gnd->width + x;

		if (ignoredTextureIdx[gnd->tiles[cube->tileIds[tileDir]]->textureIndex])
			cubeTile.lightIndex = -1;

		if (tileDir == 0) {
			for (int i = 0; i < 4; i++) {
				cubeTile.normals[i] = glm::vec4(cube->normals[i], 0);
				cubeTile.heights[i] = cube->heights[i];
			}
		}
		else if (tileDir == 1) {
			auto otherCube = gnd->cubes[x][y + 1];

			cubeTile.heights[0] = cube->h3;
			cubeTile.heights[1] = cube->h4;
			cubeTile.heights[2] = otherCube->h1;
			cubeTile.heights[3] = otherCube->h2;
		}
		else if (tileDir == 2) {
			auto otherCube = gnd->cubes[x + 1][y];

			cubeTile.heights[0] = cube->h4;
			cubeTile.heights[1] = cube->h2;
			cubeTile.heights[2] = otherCube->h3;
			cubeTile.heights[3] = otherCube->h1;
		}

		// Create samples
		for (int i = 0; i < sampleCount; i++) {
			LightmapSample& sample = samples[sampleIdx++];
			sample.cubeTileIndex = currentCubeTile;
			sample.direction = tileDir;
		}
	});
	
	gatherModelTriangles();
	gatherGpuTriangles();
	bvh.buildTree();
	gatherGpuNodes();
}

void LightmapperGpu::gatherModelTriangles()
{
	std::vector<Bvh> bvhs;
	size_t totalTriangles = 0;
	
	map->rootNode->traverse([&](Node* n) {
		if (RswModel* m = n->getComponent<RswModel>())
			if (m->shadowStrength > 0) {
				auto collider = n->getComponent<RswModelCollider>();
				auto rsmRenderer = n->getComponent<RsmRenderer>();
	
				if (!rsmRenderer || !collider)
					return;
	
				collider->calculateWorldFaces(false);
	
				auto& triangles = collider->bvh.triangles;
	
				for (int i = 0; i < triangles.size(); i++) {
					triangles[i].shadowStrength = m->shadowStrength;
					triangles[i].texId = rsmRenderer->textures[triangles[i].texId]->getId();
				}
	
				totalTriangles += triangles.size();
				bvhs.push_back(collider->bvh);
			}
		});
	
	bvh.triangles.reserve(bvh.triangles.size() + totalTriangles);
	
	for (auto& bvh2 : bvhs)
		bvh.triangles.insert(bvh.triangles.end(), bvh2.triangles.begin(), bvh2.triangles.end());
}

void LightmapperGpu::gatherGpuTriangles()
{
	gpuTriangles.resize(bvh.triangles.size());
	
	for (const auto& triangle : bvh.triangles)
	{
		if (triangle.texId < 0)
			continue;
	
		auto [it, inserted] = textureIds.try_emplace(triangle.texId, 0);
	
		if (inserted) {
			GLuint64 handle = glGetTextureHandleARB(triangle.texId);
			it->second = handle;
			glMakeTextureHandleResidentARB(handle);
		}
	}
	
	for (int i = 0; i < bvh.triangles.size(); i++)
	{
		Triangle& triangle = bvh.triangles[i];
		GpuTriangle& gpu = gpuTriangles[i];
	
		for (int i = 0; i < 3; i++) {
			gpu.vertices[i] = glm::vec4(triangle.vertices[i], 1.0f);
			gpu.uvs[i] = glm::vec4(triangle.uvs[i], 0.0f, 0.0f);
		}
	
		gpu.normal = glm::vec4(triangle.normal, 0.0f);
		gpu.shadowStrength = triangle.shadowStrength;
		gpu.texId = triangle.texId;
	
		if (triangle.texId >= 0)
			gpu.textureHandle = textureIds[triangle.texId];
	}
}

void LightmapperGpu::gatherGpuNodes()
{
	gpuBvhNodes.resize(bvh.nodes.size());
	
	for (int i = 0; i < bvh.nodes.size(); i++)
	{
		BvhNode& node = bvh.nodes[i];
		GpuBvhNode& gpu = gpuBvhNodes[i];
	
		gpu.aabbMin = glm::vec4(node.aabb.bounds[0], 0.0f);
		gpu.aabbMax = glm::vec4(node.aabb.bounds[1], 0.0f);
		gpu.center = (gpu.aabbMax - gpu.aabbMin) * 0.5f;
	
		gpu.left = node.left;
		gpu.right = node.right;
		gpu.firstTriangle = node.firstTriangle;
		gpu.triangleCount = node.triangleCount;
	}
}

void LightmapperGpu::gatherLightData(int falloffStyle)
{
	std::vector<std::vector<uint32_t>> lightGrid;
	lightGrid.resize(gnd->width * gnd->height);

	gpuLights.clear();
	gpuLights.resize(cpuLights.size());

	int usableLights = 0;

	for (uint32_t lightIndex = 0; lightIndex < cpuLights.size(); lightIndex++) {
		const auto& pair = cpuLights[lightIndex];
		auto rswLight = pair.second;

		bool isSun = falloffStyle == -1 && pair.second->lightType == RswLight::Type::Sun;
		bool isValidFalloff = rswLight->lightType != RswLight::Type::Sun && falloffStyle == (int)rswLight->falloffStyle;

		if (!isSun && !isValidFalloff)
			continue;

		auto rswObject = pair.first;
		float range = rswLight->realRange();
		auto p = roPosToWorldPos(rswObject->position);

		int minX = (int)floor((p.x - range) / 10.0f);
		int maxX = (int)floor((p.x + range) / 10.0f);

		int minY = (int)floor((p.z - range) / 10.0f - 1);
		int maxY = (int)floor((p.z + range) / 10.0f - 1);

		minX = std::max(0, minX);
		minY = std::max(0, minY);
		maxX = std::min(gnd->width - 1, maxX);
		maxY = std::min(gnd->height - 1, maxY);

		for (int y = minY; y <= maxY; y++) {
			for (int x = minX; x <= maxX; x++) {
				lightGrid[y * gnd->width + x].push_back(usableLights);
			}
		}

		auto shaderLight = &gpuLights[usableLights++];
		shaderLight->position = glm::vec4(p, 0.0);
		shaderLight->color = glm::vec4(rswLight->color, 0.0f);
		shaderLight->type = (int)rswLight->lightType;
		shaderLight->falloffStyle = (int)rswLight->falloffStyle;
		shaderLight->range = rswLight->range;
		shaderLight->intensity = rswLight->intensity;
		shaderLight->cutoff = rswLight->cutOff;
		shaderLight->diffuseLighting = rswLight->diffuseLighting;
		shaderLight->affectShadowMap = rswLight->affectShadowMap;
		shaderLight->direction = glm::vec4(rswLight->direction, 0);
		shaderLight->width = rswLight->spotlightWidth;
		shaderLight->affectLightmap = rswLight->affectLightmap;
		shaderLight->shadowTerrain = rswLight->shadowTerrain;
		shaderLight->noTerrainShading = rswLight->noTerrainShading;
		shaderLight->sunMatchRswDirection = rswLight->sunMatchRswDirection;
		shaderLight->realRange = range;

		shaderLight->falloffCount = glm::min(10, (int)rswLight->falloff.size());
		for (int i = 0; i < rswLight->falloff.size() && i < 10; i++) {
			shaderLight->falloff[i] = glm::vec4(rswLight->falloff[i].x, rswLight->falloff[i].y, 0, 0);
		}

		if (rswLight->falloff.size() > 10) {
			std::cout << "Lightmapper: light has too many falloffs! The GPU only supports up to 10." << std::endl;
		}
	}

	gpuLights.resize(usableLights);

	// Flatten lightGrid to lightIndices
	lightGridCells.clear();
	lightIndices.clear();
	lightGridCells.resize(lightGrid.size());

	int total = 0;

	for (int i = 0; i < lightGridCells.size(); i++) {
		auto& l = lightGrid[i];

		lightGridCells[i].first = total;
		lightGridCells[i].count = (uint32_t)l.size();

		total += lightGridCells[i].count;

		for (int j = 0; j < l.size(); j++) {
			lightIndices.push_back(l[j]);
		}
	}
}

void LightmapperGpu::calculateLightDirection()
{
	glm::mat4 rot = glm::mat4(1.0f);
	rot = glm::rotate(rot, glm::radians(-(float)rsw->light.latitude), glm::vec3(1, 0, 0));
	rot = glm::rotate(rot, glm::radians((float)rsw->light.longitude), glm::vec3(0, 1, 0));
	lightDirection = glm::vec3(0.0f, 1.0f, 0.0f) * glm::mat3(rot);
}

void LightmapperGpu::initResultData()
{
	results.clear();
	results.resize(samples.size());

	float intensity = (settings->additiveShadow ? rsw->light.lightmapAmbient : 1) * quality * quality;

	for (auto& result : results)
		result.color.w = intensity;
}

void LightmapperGpu::initShaders()
{
	for (auto& shader : shaders)
		delete shader;
	
	shaders.clear();
	shaders.push_back(new LightmapperShader("#version 430\n#define SUN\n"));

	// The GLSL compiler will remove lightDirection if SUN isn't defined (due to optimizations).
	// This would normally create console warnings for the other shaders below, so instead it's added manually.
	shaders[0]->bindUniform(LightmapperShader::Uniforms::lightDirection, "lightDirection");

	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 0\n"));
	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 1\n"));
	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 2\n"));
	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 3\n"));
	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 4\n"));
	shaders.push_back(new LightmapperShader("#version 430\n#define FALLOFF_STYLE 5\n"));
}

void LightmapperGpu::initIgnoreTextures()
{
	ignoredTextureIdx.clear();
	ignoredTextureIdx.resize(gnd->textures.size());

	if (settings->ignoreBacksideTexture) {
		for (int i = 0; i < gnd->textures.size(); i++)
			if (_stricmp(gnd->textures[i]->file.c_str(), "backside.bmp") == 0)
				ignoredTextureIdx[i] = true;
	}
}

void LightmapperGpu::createSSBOs()
{
	if (gpuTriangleSsbo != nullptr)
		return;

	gpuTriangleSsbo = new gl::SSBO<GpuTriangle>();
	sampleSsbo = new gl::SSBO<LightmapSample>();
	lightSsbo = new gl::SSBO<LightmapLight>();
	lightGridSsbo = new gl::SSBO<LightGridCell>();
	lightIndicesSsbo = new gl::SSBO<uint32_t>();
	gpuBvhNodeSsbo = new gl::SSBO<GpuBvhNode>();
	triangleIndicesSsbo = new gl::SSBO<uint32_t>();
	gpuCubeTileSsbo = new gl::SSBO<GpuCubeTile>();
	gpuCubesSsbo = new gl::SSBO<GpuCube>();
	resultSsbo = new gl::SSBO<LightmapResult>();
}

void LightmapperGpu::setDataSSBOs()
{
	gpuTriangleSsbo->setData(gpuTriangles, GL_STATIC_DRAW);
	sampleSsbo->setData(samples, GL_STATIC_DRAW);
	gpuBvhNodeSsbo->setData(gpuBvhNodes, GL_STATIC_DRAW);
	// Light data is set per shader
	triangleIndicesSsbo->setData(bvh.triangleIndices, GL_STATIC_DRAW);
	gpuCubeTileSsbo->setData(gpuCubeTiles, GL_STATIC_DRAW);
	gpuCubesSsbo->setData(gpuCubes, GL_STATIC_DRAW);
	resultSsbo->setData(results, GL_DYNAMIC_COPY);
}

void LightmapperGpu::bindSSBOs()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuTriangleSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sampleSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightGridSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, lightIndicesSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, gpuBvhNodeSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, triangleIndicesSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, gpuCubeTileSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, gpuCubesSsbo->getId());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, resultSsbo->getId());
}

void LightmapperGpu::executeComputeShaders()
{
	GLuint* timeQuery = new GLuint[shaders.size()];
	glGenQueries((GLsizei)shaders.size(), timeQuery);

	GLuint sampleCount = (GLuint)samples.size();
	GLuint groups = (sampleCount + 255) / 256;

	for (int i = 0; i < shaders.size(); i++) {
		gatherLightData(i - 1);

		if (gpuLights.empty())
			continue;

		auto shader = shaders[i];
		shader->use();
		
		if (i == 0)
			shader->setUniform(LightmapperShader::Uniforms::lightDirection, lightDirection);

		shader->setUniform(LightmapperShader::Uniforms::additiveShadow, settings->additiveShadow);
		shader->setUniform(LightmapperShader::Uniforms::shadows, settings->shadows);
		shader->setUniform(LightmapperShader::Uniforms::quality, quality);
		shader->setUniform(LightmapperShader::Uniforms::lightmapWidth, gnd->lightmapWidth);
		shader->setUniform(LightmapperShader::Uniforms::lightmapHeight, gnd->lightmapHeight);
		shader->setUniform(LightmapperShader::Uniforms::gndHeight, gnd->height);

		lightSsbo->setData(gpuLights, GL_STATIC_DRAW);
		lightGridSsbo->setData(lightGridCells, GL_STATIC_DRAW);
		lightIndicesSsbo->setData(lightIndices, GL_STATIC_DRAW);

		glBeginQuery(GL_TIME_ELAPSED, timeQuery[i]);
		glDispatchCompute(groups, 1, 1);
		glEndQuery(GL_TIME_ELAPSED);

		GLuint64 timeElapsedNoNs = 0;
		glGetQueryObjectui64v(timeQuery[i], GL_QUERY_RESULT, &timeElapsedNoNs);

		double milliSeconds = timeElapsedNoNs / 1000000.0;
		std::cout << "Lightmapper: Compute Shader (" << i << "): " << milliSeconds << "ms\n";

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	}

	delete[] timeQuery;
}

void LightmapperGpu::applyLightmapResult()
{
	unsigned int idx = 0;
	float count = (float)(quality * quality);
	int stride = gnd->lightmapWidth * gnd->lightmapHeight;

	traverseGnd([&](Gnd::Cube* cube, int x, int y, int tileDir) {
		Gnd::Tile* tile = gnd->tiles[cube->tileIds[tileDir]];
		Gnd::Lightmap* lightmap = gnd->lightmaps[tile->lightmapIndex];
		auto& data = lightmap->data;

		for (int xx = 1; xx < gnd->lightmapWidth - 1; xx++) {
			for (int yy = 1; yy < gnd->lightmapHeight - 1; yy++) {
				auto& result = results[idx++];
				auto color = result.color / count;

				data[xx + gnd->lightmapWidth * yy] = (unsigned char)(color.a * 255.0f);
				data[stride + 3 * (xx + gnd->lightmapWidth * yy) + 0] = glm::min(255, (int)(color.r * 255));
				data[stride + 3 * (xx + gnd->lightmapWidth * yy) + 1] = glm::min(255, (int)(color.g * 255));
				data[stride + 3 * (xx + gnd->lightmapWidth * yy) + 2] = glm::min(255, (int)(color.b * 255));
			}
		}
	});
}

void LightmapperGpu::traverseGnd(std::function<void(Gnd::Cube*, int, int, int)> visitGnd)
{
	for (int y = settings->rangeY[0]; y < settings->rangeY[1]; y++) {
		for (int x = settings->rangeX[0]; x < settings->rangeX[1]; x++) {
			if (settings->heightSelectionOnly && std::find(map->tileSelection.begin(), map->tileSelection.end(), glm::ivec2(x, y)) == map->tileSelection.end())
				continue;

			Gnd::Cube* cube = gnd->cubes[x][y];

			if (cube->tileUp != -1)
				visitGnd(cube, x, y, 0);
			if (cube->tileFront != -1 && y < gnd->height - 1)
				visitGnd(cube, x, y, 1);
			if (cube->tileSide != -1 && x < gnd->width - 1)
				visitGnd(cube, x, y, 2);
		}
	}
}

glm::vec3 LightmapperGpu::roPosToWorldPos(glm::vec3& p)
{
	return glm::vec3(gnd->width * 5.0f + p.x, -p.y, gnd->height * 5.0f + 10.0f - p.z);
}