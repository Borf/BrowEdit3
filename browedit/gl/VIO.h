#ifndef __VIO_H__
#define __VIO_H__

#include <blib/config.h>

#if defined(BLIB_IOS)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif defined(BLIB_ANDROID)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/glew.h>
#endif

#include <blib/util/NotCopyable.h>
#include <blib/VIO.h>

namespace blib
{
	namespace gl
	{
		class VIO : public blib::VIO, public blib::util::NotCopyable
		{
        private:
            // Leuke 'bug'. GLuinit is een simple usinged int. Vergelijken met een -1 why? en ook als contruct?
			GLuint vio;
			int length;
		public:
			VIO()
			{
				length = 0;
				vio = 0;
			}
			~VIO()
			{
				if(vio != 0)
					glDeleteBuffers(1, &vio);
			}

			void initGl()
			{
				if(vio == 0)
					glGenBuffers(1, &vio);
			}

			void setData(int length, void* data)
			{
				this->length = length;
				bind();
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementSize * length, data, GL_STATIC_DRAW);
			}


			void bind()
			{
				if(vio == 0)
					initGl();
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
			}

			void unBind()
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}

			int getLength()
			{
				return length;
			}

		};
	}
}

#endif
