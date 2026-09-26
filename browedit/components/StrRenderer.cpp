#include "StrRenderer.h"
#include "Rsm.h"
#include "Rsw.h"
#include "Gnd.h"
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/NodeRenderer.h>
#include <browedit/MapView.h>
#include <browedit/gl/FBO.h>
#include <glm/glm.hpp>
#include <filesystem>

StrRenderer::StrRenderer()
{
	renderContext = StrRenderContext::getInstance();
	begin();
}

StrRenderer::~StrRenderer()
{
	begin();
}

void StrRenderer::begin()
{
	selected = false;
	if (str != nullptr)
		util::ResourceManager<Str>::unload(str);
	str = nullptr;
	rswObject = nullptr;
	gnd = nullptr;
	for (auto layerTextures : textures)
		for (auto t : layerTextures)
			util::ResourceManager<gl::Texture>::unload(t);
	textures.clear();

	// Setup dummy data
	if (verts.size() == 0) {
		verts.push_back(VertexP2T2(glm::vec2(0.0f), glm::vec2(0.0f)));
		verts.push_back(VertexP2T2(glm::vec2(0.0f), glm::vec2(0.0f)));
		verts.push_back(VertexP2T2(glm::vec2(0.0f), glm::vec2(0.0f)));
		verts.push_back(VertexP2T2(glm::vec2(0.0f), glm::vec2(0.0f)));
	}
}

/// <summary>
/// Renders the specified context.
/// </summary>
/// <param name="context">The context.</param>
void StrRenderer::render(NodeRenderContext& context)
{
	if (!this->rswObject)
		this->rswObject = node->getComponent<RswObject>();
	if (!this->gnd)
		this->gnd = node->root->getComponent<Gnd>();
	if (!this->strEffect)
		this->strEffect = node->getComponent<StrEffect>();
	
	if (!strEffect)
		return;

	if (dirty || strEffect->dirty)
	{
		dirty = false;

		if (!this->str || strEffect->dirty) {
			strEffect->dirty = false;

			if (str != nullptr)
				util::ResourceManager<Str>::unload(str);

			str = util::ResourceManager<Str>::load("data\\texture\\effect\\" + util::utf8_to_iso_8859_1(strEffect->str));
		}

		if (!this->str)
			return;

		// Reload all textures
		for (auto layerTextures : textures)
			for (auto t : layerTextures)
				util::ResourceManager<gl::Texture>::unload(t);
		textures.clear();

		std::filesystem::path strFolder = std::filesystem::path(str->fileName).parent_path();

		for (auto& layer : str->layers) {
			std::vector<gl::Texture*> layerTextures;

			for (auto& textureFilename : layer->textures) {
				std::filesystem::path finalPath = strFolder / textureFilename;
				layerTextures.push_back(util::ResourceManager<gl::Texture>::load(finalPath.string()));
			}

			textures.push_back(layerTextures);
		}
	}

	if (!rswObject || !strEffect || !gnd || !str)
		return;

	// STR files always use 60 fps, regardless of what the str file itself says.
	int time = (int)(context.time * 60) % (str->maxKeyFrame + 1);
	auto shader = dynamic_cast<StrRenderContext*>(renderContext)->shader;

	glm::mat4 instanceMatrix(1.0f);
	instanceMatrix = glm::scale(instanceMatrix, glm::vec3(1, 1, -1));
	instanceMatrix = glm::translate(instanceMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	
	shader->setUniform(StrShader::Uniforms::alpha, strEffect->alpharatio);

	int layerIdx = -1;

	for (auto& layer : str->layers) {
		layerIdx++;

		if (layer->frames.size() == 0 || layer->textures.size() == 0)
			continue;

		Str::Layer::Frame* frame0 = nullptr;
		Str::Layer::Frame* frame1 = nullptr;

		// Find the base frame index for the current time
		for (int i = 0; i < layer->frames.size(); i++) {
			if (time < layer->frames[i].time)
				break;
			if (layer->frames[i].time == time ||
				(i + 1 < layer->frames.size() && time < layer->frames[i + 1].time)) {
				frame0 = &layer->frames[i];
				frame1 = i + 1 < layer->frames.size() ? &layer->frames[i + 1] : frame0;
				break;
			}
		}

		// No frame for the current time
		if (frame0 == nullptr)
			continue;

#define EASE(v0, v1, t) ((v1 - v0) * t + v0)

		// Calculate interpolation for the current frame
		float stime = 0.0f;
		
		if (frame1->time > frame0->time && frame0->isInterpolated)
			stime = 1.0f / (frame1->time - frame0->time) * (time - frame0->time);

		float angle = EASE(frame0->angle, frame1->angle, stime);
		glm::vec4 color = glm::vec4(
			EASE(frame0->color.x, frame1->color.x, stime),
			EASE(frame0->color.y, frame1->color.y, stime),
			EASE(frame0->color.z, frame1->color.z, stime),
			EASE(frame0->color.w, frame1->color.w, stime)
		);

		float positions[8];
		float uvs[8];

		for (int i = 0; i < 8; i++) {
			positions[i] = EASE(frame0->positions[i], frame1->positions[i], stime);
			uvs[i] = EASE(frame0->uvs[i], frame1->uvs[i], stime);
		}

		glm::vec2 offset = EASE(frame0->offset, frame1->offset, stime);
		int textureCount = (int)layer->textures.size();
		int textureIndex = 0;

		switch (frame0->animationType) {
			default:
			case Str::Layer::Frame::AnimationType::ANIM_STOP:
				textureIndex = (int)frame0->textureIndex;
				break;
			case Str::Layer::Frame::AnimationType::ANIM_INTERPOLATION:
				textureIndex = (int)EASE(frame0->textureIndex, frame1->textureIndex, stime);
				break;
			case Str::Layer::Frame::AnimationType::ANIM_ONCE:
				textureIndex = (int)(frame0->delay * (time - frame0->time) + frame0->textureIndex);

				if (textureIndex >= textureCount)
					textureIndex = textureCount - 1;
				break;
			case Str::Layer::Frame::AnimationType::ANIM_LOOP:
				textureIndex = (int)(frame0->delay * (time - frame0->time) + frame0->textureIndex);
				textureIndex = textureIndex % textureCount;
				break;
			case Str::Layer::Frame::AnimationType::ANIM_REVERSE_LOOP:
				textureIndex = (int)(frame0->textureIndex - frame0->delay * (time - frame0->time));
				textureIndex = ((textureIndex % textureCount) + textureCount) % textureCount;
				break;
			case Str::Layer::Frame::AnimationType::ANIM_BI_LOOP:
				if (textureCount <= 1) {
					textureIndex = 0;
					break;
				}

				int rawIndex = (int)(frame0->delay * (time - frame0->time) + frame0->textureIndex);

				int cycleLength = (textureCount - 1) * 2;

				int pingPong = rawIndex % cycleLength;

				if (pingPong >= textureCount)
					pingPong = cycleLength - pingPong;

				textureIndex = pingPong;
				break;
		}

		float delay = frame0->delay;

		// Calculate UVs
		float uv0x = uvs[0];
		float uv0y = uvs[1];
		float uv1x = uvs[2];
		float uv1y = uvs[3];

		// TODO: power of two textures handling
		// The client uses a power of two texture buffer, so we should emulate that.
		//float sx = texture.width / (float)texture.PotWidth;
		//float sy = texture.height / (float)texture.PotHeight;
		//uv0x *= sx;
		//uv0y *= sy;
		//uv1x *= sx;
		//uv1y *= sy;

		verts[0] = VertexP2T2(glm::vec2(positions[2], -positions[6]), glm::vec2(uv0x + uv1x, uv1y + uv0y));
		verts[1] = VertexP2T2(glm::vec2(positions[1], -positions[5]), glm::vec2(uv0x + uv1x, uv0y));
		verts[2] = VertexP2T2(glm::vec2(positions[0], -positions[4]), glm::vec2(uv0x, uv0y));
		verts[3] = VertexP2T2(glm::vec2(positions[3], -positions[7]), glm::vec2(uv0x, uv1y + uv0y));

		int textureIndex_i = (int)textureIndex;

		if (textureIndex_i < 0 || textureIndex_i >= textures[layerIdx].size())
			continue;

		textures[layerIdx][textureIndex_i]->bind();

		glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(VertexP2T2), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP2T2), verts[0].data + 2);

		float scale = 0.2f * strEffect->scaleratio * 0.7f;
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, 1.0f));
		modelMatrix = glm::translate(modelMatrix, glm::vec3(offset.x - 320.0f, -offset.y + 240.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, -glm::radians(angle), glm::vec3(0, 0, 1));

		shader->setUniform(StrShader::Uniforms::modelMatrix, modelMatrix);
		shader->setUniform(StrShader::Uniforms::modelPosition, glm::vec3(instanceMatrix[3]));
		shader->setUniform(StrShader::Uniforms::color, color);

		glBlendFuncSeparate(util::d3dToOpenGlBlend(frame0->blendSrc), util::d3dToOpenGlBlend(frame0->blendDst), GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_QUADS, 0, 4);
	}
#undef EASE
}

StrRenderer::StrRenderContext::StrRenderContext() : shader(util::ResourceManager<gl::Shader>::load<StrShader>())
{
	shader->use();
	shader->setUniform(StrShader::Uniforms::s_texture, 0);
	order = RendererDrawPriority::Str;
}

void StrRenderer::StrRenderContext::preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers)
{
	shader->use();
	shader->setUniform(StrShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(StrShader::Uniforms::cameraMatrix, context.viewMatrix);
	shader->setUniform(StrShader::Uniforms::color, glm::vec4(1, 1, 1, 1));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthMask(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (phase == 0) {
		// Order str effect renderers. This is only done for renderflag & 1, but it's expected to be a default flag.
		auto gnd = context.mapView->map->rootNode->getComponent<Gnd>();

		if (!gnd)
			return;

		glm::vec4 localPos(0.0f, 0.0f, context.mapView->cameraDistance, 1.0f);

		// Create camera vector from its distance
		glm::mat4 cameraMatrix = glm::mat4(1.0f);
		cameraMatrix = glm::rotate(cameraMatrix, glm::radians(context.mapView->cameraRot.x), glm::vec3(1, 0, 0));
		cameraMatrix = glm::rotate(cameraMatrix, glm::radians(context.mapView->cameraRot.y), glm::vec3(0, 1, 0));
		cameraMatrix = glm::translate(cameraMatrix, glm::vec3(0, 0, -context.mapView->cameraDistance));

		// Translate camera lookat position to RO position
		glm::vec3 cameraPos = context.mapView->cameraCenter;
		cameraPos = cameraPos - glm::vec3(5 * gnd->width, 0, 10 + 5 * gnd->height);
		cameraPos = cameraPos * glm::vec3(1, 1, -1);

		// Get final position from RO lookat position + camera position
		cameraPos = cameraPos + glm::vec3(cameraMatrix[3]);

		struct SortEntry {
			Renderer* renderer;
			float distanceSq;
		};

		std::vector<SortEntry> tempRenderers;
		tempRenderers.reserve(renderers.size());

		for (Renderer* r : renderers) {
			// Component lookups happen EXACTLY once per object here
			auto rswObject = r->node->getComponent<RswObject>();

			if (rswObject == nullptr)
				tempRenderers.push_back({ r, 0 });
			else {
				glm::vec3 objPos = rswObject->position;
				float distSq = glm::length2(objPos - cameraPos);

				tempRenderers.push_back({ r, distSq });
			}
		}

		std::sort(tempRenderers.begin(), tempRenderers.end(), [](const SortEntry& a, const SortEntry& b) { return a.distanceSq > b.distanceSq; });

		for (size_t i = 0; i < renderers.size(); ++i) {
			renderers[i] = tempRenderers[i].renderer;
		}
	}
}

void StrRenderer::StrRenderContext::postFrame(NodeRenderContext& context)
{
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
}