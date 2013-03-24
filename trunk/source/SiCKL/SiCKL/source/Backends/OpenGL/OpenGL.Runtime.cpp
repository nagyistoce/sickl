#include "Backends/OpenGL.h"

#include <stdint.h>

// opengl and friends
#include <GL/glew.h>
#include <GL/freeglut.h>

namespace SiCKL
{
	static bool _initialized = false;
	static int32_t _window_id = -1;
	static int32_t _max_texture_size = -1;
	static int32_t _max_viewport_dimensions[2] = {-1, -1};

	static GLint _vertex_shader = -1;

	bool OpenGLRuntime::Initialize()
	{
		if(_initialized == true)
		{
			return true;
		}

		static char* name = "JITStream OpenGL Runtime";
		static int32_t count = 1;

		glutInit(&count, &name);

		_window_id = glutCreateWindow(name);
		glutHideWindow();

		int major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		int version = major * 10 + minor;

		if(version != 33)	// version 3.3
		{
			glutDestroyWindow(_window_id);
			if(version < 33)
			{
				return false;
			}
			else
			{
				// newer context, rollback to old
				glutInitContextVersion(3, 3);
				_window_id = glutCreateWindow(name);
				glutHideWindow();
			}
		}

		// we have to set this to true or else we won't get all the functions we need
		glewExperimental=TRUE;
		auto result = glewInit();

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
			glutDestroyWindow(_window_id);
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
#pragma region Buffer1D

	OpenGLBuffer1D::OpenGLBuffer1D()
		: Length(-1)
		, Type(ReturnType::Invalid)
		, BufferHandle(-1)
		, TextureHandle(-1)
		, _counter(nullptr)
	{ }

	OpenGLBuffer1D::OpenGLBuffer1D(int32_t length, ReturnType::Type type, void* data)
		: Length(length)
		, Type(type)
		, BufferHandle(-1)
		, TextureHandle(-1)
	{
		_counter = new int32_t;
		*_counter = 1;

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

	OpenGLBuffer1D::OpenGLBuffer1D(const OpenGLBuffer1D& other)
		: Length(-1)
		, Type(ReturnType::Invalid)
		, TextureHandle(-1)
		, BufferHandle(-1)
		, _counter(nullptr)
	{
		*this = other;
	}

	OpenGLBuffer1D& OpenGLBuffer1D::operator=(const OpenGLBuffer1D& other)
	{
		std::memcpy(this, &other, sizeof(OpenGLBuffer1D));
		if(_counter != nullptr)
		{
			++(*_counter);
		}

		return *this;
	}

	OpenGLBuffer1D::~OpenGLBuffer1D()
	{
		if(_counter != nullptr)
		{
			--(*_counter);
			if(*_counter == 0)
			{
				delete _counter;
				// cleanup texture and buffer
				glDeleteTextures(1, &TextureHandle);
				glDeleteBuffers(1, &BufferHandle);
			}
		}
	}

	uint32_t OpenGLBuffer1D::GetBufferSize() const
	{
		return OpenGLRuntime::RequiredBufferSpace(Length, 1, Type);
	}

#pragma endregion

#pragma region Buffer2D

	OpenGLBuffer2D::OpenGLBuffer2D()
		: Width(-1)
		, Height(-1)
		, Type(ReturnType::Invalid)
		, TextureHandle(-1)
		, _counter(nullptr)
	{ }

	OpenGLBuffer2D::OpenGLBuffer2D(int32_t width, int32_t height, ReturnType::Type type, void* data)
		: Width(width)
		, Height(height)
		, Type(type)
		, TextureHandle(-1)
	{
		_counter = new int32_t;
		*_counter = 1;

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

	OpenGLBuffer2D::OpenGLBuffer2D(const OpenGLBuffer2D& other)
		: Width(-1)
		, Height(-1)
		, Type(ReturnType::Invalid)
		, TextureHandle(-1)
		, _counter(nullptr)
	{
		*this = other;
	}

	OpenGLBuffer2D& OpenGLBuffer2D::operator=(const OpenGLBuffer2D& other)
	{
		std::memcpy(this, &other, sizeof(OpenGLBuffer2D));
		if(_counter != nullptr)
		{
			++(*_counter);
		}

		return *this;
	}

	OpenGLBuffer2D::~OpenGLBuffer2D()
	{
		if(_counter != nullptr)
		{
			--(*_counter);
			if(*_counter == 0)
			{
				delete _counter;
				// cleanup texture handle
				glDeleteTextures(1, &TextureHandle);
			}
		}
	}

	uint32_t OpenGLBuffer2D::GetBufferSize() const
	{
		return OpenGLRuntime::RequiredBufferSpace(Width, Height, Type);
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
#pragma endregion
}