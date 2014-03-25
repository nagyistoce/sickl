#include "Backends/OpenGL.h"

#include <string>
#include <stack>

using namespace std;

#undef If
#undef ElseIf
#undef Else

namespace SiCKL
{
#pragma region GLSL generation
	// generates names of the form a, b c, ... aa, ab, ac, ... ba, bb, etc
	std::string OpenGLCompiler::get_var_name( symbol_id_t x )
	{
#		define LETTER(X) char('a' + (X))

		stack<char> digits;

		digits.push(LETTER(x % 26));
		x -= x % 26;

		while(x > 0)
		{
			x /= 26;
			digits.push(LETTER((x - 1) % 26));
			if((x % 26) == 0)
			{
				x -= 26;
			}
			else
			{
				x -= x % 26;
			}
		}
		stringstream ss;
		while(digits.size())
		{
			ss << digits.top();
			digits.pop();
		}
		return ss.str();
#		undef LETTER
	}

	void OpenGLCompiler::print_var(symbol_id_t x)
	{
		_ss << get_var_name(x);
	}

	void OpenGLCompiler::print_type(ReturnType::Type type)
	{
		switch(type)
		{
		case ReturnType::Bool:
			_ss << "bool";
			break;
		case ReturnType::Int:
			_ss << "int";
			break;
		case ReturnType::UInt:
			_ss << "uint";
			break;
		case ReturnType::Float:
			_ss << "float";
			break;
		case ReturnType::Int2:
			_ss << "ivec2";
			break;
		case ReturnType::UInt2:
			_ss << "uvec2";
			break;
		case ReturnType::Float2:
			_ss << "vec2";
			break;
		case ReturnType::Int3:
			_ss << "ivec3";
			break;
		case ReturnType::UInt3:
			_ss << "uvec3";
			break;
		case ReturnType::Float3:
			_ss << "vec3";
			break;
		case ReturnType::Int4:
			_ss << "ivec4";
			break;
		case ReturnType::UInt4:
			_ss << "uvec4";
			break;
		case ReturnType::Float4:
			_ss << "vec4";
			break;
		default:
			if(type & ReturnType::Buffer1D)
			{
				type = (ReturnType::Type)(type ^ ReturnType::Buffer1D);
				switch(type)
				{
				case ReturnType::Int:
				case ReturnType::Int2:
				case ReturnType::Int3:
				case ReturnType::Int4:
					_ss << "isamplerBuffer";
					break;
				case ReturnType::UInt:
				case ReturnType::UInt2:
				case ReturnType::UInt3:
				case ReturnType::UInt4:
					_ss << "usamplerBuffer";
					break;
				case ReturnType::Float:
				case ReturnType::Float2:
				case ReturnType::Float3:
				case ReturnType::Float4:
					_ss << "samplerBuffer";
					break;
				default:
					COMPUTE_ASSERT(false);
					break;
				}
			}
			else if(type & ReturnType::Buffer2D)
			{
				type = (ReturnType::Type)(type ^ ReturnType::Buffer2D);
				switch(type)
				{
				case ReturnType::Int:
				case ReturnType::Int2:
				case ReturnType::Int3:
				case ReturnType::Int4:
					_ss << "isampler2DRect";
					break;
				case ReturnType::UInt:
				case ReturnType::UInt2:
				case ReturnType::UInt3:
				case ReturnType::UInt4:
					_ss << "usampler2DRect";
					break;
				case ReturnType::Float:
				case ReturnType::Float2:
				case ReturnType::Float3:
				case ReturnType::Float4:
					_ss << "sampler2DRect";
					break;
				default:
					COMPUTE_ASSERT(false);
					break;
				}
			}
			else
			{
				// unknown return type
				COMPUTE_ASSERT(false);
			}
		}
	}

	void OpenGLCompiler::print_declaration(symbol_id_t symbol_id, ReturnType::Type type)
	{
		print_type(type);
		_ss << " ";
		print_var(symbol_id);
		_ss << ";" << endl;
	}
	void OpenGLCompiler::print_indent()
	{
		for(uint32_t i = 0; i < _indent; i++)
		{
			_ss << " ";
		}
	}
	void OpenGLCompiler::print_newline(const ASTNode* node)
	{
		switch(node->_node_type)
		{
		default:
			_ss << ";" << endl;
		case NodeType::Block:
		case NodeType::If:
		case NodeType::ElseIf:
		case NodeType::Else:
		case NodeType::While:
		case NodeType::ForInRange:
			break;
		}
	}

	void OpenGLCompiler::print_operator(const char* op, const ASTNode* left, const ASTNode* right)
	{
		_ss << "(";
		print_code(left);
		_ss << " " << op << " ";
		print_code(right);
		_ss << ")";
	}

	void OpenGLCompiler::print_code(const ASTNode* node)
	{
		switch(node->_node_type)
		{
		case NodeType::Main:
			// no nested mains mk
			COMPUTE_ASSERT(_indent == 0);
			_indent++;

			for(uint32_t i = 0; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}

			break;
		case NodeType::Var:
			print_var(node->_u.sid);
			break;
		case NodeType::Literal:
			switch(node->_return_type)
			{
			case ReturnType::Bool:
				_ss << ((*(bool*)node->_u.literal.data) == true ? "true" : "false");
				break;
			case ReturnType::Int:
				_ss << *(int32_t*)node->_u.literal.data;
				break;
			case ReturnType::UInt:
				_ss << *(uint32_t*)node->_u.literal.data << "u";
				break;
			case ReturnType::Float:
				{
					float val =  *(float*)node->_u.literal.data;
					char buffer[128];
					int32_t length = sprintf(buffer, "%f", val);
					for(int i = length - 1; 
						(buffer[i - 1]) >= '0' && (buffer[i - 1] <= '9')
						&& buffer[i] == '0'; --i)
					{
						buffer[i] = 0;
					}
					_ss << buffer << "f";
				}
				break;
			}
			break;
		case NodeType::Member:
			COMPUTE_ASSERT(node->_count == 2);
			switch(node->_children[0]->_return_type)
			{
			case ReturnType::Int2:
			case ReturnType::UInt2:
			case ReturnType::Float2:
			case ReturnType::Int3:
			case ReturnType::UInt3:
			case ReturnType::Float3:
			case ReturnType::Int4:
			case ReturnType::UInt4:
			case ReturnType::Float4:
				break;
			default:
				COMPUTE_ASSERT(false);
			}
			COMPUTE_ASSERT(node->_children[1]->_node_type == NodeType::Literal);

			print_code(node->_children[0]);
			switch(*(int32_t*)node->_children[1]->_u.literal.data)
			{
			case 0:
				_ss << ".x";
				break;
			case 1:
				_ss << ".y";
				break;
			case 2:
				_ss << ".z";
				break;
			case 3:
				_ss << ".w";
				break;
			default:
				// invalid member id
				COMPUTE_ASSERT(false);
			}
			break;
			/// Operators
		case NodeType::Assignment:
			COMPUTE_ASSERT(node->_count == 2);
			// var name
			if(node->_children[0]->_node_type == NodeType::Var 
				&& _declared_vars.find(node->_children[0]->_u.sid) == _declared_vars.end())
			{
				print_type(node->_children[0]->_return_type);
				_ss << " ";
				_declared_vars.insert(node->_children[0]->_u.sid);
			}

			print_code(node->_children[0]);
			_ss << " = ";
			// values
			print_code(node->_children[1]);
			break;
		case NodeType::Equal:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("==", node->_children[0], node->_children[1]);
			break;
		case NodeType::NotEqual:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("!=", node->_children[0], node->_children[1]);
			break;
		case NodeType::Greater:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator(">", node->_children[0], node->_children[1]);
			break;
		case NodeType::GreaterEqual:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator(">=", node->_children[0], node->_children[1]);
			break;
		case NodeType::Less:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("<", node->_children[0], node->_children[1]);
			break;
		case NodeType::LessEqual:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("<=", node->_children[0], node->_children[1]);
			break;
		case NodeType::LogicalAnd:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("&&", node->_children[0], node->_children[1]);
			break;
		case NodeType::LogicalOr:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("||", node->_children[0], node->_children[1]);
			break;
		case NodeType::LogicalNot:
			COMPUTE_ASSERT(node->_count == 1);
			_ss << "!(";
			print_code(node->_children[0]);
			_ss << ")";
			break;
		case NodeType::BitwiseAnd:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("&", node->_children[0], node->_children[1]);
			break;
		case NodeType::BitwiseOr:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("|", node->_children[0], node->_children[1]);
			break;
		case NodeType::BitwiseXor:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("^", node->_children[0], node->_children[1]);
			break;
		case NodeType::BitwiseNot:
			COMPUTE_ASSERT(node->_count == 1);
			_ss << "~(";
			print_code(node->_children[0]);
			_ss << ")";
			break;
		case NodeType::LeftShift:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("<<", node->_children[0], node->_children[1]);
			break;
		case NodeType::RightShift:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator(">>", node->_children[0], node->_children[1]);
			break;
			break;
		case NodeType::UnaryMinus:
			COMPUTE_ASSERT(node->_count == 1);
			_ss << "-";
			print_code(node->_children[0]);
			break;
		case NodeType::Add:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("+", node->_children[0], node->_children[1]);
			break;
		case NodeType::Subtract:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("-", node->_children[0], node->_children[1]);
			break;
		case NodeType::Multiply:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("*", node->_children[0], node->_children[1]);
			break;
		case NodeType::Divide:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("/", node->_children[0], node->_children[1]);
			break;
		case NodeType::Modulo:
			COMPUTE_ASSERT(node->_count == 2);
			print_operator("%", node->_children[0], node->_children[1]);
			break;
			/// Control Flow
		case NodeType::Block:
			_ss << "{" << endl;
			_indent++;
			for(uint32_t i = 0; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}
			_indent--;
			print_indent();
			_ss << "}" << endl;
			break;
		case NodeType::If:
			COMPUTE_ASSERT(node->_count >= 1);
			_ss << "if ( ";
			// conditional
			print_code(node->_children[0]);
			_ss << " )" << endl;
			print_indent();
			_ss << "{" << endl;
			_indent++;

			for(uint32_t i = 1; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}
			_indent--;
			print_indent();
			_ss << "}" << endl;
			break;
		case NodeType::ElseIf:
			COMPUTE_ASSERT(node->_count >= 1);
			_ss << "else if ( ";
			// conditional
			print_code(node->_children[0]);
			_ss << " )" << endl;
			print_indent();
			_ss << "{" << endl;
			_indent++;

			for(uint32_t i = 1; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}

			_indent--;
			print_indent();
			_ss << "}" << endl;
			break;
		case NodeType::Else:
			COMPUTE_ASSERT(node->_count >= 0);
			_ss << "else" << endl;
			print_indent();
			_ss << "{" << endl;
			_indent++;

			for(uint32_t i = 0; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}

			_indent--;
			print_indent();
			_ss << "}" << endl;
			break;
		case NodeType::While:
			COMPUTE_ASSERT(node->_count >= 1);
			_ss << "while ( ";
			// conditional
			print_code(node->_children[0]);
			_ss << " )" << endl;
			print_indent();
			_ss << "{" << endl;
			_indent++;

			for(uint32_t i = 1; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}
			_indent--;
			print_indent();
			_ss << "}" << endl;
			break;
		case NodeType::ForInRange:
			COMPUTE_ASSERT(node->_count >= 3)
			COMPUTE_ASSERT(node->_children[0]->_return_type == ReturnType::Int);
			COMPUTE_ASSERT(node->_children[1]->_node_type == NodeType::Literal);
			COMPUTE_ASSERT(node->_children[1]->_return_type == ReturnType::Int);
			COMPUTE_ASSERT(node->_children[2]->_node_type == NodeType::Literal);
			COMPUTE_ASSERT(node->_children[2]->_return_type == ReturnType::Int);

			_ss << "for (int ";
			print_var(node->_children[0]->_u.sid);
			_ss << " = ";
			_ss << *(int32_t*)node->_children[1]->_u.literal.data;
			_ss << "; ";
			print_var(node->_children[0]->_u.sid);
			_ss << " < ";
			_ss << *(int32_t*)node->_children[2]->_u.literal.data;
			_ss << "; ++";
			print_var(node->_children[0]->_u.sid);
			_ss << ") "  << endl;
			
			print_indent();
			_ss << "{" << endl;
			_indent++;

			for(uint32_t i = 3; i < node->_count; i++)
			{
				print_indent();
				print_code(node->_children[i]);
				print_newline(node->_children[i]);
			}
			_indent--;
			print_indent();
			_ss << "}" << endl;

			break;
		/// Functions
		case NodeType::Constructor:
			print_type(node->_return_type);
			_ss << "(";
			for(uint32_t i = 0; i < node->_count; i++)
			{
				if(i != 0)
				{
					_ss << ", ";
				}
				print_code(node->_children[i]);
			}
			_ss << ")";
			break;
		case NodeType::Cast:
			COMPUTE_ASSERT(node->_count == 1);
			print_type(node->_return_type);
			_ss << "(";
			print_code(node->_children[0]);
			_ss << ")";
			break;
		case NodeType::Function:
			COMPUTE_ASSERT(node->_count >= 1);
			print_function(node);
			break;
		case NodeType::Sample1D:
			COMPUTE_ASSERT(node->_count == 2);
			_ss << "texelFetch(";
			print_var(node->_children[0]->_u.sid);
			_ss << ", ";
			print_code(node->_children[1]);
			_ss << ")";
			switch(node->_return_type)
			{
			case ReturnType::Int:
			case ReturnType::UInt:
			case ReturnType::Float:
				_ss << ".x";
				break;
			case ReturnType::Int2:
			case ReturnType::UInt2:
			case ReturnType::Float2:
				_ss << ".xy";
				break;
			case ReturnType::Int3:
			case ReturnType::UInt3:
			case ReturnType::Float3:
				_ss << ".xyz";
				break;
			case ReturnType::Int4:
			case ReturnType::UInt4:
			case ReturnType::Float4:
				_ss << ".xyzw";
				break;
			}
			break;
		case NodeType::Sample2D:
			COMPUTE_ASSERT(node->_count == 2 || node->_count == 3);
			_ss << "texelFetch(";
			print_var(node->_children[0]->_u.sid);
			_ss << ", ";

			if(node->_count == 2)
			{
				COMPUTE_ASSERT(node->_children[1]->_return_type == ReturnType::Int2);
				print_code(node->_children[1]);
				_ss << ")";
			}
			else if(node->_count == 3)
			{
				_ss << "ivec2(";
				print_code(node->_children[1]);
				_ss << ",";
				print_code(node->_children[2]);
				_ss << "))";
			}
			switch(node->_return_type)
			{
			case ReturnType::Int:
			case ReturnType::UInt:
			case ReturnType::Float:
				_ss << ".x";
				break;
			case ReturnType::Int2:
			case ReturnType::UInt2:
			case ReturnType::Float2:
				_ss << ".xy";
				break;
			case ReturnType::Int3:
			case ReturnType::UInt3:
			case ReturnType::Float3:
				_ss << ".xyz";
				break;
			case ReturnType::Int4:
			case ReturnType::UInt4:
			case ReturnType::Float4:
				_ss << ".xyzw";
				break;
			}
			break;
		case NodeType::GetIndex:
			// cast the interpolated vec2 index to integer
			_ss << "ivec2(index)";
			break;
		case NodeType::GetNormalizedIndex:
			// on 0,1
			_ss << "normalized_index";
			break;
		default:
			// unknown AST node type
			COMPUTE_ASSERT(false);
		}
	}

	// function built ins
	void OpenGLCompiler::print_function(const ASTNode* node)
	{
		COMPUTE_ASSERT(node->_children[0]->_node_type == NodeType::Literal);
		COMPUTE_ASSERT(node->_children[0]->_return_type == ReturnType::Int);
		const int32_t func_id = (*(int32_t*)node->_children[0]->_u.literal.data);

		switch(func_id)
		{
		case BuiltinFunction::Index:
			COMPUTE_ASSERT(node->_count == 1);
			_ss << "ivec2(index)";
			break;
		case BuiltinFunction::NormalizedIndex:
			COMPUTE_ASSERT(node->_count == 1);
			_ss << "normalized_index";
			break;
		default:
			const char* function_names[] =
			{
				nullptr,
				nullptr,
				"sin",
				"cos",
				"tan",
				"asin",
				"acos",
				"atan",
				"sinh",
				"cosh",
				"tanh",
				"asinh",
				"acosh",
				"atanh",
				"pow",
				"exp",
				"log",
				"exp2",
				"log2",
				"sqrt",
				"abs",
				"sign",
				"floor",
				"ceil",
				"min",
				"max",
				"clamp",
				"isnan",
				"isinf",
				"length",
				"distance",
				"dot",
				"cross",
				"normalize",
			};
			_ss << function_names[func_id] << "(";
			for(uint32_t i = 1; i < node->_count; i++)
			{
				if(i != 1)
				{
					_ss << ", ";
				}

				print_code(node->_children[i]);
			}
			_ss << ")";
			break;
		}
	}

	void OpenGLCompiler::print_glsl(const ASTNode* const_data, const ASTNode* out_data, const ASTNode* main)
	{
		_declared_vars.clear();
		// build GLSL source
		_ss << "#version 330" << endl << endl;

		/** Index from vertex shader **/
		_ss << "// from vertex shader" << endl;
		_ss << "noperspective in vec2 index;" << endl;
		_ss << "noperspective in vec2 normalized_index;" << endl << endl;

		/** Uniform Data **/
		_ss << "// uniform inputs" << endl;
		for(uint32_t i = 0; i < const_data->_count; i++)
		{
			_ss << "uniform ";
			print_declaration(const_data->_children[i]->_u.sid, const_data->_children[i]->_return_type);
			_declared_vars.insert(const_data->_children[i]->_u.sid);
		}
		_ss << endl;

		/** Output Data **/
		_ss << "// outputs" << endl;
		for(uint32_t i = 0; i < out_data->_count; i++)
		{
			_ss << "layout (location = " << (i) << ") out ";
			print_declaration(out_data->_children[i]->_u.sid, out_data->_children[i]->_return_type);
			_declared_vars.insert(out_data->_children[i]->_u.sid);
		}
		_ss << endl;


		/** Main **/
		_ss << "void main()" << endl << "{" << endl;
		_ss << " // code" << endl;
		_indent = 0;
		print_code(main);
		_ss << "}" << endl;
	}
#pragma endregion

	OpenGLProgram* OpenGLCompiler::Build(const Source& in_source)
	{
		_ss = std::stringstream();

		const ASTNode* root = &in_source.GetRoot();
		const ASTNode* const_data = nullptr;
		const ASTNode* out_data = nullptr;
		const ASTNode* main = nullptr;

		// main, const data and out data
		COMPUTE_ASSERT(root->_count == 3);	

		// get the three important nodes here
		for(uint32_t i = 0; i < root->_count; i++)
		{
			switch(root->_children[i]->_node_type)
			{
			case NodeType::ConstData:
				const_data = root->_children[i];
				break;
			case NodeType::OutData:
				out_data = root->_children[i];
				break;
			case NodeType::Main:
				main = root->_children[i];
				break;
			}
		}

		COMPUTE_ASSERT(const_data != nullptr &&
			out_data != nullptr &&
			main != nullptr);
		/// Generate GLSL

		print_glsl(const_data, out_data, main);


		/// Generate Program Interface
			
		OpenGLProgram* result = new OpenGLProgram(_ss.str(), const_data, out_data);


		return result;
	}

	
}
