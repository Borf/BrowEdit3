#ifndef __VAO_H__
#define __VAO_H__

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <pewpew/vbo.h>

template<class T>
class VAO
{
private:
	GLuint vao;
	VAO(const VAO &other);
public:
	VAO(VBO<T>* vbo)
	{
		glGenVertexArraysOES(1, &vao);
		bind();
		vbo->bind();
		vbo->setVAO();
	}

	~VAO()
	{
		glDeleteVertexArraysOES(1, &vao);
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



#endif
