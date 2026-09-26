#include "SkyMapRenderer.h"
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/NodeRenderer.h>
#include <glad/gl.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/Vertex.h>

#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

SkyMapRenderer::SkyMapRenderer()
{
	renderContext = SkyMapRenderContext::getInstance();
}

SkyMapRenderer::~SkyMapRenderer()
{
	if (cloudAtlas)
		util::ResourceManager<gl::Texture>::unload(cloudAtlas);
	if (starAtlas)
		util::ResourceManager<gl::Texture>::unload(starAtlas);
	if (fogAtlas)
		util::ResourceManager<gl::Texture>::unload(fogAtlas);

	for (auto& cloudInstance : cloudInstances) {
		if (cloudInstance.renderInfo != nullptr) {
			if (cloudInstance.renderInfo->vbo)
				delete cloudInstance.renderInfo->vbo;
			if (cloudInstance.renderInfo->ebo)
				delete cloudInstance.renderInfo->ebo;
			if (cloudInstance.renderInfo->vao)
				delete cloudInstance.renderInfo->vao;

			delete cloudInstance.renderInfo;
		}
	}

	if (ubo)
		delete ubo;
}

gl::Texture* SkyMapRenderer::createTextureAtlas(std::initializer_list<std::string> textures)
{
	gl::Texture* tex = new gl::Texture(256, 256 * (int)textures.size());
	int index = 0;
	
	for (const auto& texture_i : textures) {
		std::string texture = "data\\texture\\effect\\" + texture_i;

		std::istream* is = util::FileIO::open(texture);
		if (!is)
		{
			std::cerr << "Texture: Could not open " << texture << std::endl;
			continue;
		}
		is->seekg(0, std::ios_base::end);
		std::size_t len = is->tellg();
		if (len <= 0 || len > 100 * 1024 * 1024)
		{
			std::cerr << "Texture: Error opening texture " << texture << ", file is either empty or too large" << std::endl;
			delete is;
			continue;
		}
	
		char* buffer = new char[len];
		is->seekg(0, std::ios_base::beg);
		is->read(buffer, len);
		delete is;
	
		int width, height, comp;
		unsigned char* data = stbi_load_from_memory((stbi_uc*)buffer, (int)len, &width, &height, &comp, 4);
		if (!data)
		{
			std::cerr << "Texture: " << texture << " could not load; error: " << stbi_failure_reason() << std::endl;
			continue;
		}
	
		if (width != 256 || height != 256)
		{
			std::cerr << "Texture: " << texture << " has invalid dimensions. Expected 256x256, found " << width << ", " << height << std::endl;
			continue;
		}

		util::imageDitherAndPinkRemove(texture_i, data, width, height);
	
		tex->setSubImage((char*)data, 0, 256 * index, 256, 256);
		index++;
	}

	return tex;
}

void SkyMapRenderer::reload()
{
	for (auto& cloudInstance : cloudInstances) {
		if (cloudInstance.renderInfo != nullptr) {
			if (cloudInstance.renderInfo->vbo)
				delete cloudInstance.renderInfo->vbo;
			if (cloudInstance.renderInfo->ebo)
				delete cloudInstance.renderInfo->ebo;
			if (cloudInstance.renderInfo->vao)
				delete cloudInstance.renderInfo->vao;

			delete cloudInstance.renderInfo;
		}
	}

	cloudInstances.clear();

	for (auto& cloudEffect : lubSkyMap->clouds) {
		struct CloudInstance cloudInstance;
		cloudInstance.textureAtlas = cloudAtlas;
		cloudInstance.source = cloudEffect;

		cloudInstances.push_back(cloudInstance);
	}

	if (lubSkyMap->Star_Effect) {
		struct CloudInstance cloudInstance;
		cloudInstance.textureAtlas = starAtlas;
		cloudInstance.source = lubSkyMap->getStarEffectTemplate();

		cloudInstances.push_back(cloudInstance);
	}

	for (auto id : lubSkyMap->oldClouds) {
		struct CloudInstance cloudInstance;
		cloudInstance.textureAtlas = cloudAtlas;
		cloudInstance.source = lubSkyMap->getTemplate(id);

		if (cloudInstance.source != nullptr) {
			if (cloudInstance.source->subClouds.size() > 0) {
				for (auto& cloudEffect : cloudInstance.source->subClouds) {
					struct CloudInstance subCloudInstance;
					subCloudInstance.textureAtlas = cloudAtlas;
					subCloudInstance.source = cloudEffect;

					cloudInstances.push_back(subCloudInstance);
				}
			
				continue;
			}
			else if (cloudInstance.source->useStarTextures) {
				cloudInstance.textureAtlas = starAtlas;
			}
			else if (cloudInstance.source->useFogTextures) {
				cloudInstance.textureAtlas = fogAtlas;
			}
		}

		cloudInstances.push_back(cloudInstance);
	}
}

void checkGLErrors() {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL Error encountered: " << err << std::endl;
		// Optionally map the error code to a string for readability
	}
}

void SkyMapRenderer::render(NodeRenderContext& context)
{
	if (!this->lubSkyMap)
		this->lubSkyMap = node->getComponent<LubSkyMap>();
	if (!this->rsw)
		this->rsw = node->getComponent<Rsw>();
	if (!this->gnd)
		this->gnd = node->getComponent<Gnd>();

	if (!lubSkyMap || !rsw || !gnd || !lubSkyMap->isEnabled)
		return;

	// Ensure atlas are created
	if (!atlasLoaded) {
		this->cloudAtlas = createTextureAtlas({ "cloud1.tga", "cloud2.tga", "cloud3.tga", "cloud4.tga" });
		this->starAtlas = createTextureAtlas({ "star01.bmp", "star02.bmp", "star03.bmp", "star04.bmp", "star05.bmp", "star06.bmp" });
		this->fogAtlas = createTextureAtlas({ "fog1.tga", "fog2.tga", "fog3.tga" });
		atlasLoaded = true;
	}

	if (dirty) {
		reload();
		dirty = false;
	}

	if (!ubo)
		ubo = new gl::UBO<ParticleParams>();

	auto shader = dynamic_cast<SkyMapRenderContext*>(renderContext)->shader;

	time = context.time;
	shader->setUniform(SkyMapShader::Uniforms::uTime, time / 1.6f);

	for (auto& cloudInstance : cloudInstances) {
		auto lubSkyMap = cloudInstance.source;

		if (lubSkyMap == nullptr || !cloudInstance.textureAtlas)
			continue;

		// TODO: Move this during the loading phase, or better, keep the texture count instead
		if (cloudInstance.textureAtlas == cloudAtlas)
			cloudInstance.params.uvSplit = 4;
		else if (cloudInstance.textureAtlas == starAtlas)
			cloudInstance.params.uvSplit = 6;
		else if (cloudInstance.textureAtlas == fogAtlas)
			cloudInstance.params.uvSplit = 3;

		cloudInstance.params.Size = lubSkyMap->Size;
		cloudInstance.params.Size_Extra = lubSkyMap->Size_Extra;
		cloudInstance.params.Expand_Rate = lubSkyMap->Expand_Rate;
		cloudInstance.params.Alpha_Inc_Time = lubSkyMap->Alpha_Inc_Time;
		cloudInstance.params.Alpha_Inc_Time_Extra = lubSkyMap->Alpha_Inc_Time_Extra;
		cloudInstance.params.Alpha_Inc_Speed = lubSkyMap->Alpha_Inc_Speed;
		cloudInstance.params.Alpha_Dec_Time = lubSkyMap->Alpha_Dec_Time;
		cloudInstance.params.Alpha_Dec_Time_Extra = lubSkyMap->Alpha_Dec_Time_Extra;
		cloudInstance.params.Alpha_Dec_Speed = lubSkyMap->Alpha_Dec_Speed;
		cloudInstance.params.Height = lubSkyMap->Height;
		cloudInstance.params.Height_Extra = lubSkyMap->Height_Extra;
		cloudInstance.params.uvCycleSpeed = lubSkyMap->uvCycleSpeed;
		cloudInstance.params.dirMode = lubSkyMap->dirMode;
		cloudInstance.params.forcedDir = lubSkyMap->forcedDir;
		cloudInstance.params.scaleX = lubSkyMap->scaleX;
		cloudInstance.params.scaleY = lubSkyMap->scaleY;

		int num = lubSkyMap->Num;

		if (num < 0) {
			num = (int)(lubSkyMap->NumPerSquared * gnd->width * gnd->height * 100.0f);
		}

		num = glm::min(num, 10000);

		auto ri = cloudInstance.renderInfo;

		// Load render data
		if (ri == nullptr) {
			cloudInstance.renderInfo = new RenderInfo();
			ri = cloudInstance.renderInfo;

			std::vector<VertexP3T2> verts;
			verts.push_back(VertexP3T2(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0, 0)));
			verts.push_back(VertexP3T2(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec2(1, 0)));
			verts.push_back(VertexP3T2(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec2(0, 1)));
			verts.push_back(VertexP3T2(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1, 1)));
			
			ri->vao = new gl::VAO();
			ri->vao->bind();

			ri->vbo = new gl::VBO<VertexP3T2>();
			ri->vbo->setData(verts, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));

			unsigned int quadIndices[] = {0, 1, 2, 2, 3, 1};
			ri->ebo = new gl::EBO();
			ri->ebo->setData(std::size(quadIndices), quadIndices, GL_STATIC_DRAW);

			ri->instance_vbo = new gl::VBO<Particle>();
			ri->instance_vbo->setData(cloudInstance.particles, GL_STREAM_DRAW);
			
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);
			glEnableVertexAttribArray(4);
			glEnableVertexAttribArray(5);
			glEnableVertexAttribArray(6);
			glEnableVertexAttribArray(7);
			glEnableVertexAttribArray(8);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(Particle), (void*)(0 * sizeof(float)));
			glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(Particle), (void*)(3 * sizeof(float)));
			glVertexAttribPointer(4, 1, GL_FLOAT, false, sizeof(Particle), (void*)(4 * sizeof(float)));
			glVertexAttribPointer(5, 1, GL_FLOAT, false, sizeof(Particle), (void*)(5 * sizeof(float)));
			glVertexAttribPointer(6, 1, GL_FLOAT, false, sizeof(Particle), (void*)(6 * sizeof(float)));
			glVertexAttribPointer(7, 1, GL_FLOAT, false, sizeof(Particle), (void*)(7 * sizeof(float)));
			glVertexAttribPointer(8, 1, GL_FLOAT, false, sizeof(Particle), (void*)(8 * sizeof(float)));
			glVertexAttribDivisor(2, 1);
			glVertexAttribDivisor(3, 1);
			glVertexAttribDivisor(4, 1);
			glVertexAttribDivisor(5, 1);
			glVertexAttribDivisor(6, 1);
			glVertexAttribDivisor(7, 1);
			glVertexAttribDivisor(8, 1);
			
			for (int i = 0; i < cloudInstance.particles.size(); i++) {
				auto& particle = cloudInstance.particles[i];
				particle.expandDelay = 10.0f * (rand() / (float)RAND_MAX);
				updateParticle(cloudInstance, particle, i);

				// Adjust start/end on creation for a smoother start
				float dur = cloudInstance.particles[i].lifeEnd - cloudInstance.particles[i].lifeStart;
				float adjust = dur * (rand() / (float)RAND_MAX);
				cloudInstance.particles[i].lifeStart -= adjust;
				cloudInstance.particles[i].lifeEnd -= adjust;
			}
			
			// Why is it being setup twice...? I mean, it makes sense since the values were updated, but why do it in the first time?
			ri->instance_vbo->setData(cloudInstance.particles, GL_STREAM_DRAW);
		}

		glBlendFuncSeparate(lubSkyMap->blendSrc, lubSkyMap->blendDst, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		shader->setUniform(SkyMapShader::Uniforms::color, glm::vec4(lubSkyMap->Color, 1.0f));
		
		ubo->setData(&cloudInstance.params);
		
		cloudInstance.textureAtlas->bind();
		
		ri->vao->bind();
		ri->instance_vbo->bind();

		if (cloudInstance.particles.size() != num) {
			cloudInstance.particles.resize(num);
			ri->instance_vbo->setData(cloudInstance.particles, GL_STREAM_DRAW);
		}
		
		std::vector<int> updatedIndices;
		
		for (int i = 0; i < cloudInstance.particles.size(); i++) {
			if (time > cloudInstance.particles[i].lifeEnd) {
				updateParticle(cloudInstance, cloudInstance.particles[i], i);
				updatedIndices.push_back(i);
			}
		}
		
		if (updatedIndices.size() > 0) {
			int particleSize = sizeof(SkyMapRenderer::Particle);
			std::vector<Particle> tempBuffer(updatedIndices.size());
			for (int i = 0; i < updatedIndices.size(); i++)
				tempBuffer[i] = cloudInstance.particles[updatedIndices[i]];
			
			glBindBuffer(GL_COPY_WRITE_BUFFER, ri->instance_vbo->getBufferId());
			void* basePtr = glMapBufferRange(
				GL_COPY_WRITE_BUFFER,
				0,
				cloudInstance.particles.size() * particleSize, 
				GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT
			);
			if (basePtr != nullptr) {
				char* bytePtr = static_cast<char*>(basePtr);
			
				for (int i = 0; i < updatedIndices.size(); i++) {
					int idx = updatedIndices[i];
			
					memcpy(bytePtr + (idx * particleSize), &tempBuffer[i], particleSize);
				}
				glUnmapBuffer(GL_COPY_WRITE_BUFFER);
			}
		}
		
		ri->ebo->bind();
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int)cloudInstance.particles.size());
	}
}

void SkyMapRenderer::updateParticle(SkyMapRenderer::CloudInstance& cloudInstance, SkyMapRenderer::Particle& particle, int i)
{
	particle.position.x = (rand() / (float)RAND_MAX) * gnd->width * 10;
	particle.position.y = -cloudInstance.params.Height;
	particle.position.z = (rand() / (float)RAND_MAX) * gnd->height * 10;

	if (cloudInstance.source->snapToGround) {
		int x = glm::min((int)(particle.position.x / 10.0f), gnd->width - 1);
		int y = glm::min((int)(particle.position.z / 10.0f), gnd->height - 1);

		particle.position.y = -gnd->cubes[x][gnd->height - y - 1]->heights[0] + cloudInstance.params.Size;
	}

	particle.seed = (rand() / (float)RAND_MAX);

	float alphaIncTime = cloudInstance.params.Alpha_Inc_Time + particle.seed * cloudInstance.params.Alpha_Inc_Time_Extra;
	float duration = alphaIncTime / 100.0f;
	float alpha = glm::min(2.55f, duration * cloudInstance.params.Alpha_Inc_Speed);
	float startDecreaseTime = cloudInstance.params.Alpha_Dec_Time / 100.0f + (rand() / (float)RAND_MAX) * cloudInstance.params.Alpha_Dec_Time_Extra / 100.0f;

	if (startDecreaseTime > duration)
		duration = startDecreaseTime;

	if (cloudInstance.params.Alpha_Dec_Speed <= 0) {
		duration = std::numeric_limits<float>::max();
	}
	else {
		duration += alpha / cloudInstance.params.Alpha_Dec_Speed;
	}

	particle.lifeEnd = time + duration * 1.6f;
	particle.alphaDecTime = startDecreaseTime;
	particle.lifeStart = time / 1.6f;

	if (cloudInstance.textureAtlas == starAtlas) {
		if (cloudInstance.source->oldCloudEffect == 15) {
			particle.uvStart = 0;
		}
		else {
			particle.uvStart = (float)(int)(cloudInstance.params.uvSplit * (rand() / (float)RAND_MAX));
		}
	}
	else {
		particle.uvStart = (i % (int)cloudInstance.params.uvSplit) / cloudInstance.params.uvSplit;
	}
}

SkyMapRenderer::SkyMapRenderContext::SkyMapRenderContext() : shader(util::ResourceManager<gl::Shader>::load<SkyMapShader>())
{
	shader->use();
	shader->setUniform(SkyMapShader::Uniforms::s_texture, 0);
	order = RendererDrawPriority::SkyMap;
}

void SkyMapRenderer::SkyMapRenderContext::preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthMask(false);
	shader->use();
	shader->setUniform(SkyMapShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(SkyMapShader::Uniforms::cameraMatrix, context.viewMatrix);
}

void SkyMapRenderer::SkyMapRenderContext::postFrame(NodeRenderContext& context)
{
	glDepthMask(true);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}