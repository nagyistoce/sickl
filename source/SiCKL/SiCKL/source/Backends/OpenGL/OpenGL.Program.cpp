#include "Backends/OpenGL.h"

#include <GL/glew.h>

namespace SiCKL
{
	OpenGLProgram::OpenGLProgram(const std::string& fragment_source, const ASTNode* uniforms, const ASTNode* outputs)
		: _source(fragment_source)
		, _vertex_array(-1)
		, _vertex_buffer(-1)
		, _frame_buffer(-1)
		, _uniform_count(-1)
		, _uniforms(nullptr)
		, _fragment_shader(-1)
		, _program(-1)
		, _size_handle(-1)
	{
		/// Build Shader Program

		int32_t fragment_source_length = _source.length();
		const char* fragment_source_buffer = _source.c_str();

		// compile the fragment shader
		_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(_fragment_shader, 1, (const GLchar**)&fragment_source_buffer, &fragment_source_length);
		glCompileShader(_fragment_shader);

		GLint compile_status = -1;
		glGetShaderiv(_fragment_shader, GL_COMPILE_STATUS, &compile_status);
		COMPUTE_ASSERT(compile_status == GL_TRUE );

		// create final program
		_program = glCreateProgram();

		// attach vertex shader
		glAttachShader(_program, OpenGLRuntime::GetVertexShader());
		// and our new fragment shader
		glAttachShader(_program, _fragment_shader);

		// link
		glLinkProgram(_program);

		GLint link_status = -1;
		glGetProgramiv(_program, GL_LINK_STATUS, &link_status);
		COMPUTE_ASSERT(link_status == GL_TRUE);

		/// Get the Uniforms

		// set the size uniform for the vertex shader
		_size_handle = glGetUniformLocation(_program, "size");
		COMPUTE_ASSERT(glGetError() == GL_NO_ERROR);

		_uniform_count = uniforms->_count;
		_uniforms = new Uniform[_uniform_count];

		uint32_t _texture_handle_counter = 0;

		for(uint32_t i = 0; i < uniforms->_count; i++)
		{
			ASTNode* n = uniforms->_children[i];
			Uniform& in = _uniforms[i];

			in._name = n->_name;
			in._param_location = glGetUniformLocation(_program, OpenGLCompiler::get_var_name(n->_u.sid).c_str());

			COMPUTE_ASSERT(glGetError() == GL_NO_ERROR);

			switch(n->_return_type)
			{
			// scalar values
			case ReturnType::Int:
			case ReturnType::Int2:
			case ReturnType::Int3:
			case ReturnType::Int4:
			case ReturnType::UInt:
			case ReturnType::UInt2:
			case ReturnType::UInt3:
			case ReturnType::UInt4:
			case ReturnType::Float:
			case ReturnType::Float2:
			case ReturnType::Float3:
			case ReturnType::Float4:
				in._type = n->_return_type;
				break;
			// samplers 
			default:
				// sampler2DRect
				if(n->_return_type & ReturnType::Buffer2D)
				{
					ReturnType::Type type = (ReturnType::Type)(n->_return_type ^ ReturnType::Buffer2D);
					switch(type)
					{
					case ReturnType::Int:
					case ReturnType::Int2:
					case ReturnType::Int3:
					case ReturnType::Int4:
					case ReturnType::UInt:
					case ReturnType::UInt2:
					case ReturnType::UInt3:
					case ReturnType::UInt4:
					case ReturnType::Float:
					case ReturnType::Float2:
					case ReturnType::Float3:
					case ReturnType::Float4:
						break;
					default:
						COMPUTE_ASSERT(false);

					}
					in._type = n->_return_type;
					in._sampler.texture_unit = GL_TEXTURE0 + _texture_handle_counter++;
				}
				else if(n->_return_type & ReturnType::Buffer1D)
				{
					COMPUTE_ASSERT(false);
				}
				else
				{
					COMPUTE_ASSERT(false);
				}
				break;
			}
			// also add in support for textures
		}
		/// And Get Outputs

		_output_count = outputs->_count;
		_outputs = new Output[_output_count];

		_render_buffers = new GLenum[_output_count];

		for(uint32_t i = 0; i < outputs->_count; i++)
		{
			ASTNode* n = outputs->_children[i];
			Output& out = _outputs[i];

			out._name = n->_name;
			out._texture_handle = -1;

			switch(n->_return_type)
			{
			case ReturnType::Int:
			case ReturnType::UInt:
			case ReturnType::Float:
			case ReturnType::Int2:
			case ReturnType::UInt2:
			case ReturnType::Float2:
			case ReturnType::Int3:
			case ReturnType::UInt3:
			case ReturnType::Float3:
			case ReturnType::Int4:
			case ReturnType::UInt4:
			case ReturnType::Float4:
				out._type = n->_return_type;
				break;
			default:
				COMPUTE_ASSERT(false);
			}
			_render_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
	}

	void OpenGLProgram::Initialize(int32_t in_width, int32_t in_height)
	{
		// make sure the passed in size is ok
		COMPUTE_ASSERT(in_width > 0 && in_height > 0);

		// and doesn't exceed our render viewport
		const int32_t* max_viewport_dimensions = OpenGLRuntime::GetMaxViewportSize();
		COMPUTE_ASSERT(in_width <= max_viewport_dimensions[0] && 
						in_height <= max_viewport_dimensions[1]);
		// or the max texture size
		const int32_t max_texture_size = OpenGLRuntime::GetMaxTextureSize();
		COMPUTE_ASSERT(in_width <= max_texture_size &&
						in_height <= max_texture_size);
		
		// set our size
		_size[0] = in_width;
		_size[1] = in_height;

		// define our render quad
		float vertices[] =
		{
			0.0f, 0.0f,
			(float)in_width, 0.0f,
			(float)in_width, (float)in_height,
			0.0f, (float)in_height,
		};

		// generate array object
		glGenVertexArrays(1, &_vertex_array);
		glBindVertexArray(_vertex_array);
		// generate the buffer object
		glGenBuffers(1, &_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// now generate our framebuffer
		glGenFramebuffers(1, &_frame_buffer);
	}

	OpenGLProgram::~OpenGLProgram()
	{
		delete[] _outputs;
		delete[] _uniforms;
		delete[] _render_buffers;
	}

	int32_t OpenGLProgram::GetUniformHandle(const char* in_name)
	{
		return -1;
	}

	int32_t OpenGLProgram::GetOutputHandle(const char* in_name)
	{
		return -1;
	}

	// setup inputs for shader

#define SET_UNIFORM_HEADER(return_type)\
	COMPUTE_ASSERT(index >= 0);\
	COMPUTE_ASSERT(_uniform_count > index);\
	COMPUTE_ASSERT(_uniforms[index]._type == return_type);\

#define SET_UNIFORM1(type_t, return_type, u)\
	void OpenGLProgram::SetUniform(int32_t index, type_t val0)\
	{\
		SET_UNIFORM_HEADER(return_type)\
		_uniforms[index].##u = val0;\
	}

#define SET_UNIFORM2(type_t, return_type, u)\
	void OpenGLProgram::SetUniform(int32_t index, type_t val0, type_t val1)\
	{\
		SET_UNIFORM_HEADER(return_type)\
		_uniforms[index].##u##.x = val0;\
		_uniforms[index].##u##.y = val1;\
	}

#define SET_UNIFORM3(type_t, return_type, u)\
	void OpenGLProgram::SetUniform(int32_t index, type_t val0, type_t val1, type_t val2)\
	{\
	SET_UNIFORM_HEADER(return_type)\
	_uniforms[index].##u##.x = val0;\
	_uniforms[index].##u##.y = val1;\
	_uniforms[index].##u##.z = val2;\
	}
	
#define SET_UNIFORM4(type_t, return_type, u)\
	void OpenGLProgram::SetUniform(int32_t index, type_t val0, type_t val1, type_t val2, type_t val3)\
	{\
	SET_UNIFORM_HEADER(return_type)\
	_uniforms[index].##u##.x = val0;\
	_uniforms[index].##u##.y = val1;\
	_uniforms[index].##u##.z = val2;\
	_uniforms[index].##u##.w = val3;\
	}

	SET_UNIFORM1(bool, ReturnType::Bool, _bool)
	SET_UNIFORM1(int32_t, ReturnType::Int, _int)
	SET_UNIFORM1(uint32_t, ReturnType::UInt, _uint)
	SET_UNIFORM1(float, ReturnType::Float, _float)

	SET_UNIFORM2(int32_t, ReturnType::Int2, _ivec)
	SET_UNIFORM2(uint32_t, ReturnType::UInt2, _uvec)
	SET_UNIFORM2(float, ReturnType::Float2, _fvec)

	SET_UNIFORM3(int32_t, ReturnType::Int3, _ivec)
	SET_UNIFORM3(uint32_t, ReturnType::UInt3, _uvec)
	SET_UNIFORM3(float, ReturnType::Float3, _fvec)

	SET_UNIFORM4(int32_t, ReturnType::Int4, _ivec)
	SET_UNIFORM4(uint32_t, ReturnType::UInt4, _uvec)
	SET_UNIFORM4(float, ReturnType::Float4, _fvec)

	/// Texture Setters
	void OpenGLProgram::SetUniform(int32_t index, const OpenGLBuffer1D& val)
	{
		// not yet implemented
		COMPUTE_ASSERT(false);
	}

	void OpenGLProgram::SetUniform(int32_t index, const OpenGLBuffer2D& val)
	{
		COMPUTE_ASSERT(index >= 0);
		COMPUTE_ASSERT(_uniform_count > index);		
		COMPUTE_ASSERT(_uniforms[index]._type & ReturnType::Buffer2D);
		COMPUTE_ASSERT((_uniforms[index]._type ^ ReturnType::Buffer2D) == val.Type);

		_uniforms[index]._sampler.handle = val.TextureHandle;
	}

	void OpenGLProgram::BindOutput(int32_t index, const OpenGLBuffer2D& output)
	{
		COMPUTE_ASSERT(index >= 0);
		COMPUTE_ASSERT(_output_count > index);

		COMPUTE_ASSERT(output.Type == _outputs[index]._type);
		COMPUTE_ASSERT(output.Width == _size[0]);
		COMPUTE_ASSERT(output.Height == _size[1]);

		_outputs[index]._texture_handle = output.TextureHandle;
	}

	void OpenGLProgram::Run()
	{
		glUseProgram(_program);
		// bind render targets
		glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
		for(int32_t i = 0; i < _output_count; i++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_RECTANGLE, _outputs[i]._texture_handle, 0);
		}
		glDrawBuffers(_output_count, _render_buffers);
		// setup our render viewport
		glViewport(0, 0, _size[0], _size[1]);
		// pass in the render target size for vertex shader
		glUniform2f(_size_handle, (float)_size[0], (float)_size[1]);
		for(int32_t i = 0; i < _uniform_count; i++)
		{
			Uniform& u = _uniforms[i];
			switch(u._type)
			{
			case ReturnType::Bool:
				glUniform1i(u._param_location, u._bool ? 1 : 0);
				break;
			case ReturnType::Int:
				glUniform1i(u._param_location, u._int);
				break;
			case ReturnType::UInt:
				glUniform1ui(u._param_location, u._uint);
				break;
			case ReturnType::Float:
				glUniform1f(u._param_location, u._float);
				break;
			case ReturnType::Int2:
				glUniform2i(u._param_location, u._ivec.x, u._ivec.y);
				break;
			case ReturnType::UInt2:
				glUniform2ui(u._param_location, u._uvec.x, u._uvec.y);
				break;
			case ReturnType::Float2:
				glUniform2f(u._param_location, u._fvec.x, u._fvec.y);
				break;
			case ReturnType::Int3:
				glUniform3i(u._param_location, u._ivec.x, u._ivec.y, u._ivec.z);
				break;
			case ReturnType::UInt3:
				glUniform3ui(u._param_location, u._uvec.x, u._uvec.y, u._uvec.z);
				break;
			case ReturnType::Float3:
				glUniform3f(u._param_location, u._fvec.x, u._fvec.y, u._fvec.z);
				break;
			case ReturnType::Int4:
				glUniform4i(u._param_location, u._ivec.x, u._ivec.y, u._ivec.z, u._ivec.w);
				break;
			case ReturnType::UInt4:
				glUniform4ui(u._param_location, u._uvec.x, u._uvec.y, u._uvec.z, u._uvec.w);
				break;
			case ReturnType::Float4:
				glUniform4f(u._param_location, u._fvec.x, u._fvec.y, u._fvec.z, u._fvec.w);
				break;
			default:
				if(u._type & ReturnType::Buffer2D)
				{
					glActiveTexture(u._sampler.texture_unit);
					glBindTexture(GL_TEXTURE_RECTANGLE, u._sampler.handle);
					glUniform1i(u._param_location, u._sampler.texture_unit - GL_TEXTURE0);
				}
				else if(u._type & ReturnType::Buffer1D)
				{
					// not yet implemented
					COMPUTE_ASSERT(false);
				}
				else
				{
					// unknown return type
					COMPUTE_ASSERT(false);
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
		glEnableVertexAttribArray(0);
		// draw
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		// disable array
		glDisableVertexAttribArray(0);
		// unbind the array buffer
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// unbind this program
		glUseProgram(0);
	}

	void OpenGLProgram::get_output(int32_t i, void** in_out_buffer)
	{
		COMPUTE_ASSERT(i < _output_count);

		if(*in_out_buffer == nullptr)
		{
			*in_out_buffer = malloc(OpenGLRuntime::RequiredBufferSpace(_size[0], _size[1], _outputs[i]._type));
		}
		int32_t format, type;

		switch(_outputs[i]._type)
		{
		case ReturnType::Int:
			format = GL_RED_INTEGER;
			type = GL_INT;
			break;
		case ReturnType::UInt:
			format = GL_RED_INTEGER;
			type = GL_UNSIGNED_INT;
			break;
		case ReturnType::Float:
			format = GL_RED;
			type = GL_FLOAT;
			break;
		case ReturnType::Int2:
			format = GL_RG_INTEGER;
			type = GL_INT;
			break;
		case ReturnType::UInt2:
			format = GL_RG_INTEGER;
			type = GL_UNSIGNED_INT;
			break;
		case ReturnType::Float2:
			format = GL_RG;
			type = GL_FLOAT;
			break;
		case ReturnType::Int3:
			format = GL_RGB_INTEGER;
			type = GL_INT;
			break;
		case ReturnType::UInt3:
			format = GL_RGB_INTEGER;
			type = GL_UNSIGNED_INT;
			break;
		case ReturnType::Float3:
			format = GL_RGB;
			type = GL_FLOAT;
			break;
		case ReturnType::Int4:
			format = GL_RGBA_INTEGER;
			type = GL_INT;
			break;
		case ReturnType::UInt4:
			format = GL_RGBA_INTEGER;
			type = GL_UNSIGNED_INT;
			break;
		case ReturnType::Float4:
			format = GL_RGBA;
			type = GL_FLOAT;
			break;
		default:
			COMPUTE_ASSERT(false);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
		{
			auto err = glGetError();
			COMPUTE_ASSERT(err == GL_NO_ERROR);
		}
		const int32_t& width = _size[0];
		const int32_t& height = _size[1];
		glReadPixels(0, 0, width, height, format, type, *in_out_buffer);

		// verify read happened ok
		auto er = glGetError();
		COMPUTE_ASSERT(er == GL_NO_ERROR);
	}
}