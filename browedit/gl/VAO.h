#pragma once

#include <glad/gl.h>
#include <vector>

namespace gl
{
	class VAO
	{
	private:
		GLuint vao;
		VAO(const VAO& other);
	public:
		VAO()
		{
			glGenVertexArrays(1, &vao);
		}

		~VAO()
		{
			glDeleteVertexArrays(1, &vao);
		}

		void bind()
		{
			glBindVertexArray(vao);
		}

		void unBind()
		{
			glBindVertexArray(0);
		}
	};
}
