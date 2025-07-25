#include <browedit/util/FileIO.h>
#include "Shader.h"
#include <iostream>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace gl
{

	Shader::Shader(const std::string& file, int end) : file(file)
	{
		uniforms = new int[end];
		this->programId = glCreateProgram();
		if (util::FileIO::exists(file + ".vs"))
			addShader(GL_VERTEX_SHADER, file + ".vs");
		if (util::FileIO::exists(file + ".fs"))
			addShader(GL_FRAGMENT_SHADER, file + ".fs");

		glLinkProgram(programId);
		GLint status;
		glGetProgramiv(programId, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length, charsWritten;
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);
			char* infolog = new char[length + 1];
			memset(infolog, 0, length + 1);
			glGetProgramInfoLog(programId, length, &charsWritten, infolog);
			std::cerr << "Error compiling shader:\n" << infolog << std::endl;
			delete[] infolog;
			return;
		}
		glUseProgram(this->programId);
	}

	Shader::~Shader()
	{
		delete[] uniforms;
	}


	void Shader::use()
	{
		glUseProgram(this->programId);
	}



	void Shader::bindUniform(int enumValue, const std::string& name)
	{
		uniforms[enumValue] = glGetUniformLocation(programId, name.c_str());
		if (uniforms[enumValue] == -1)
			std::cerr << "Shader: In "<<this->file<<", uniform " << name << " not found!" << std::endl;;
	}


	void Shader::addShader(GLuint shaderType, const std::string& fileName)
	{
		std::istream* is = util::FileIO::open(fileName);
		std::string contents((std::istreambuf_iterator<char>(*is)), std::istreambuf_iterator<char>());
		delete is;

		GLuint shaderId = glCreateShader(shaderType);
		const char* shaderSource = contents.c_str();
		glShaderSource(shaderId, 1, &shaderSource, NULL);
		glCompileShader(shaderId);
		GLint status;
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length, charsWritten;
			glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
			char* infolog = new char[length + 1];
			memset(infolog, 0, length + 1);
			glGetShaderInfoLog(shaderId, length, &charsWritten, infolog);
			std::cerr << "Error compiling shader:\n" << infolog << std::endl;
			delete[] infolog;
			return;
		}
		glAttachShader(programId, shaderId);
	}

	void Shader::setUniform(int name, bool value)
	{
		glUniform1i(uniforms[name], value ? 1 : 0);
	}

	void Shader::setUniform(int name, int value)
	{
		glUniform1i(uniforms[name], value);
	}
	void Shader::setUniform(int name, float value)
	{
		glUniform1f(uniforms[name], value);
	}
	void Shader::setUniform(int name, const glm::mat3& value)
	{
		glUniformMatrix3fv(uniforms[name], 1, false, glm::value_ptr(value));
	}
	void Shader::setUniform(int name, const glm::mat4& value)
	{
		glUniformMatrix4fv(uniforms[name], 1, false, glm::value_ptr(value));
	}
	void Shader::setUniform(int name, const glm::vec2& value)
	{
		glUniform2fv(uniforms[name], 1, glm::value_ptr(value));
	}
	void Shader::setUniform(int name, const glm::vec3& value)
	{
		glUniform3fv(uniforms[name], 1, glm::value_ptr(value));
	}
	void Shader::setUniform(int name, const glm::vec4& value)
	{
		glUniform4fv(uniforms[name], 1, glm::value_ptr(value));
	}

}