#pragma once

#include <string>
#include <glm/glm.hpp>

using GLuint = unsigned int;

namespace gl
{
	class Shader
	{
		std::string file;
		std::string header;
		int* uniforms;
	public:
		GLuint programId;

		Shader(const std::string& file, int end, std::string header = "");
		~Shader();
		void setUniform(int name, bool value);
		void setUniform(int name, int value);
		void setUniform(int name, float value);
		void setUniform(int name, const glm::mat3& value);
		void setUniform(int name, const glm::mat4& value);
		void setUniform(int name, const glm::vec2& value);
		void setUniform(int name, const glm::vec3& value);
		void setUniform(int name, const glm::vec4& value);

		void use();

		void bindUniform(int enumValue, const std::string& name);
	private:
		void addShader(GLuint type, const std::string& fileName);
	protected:
		virtual void bindUniforms() = 0;
	};
}