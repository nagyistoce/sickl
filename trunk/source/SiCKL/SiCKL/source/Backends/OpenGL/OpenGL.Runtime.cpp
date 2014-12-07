#include "Backends/OpenGL.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

// opengl and friends
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// glfw is linked against newer version of C libs from VS 11 with a different pow method
// since we're building with VS 10, we need to define this func
#ifdef _WIN32
extern "C" double _libm_sse2_pow_precise(double a, double b)
{
    return pow(a,b);
}
#endif

namespace SiCKL
{
	static bool _initialized = false;
    //static int32_t _window_id = -1;
    GLFWwindow* _window = nullptr;
    static int32_t _max_texture_size = -1;
    static int32_t _max_viewport_dimensions[2] = {-1, -1};

	static GLint _vertex_shader = -1;

	static int32_t _version = -1;

	static inline int32_t GetOpenGLVersion()
	{
		return _version;
	}

	bool OpenGLRuntime::Initialize()
	{
		if(_initialized == true)
		{
			return true;
		}

        if(!glfwInit())
        {
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        static const char* name = "SiCKL OpenGL Runtime";
        _window = glfwCreateWindow(1, 1, name, nullptr, nullptr);
        if(_window == nullptr)
        {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(_window);
        glfwHideWindow(_window);
        int major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        int version = major * 10 + minor;

        if(version < 33)	// version 3.3
        {
            glfwTerminate();
            return false;
        }

        // save off our version
        _version = version;

		// we have to set this to true or else we won't get all the functions we need
		glewExperimental=true;
		auto result = glewInit();
		if(result != GLEW_OK)
		{
            glfwTerminate();
			return false;
		}

		while(glGetError() != GL_NO_ERROR);
        ///  Setup initial properties

		// fill polygons drawn
		// http://www.opengl.org/sdk/docs/man3/xhtml/glPolygonMode.xml
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// according to docs, these are the only two capabilities
		// enabeld by default (the rest are disabled)
		 // http://www.opengl.org/sdk/docs/man3/xhtml/glDisable.xml
		glDisable(GL_DITHER);
		glDisable(GL_MULTISAMPLE);
		// some helpful constants
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_max_texture_size);
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &_max_viewport_dimensions[0]);

		/// Compile our vertex shader all programs use

		const char* vertex_shader_source = 
			"#version 330\n"
			// calculate the texture coordinates
			"layout (location = 0) in vec2 position;\n"
			// viewport size
			"uniform vec2 size;\n"
			"noperspective out vec2 index;\n"
			"noperspective out vec2 normalized_index;\n"
			"void main(void)\n"
			"{\n"
			// convert to normalized device coordinates
			" gl_Position.xy = 2.0 * (position / size) - vec2(1.0, 1.0);\n"
			" gl_Position.z = 0.0;\n"
			" gl_Position.w =  1.0;\n"
			// and save off this position for interpolation
			" index = position;\n"
			" normalized_index = position / size;\n"
			"}\n";

		//printf("%s\n", vertex_shader_source);

		int32_t vertex_source_length = strlen(vertex_shader_source);
		_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(_vertex_shader, 1, (const GLchar**)&vertex_shader_source, &vertex_source_length);
		glCompileShader(_vertex_shader);

		GLint compile_status = -1;
		glGetShaderiv(_vertex_shader, GL_COMPILE_STATUS, &compile_status);
		COMPUTE_ASSERT(compile_status == GL_TRUE);

		_initialized = true;

		return true;
	}

	bool OpenGLRuntime::Finalize()
	{
		if(_initialized)
		{
            glfwTerminate();
			_initialized = false;
		}

		return true;
	}

	int32_t OpenGLRuntime::GetMaxTextureSize()
	{
		return _max_texture_size;
	}

	const int32_t* OpenGLRuntime::GetMaxViewportSize()
	{
		return &_max_viewport_dimensions[0];
	}

	GLint OpenGLRuntime::GetVertexShader()
	{
		return _vertex_shader;
	}

	uint32_t OpenGLRuntime::RequiredBufferSpace(uint32_t width, uint32_t height, ReturnType::Type type)
	{
		uint32_t texture_size = width * height;
		switch(type)
		{
		case ReturnType::Int:
		case ReturnType::UInt:
		case ReturnType::Float:
			texture_size *= 4;
			break;

		case ReturnType::Int2:
		case ReturnType::UInt2:
		case ReturnType::Float2:
			texture_size *= 8;
			break;
		case ReturnType::Int3:
		case ReturnType::UInt3:
		case ReturnType::Float3:
			texture_size *= 12;
			break;

		case ReturnType::Int4:
		case ReturnType::UInt4:
		case ReturnType::Float4:
			texture_size *= 16;
			break;

		default:
			COMPUTE_ASSERT(false);
		}

		return texture_size;
	}

	/// OpenGL Buffer Creation

	OpenGLBuffer1D::OpenGLBuffer1D()
		: Length(-1)
		, Type(ReturnType::Invalid)
		, BufferHandle(-1)
		, TextureHandle(-1)
	{ }

	OpenGLBuffer1D::OpenGLBuffer1D(int32_t length, ReturnType::Type type, void* data)
		: Length(length)
		, Type(type)
		, BufferHandle(-1)
		, TextureHandle(-1)
	{
		void* initial_data = data;
		if(data == nullptr)
		{
			const uint32_t buffer_size = GetBufferSize();
			initial_data = malloc(buffer_size);
			COMPUTE_ASSERT(initial_data != nullptr);
			memset(initial_data, 0x00, buffer_size);
		}

		// create buffer, allocate space and copy in data
		glGenBuffers(1, (GLuint*)&BufferHandle);
		glBindBuffer(GL_TEXTURE_BUFFER, BufferHandle);
		glBufferData(GL_TEXTURE_BUFFER, GetBufferSize(), initial_data, GL_STATIC_READ);
		
		// texture gen
		glGenTextures(1, (GLuint*)&TextureHandle);
		glBindTexture(GL_TEXTURE_BUFFER, TextureHandle);
		switch(type)
		{
		case ReturnType::Int:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, BufferHandle);
			break;
		case ReturnType::UInt:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, BufferHandle);
			break;
		case ReturnType::Float:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, BufferHandle);
			break;
		case ReturnType::Int2:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, BufferHandle);
			break;
		case ReturnType::UInt2:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32UI, BufferHandle);
			break;
		case ReturnType::Float2:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, BufferHandle);
			break;
		case ReturnType::Int3:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, BufferHandle);
			break;
		case ReturnType::UInt3:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32UI, BufferHandle);
			break;
		case ReturnType::Float3:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, BufferHandle);
			break;
		case ReturnType::Int4:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32I, BufferHandle);
			break;
		case ReturnType::UInt4:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, BufferHandle);
			break;
		case ReturnType::Float4:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, BufferHandle);
			break;
		default:
			COMPUTE_ASSERT(false);
		}

		// make sure the texture got created ok
		auto err = glGetError();
		COMPUTE_ASSERT(err == GL_NO_ERROR);

		// clenaup
		if(data == nullptr)
		{
			free(initial_data);
		}

		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}

	void OpenGLBuffer1D::Delete()
	{
		glDeleteTextures(1, &TextureHandle);
		glDeleteBuffers(1, &BufferHandle);
	}

	void OpenGLBuffer1D::SetData( void* in_buffer )
	{
		glBindBuffer(GL_TEXTURE_BUFFER, BufferHandle);
		glBufferSubData (GL_TEXTURE_BUFFER, 0, GetBufferSize(), in_buffer);
	}

	uint32_t OpenGLBuffer1D::GetBufferSize() const
	{
		return OpenGLRuntime::RequiredBufferSpace(Length, 1, Type);
	}

	OpenGLBuffer2D::OpenGLBuffer2D()
		: Width(-1)
		, Height(-1)
		, Type(ReturnType::Invalid)
		, TextureHandle(-1)
	{ }

	OpenGLBuffer2D::OpenGLBuffer2D(int32_t width, int32_t height, ReturnType::Type type, void* data)
		: Width(width)
		, Height(height)
		, Type(type)
		, TextureHandle(-1)
	{
		glGenTextures(1, (GLuint*)&TextureHandle);
		glBindTexture(GL_TEXTURE_RECTANGLE, TextureHandle);

		// nearest neighbor sampling
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// clamp uv coordinates
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		void* initial_data = data;
		if(data == nullptr)
		{
			const uint32_t buffer_size = GetBufferSize();
			initial_data = malloc(buffer_size);
			COMPUTE_ASSERT(initial_data != nullptr);
			memset(initial_data, 0x00, buffer_size);
		}

		switch(type)
		{
		// single
		case ReturnType::Int:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, initial_data);
			break;
		case ReturnType::UInt:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, initial_data);
			break;
		case ReturnType::Float:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, initial_data);
			break;
		// double
		case ReturnType::Int2:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RG32I, width, height, 0, GL_RG_INTEGER, GL_INT, initial_data);
			break;
		case ReturnType::UInt2:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RG32UI, width, height, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, initial_data);
			break;
		case ReturnType::Float2:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, initial_data);
			break;
		// triple
		case ReturnType::Int3:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32I, width, height, 0, GL_RGB_INTEGER, GL_INT, initial_data);
			break;
		case ReturnType::UInt3:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, initial_data);
			break;
		case ReturnType::Float3:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, initial_data);
			break;
		// quad
		case ReturnType::Int4:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32I, width, height, 0, GL_RGBA_INTEGER, GL_INT, initial_data);
			break;
		case ReturnType::UInt4:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32UI, width, height, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, initial_data);
			break;
		case ReturnType::Float4:
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, initial_data);
			break;
		default:
			COMPUTE_ASSERT(false);
		}
		 
		// make sure the texture got created ok
		auto err = glGetError();
		COMPUTE_ASSERT(err == GL_NO_ERROR);

		if(data == nullptr)
		{
			free(initial_data);
		}

		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}

	void OpenGLBuffer2D::Delete()
	{
		// cleanup texture handle
		glDeleteTextures(1, &TextureHandle);
	}

	uint32_t OpenGLBuffer2D::GetBufferSize() const
	{
		return OpenGLRuntime::RequiredBufferSpace(Width, Height, Type);
	}

    void OpenGLBuffer2D::SetData(const OpenGLBuffer2D& in_buffer)
	{
		COMPUTE_ASSERT(this != &in_buffer);

		COMPUTE_ASSERT(this->Width == in_buffer.Width);
		COMPUTE_ASSERT(this->Height == in_buffer.Height);
		COMPUTE_ASSERT(this->Type == in_buffer.Type);

        // old versions

		if(GetOpenGLVersion() < 43)
		{
			// Texture -> CPU -> Texture copy
            void* buffer = nullptr;
            in_buffer.GetData(buffer);

            this->SetData(buffer);
            free(buffer);
		}
		else
		{
			glCopyImageSubData(in_buffer.TextureHandle, GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, this->TextureHandle, GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, this->Width, this->Height, 1);

			// make sure the texture got created ok
			auto err = glGetError();
			COMPUTE_ASSERT(err == GL_NO_ERROR);
			// Texture -> Texture copy
		}
	}

    void OpenGLBuffer2D::SetData( const void* in_buffer )
	{
		glBindTexture(GL_TEXTURE_RECTANGLE, TextureHandle);
		switch(Type)
		{
			// single
		case ReturnType::Int:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RED_INTEGER, GL_INT, in_buffer);
			break;
		case ReturnType::UInt:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RED_INTEGER, GL_UNSIGNED_INT, in_buffer);
			break;
		case ReturnType::Float:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RED, GL_FLOAT, in_buffer);
			break;
			// double
		case ReturnType::Int2:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RG_INTEGER, GL_INT, in_buffer);
			break;
		case ReturnType::UInt2:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RG_INTEGER, GL_UNSIGNED_INT, in_buffer);
			break;
		case ReturnType::Float2:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RG, GL_FLOAT, in_buffer);
			break;
			// triple
		case ReturnType::Int3:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGB_INTEGER, GL_INT, in_buffer);
			break;
		case ReturnType::UInt3:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGB_INTEGER, GL_UNSIGNED_INT, in_buffer);
			break;
		case ReturnType::Float3:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGB, GL_FLOAT, in_buffer);
			break;
			// quad
		case ReturnType::Int4:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGBA_INTEGER, GL_INT, in_buffer);
			break;
		case ReturnType::UInt4:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, in_buffer);
			break;
		case ReturnType::Float4:
			glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, Width, Height, GL_RGBA, GL_FLOAT, in_buffer);
			break;
		default:
			COMPUTE_ASSERT(false);
		}
	}

	void OpenGLBuffer2D::get_data(void** in_out_buffer) const
	{
		if(*in_out_buffer == nullptr)
		{
			*in_out_buffer = malloc(GetBufferSize());
		}

		glBindTexture(GL_TEXTURE_RECTANGLE, TextureHandle);

		switch(Type)
		{
		// single
		case ReturnType::Int:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RED_INTEGER, GL_INT, *in_out_buffer);
			break;
		case ReturnType::UInt:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, *in_out_buffer);
			break;
		case ReturnType::Float:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RED, GL_FLOAT, *in_out_buffer);
			break;
		// double
		case ReturnType::Int2:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RG_INTEGER, GL_INT, *in_out_buffer);
			break;
		case ReturnType::UInt2:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, *in_out_buffer);
			break;
		case ReturnType::Float2:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RG, GL_FLOAT, *in_out_buffer);
			break;
		// triple
		case ReturnType::Int3:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGB_INTEGER, GL_INT, *in_out_buffer);
			break;
		case ReturnType::UInt3:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, *in_out_buffer);
			break;
		case ReturnType::Float3:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGB, GL_FLOAT, *in_out_buffer);
			break;
		// quad
		case ReturnType::Int4:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGBA_INTEGER, GL_INT, *in_out_buffer);
			break;
		case ReturnType::UInt4:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, *in_out_buffer);
			break;
		case ReturnType::Float4:
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, GL_FLOAT, *in_out_buffer);
			break;
		default:
			COMPUTE_ASSERT(false);
		}
	}
}
