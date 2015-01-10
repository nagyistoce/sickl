#include "SiCKL.h"

namespace SiCKL
{
	const char* NodeTypes[] =
	{
		"invalid",
		"program",
		"const_block",
		"out_block",
		"main_block",
		"block",

		"if",
		"elseif",
		"else",
		"while",
		"forinrange",

		"out_var",
		"const_var",
		"var",
		"literal",

		":=",

		"==",
		"!=",
		">",
		">=",
		"<",
		"<=",

		"and",
		"or",
		"not",

		"&",
		"|",
		"^",
		"~",

		"<<",
		">>",

		"-",
		"+",
		"-",
		"*",
		"/",
		"%",

		"constructor",
		"cast",
		"function",

		"sample1d",
		"sample2d",
		"member",
		"get_index",
		"get_normalized_index",
	};

    static_assert(count_of(NodeTypes) == (NodeType::Max + 1), "Mismatch between NodeTypes string and enum");

#define RETURN_TYPE_SWITCH(F, B)	\
	switch(type)\
	{\
	case ReturnType::Bool:\
		return F "bool" B;\
	case ReturnType::Int:\
		return F "int" B;\
	case ReturnType::UInt:\
		return F "uint" B;\
	case ReturnType::Float:\
		return F "float" B;\
	case ReturnType::Int2:\
		return F "int2" B;\
	case ReturnType::UInt2:\
		return F "uint2" B;\
	case ReturnType::Float2:\
		return F "float2" B;\
	case ReturnType::Int3:\
		return F "int3" B;\
	case ReturnType::UInt3:\
		return F "uint3" B;\
	case ReturnType::Float3:\
		return F "float3" B;\
	case ReturnType::Int4:\
		return F "int4" B;\
	case ReturnType::UInt4:\
		return F "uint4" B;\
	case ReturnType::Float4:\
		return F "float4" B;\
	default:\
		COMPUTE_ASSERT(false);\
	}\
		
	const char* GetReturnType(ReturnType::Type type)
	{
		if(type == ReturnType::Invalid)
		{
			return "invalid";
		}
		else if(type == ReturnType::Void)
		{
			return "void";
		}

		if(type & ReturnType::Buffer1D)
		{
			type = (ReturnType::Type)(type ^ ReturnType::Buffer1D);
			COMPUTE_ASSERT(type != ReturnType::Void);
			RETURN_TYPE_SWITCH("buffer1d<",">")
		}
		else if(type & ReturnType::Buffer2D)
		{
			type = (ReturnType::Type)(type ^ ReturnType::Buffer2D);
			COMPUTE_ASSERT(type != ReturnType::Void);
			RETURN_TYPE_SWITCH("buffer2d<",">")
		}
		else
		{
			RETURN_TYPE_SWITCH("","")
		}
		COMPUTE_ASSERT(false);
		return "unknown";
	}

	/// struct Data implementation

	Data::Data()
		: _id(invalid_symbol)
		, _node(nullptr)
		, _name(nullptr)
	{ }

	Data::Data(symbol_id_t in_id, struct ASTNode* in_node, const char* in_name)
		: _id(in_id)
		, _node(in_node)
		, _name(in_name)
	{ }

	Data::~Data()
	{
		if(_node)
		{
			delete _node;
			_node = nullptr;
		}
	}




	/// AST related guff

	ASTNode::ASTNode()
		: _node_type(NodeType::Invalid)
		, _count(0)
		, _capacity(1)
		, _return_type(ReturnType::Invalid)
        , _u({0})
        , _name(nullptr)
	{
		_children = new ASTNode*[_capacity];
		_children[0] = nullptr;
	}

	ASTNode::ASTNode(const ASTNode& in_node)
        : _u({0})
	{
		_node_type = in_node._node_type;
		_return_type = in_node._return_type;
		_name = in_node._name;

		if(_node_type == NodeType::Literal)
		{
			_u.literal.size = in_node._u.literal.size;
			_u.literal.data = malloc(_u.literal.size);
			memcpy(_u.literal.data, in_node._u.literal.data, _u.literal.size);
		}
		else
		{
			_u = in_node._u;
		}

		_count = in_node._count;
		if(_count == 0)
		{
			_capacity = 1;
			_children = new ASTNode*[1];
			_children[0] = nullptr;
		}
		else
		{
			_capacity = _count;
			_children = new ASTNode*[_capacity];
			for(uint32_t i = 0; i < _count; i++)
			{
				_children[i] = new ASTNode(*in_node._children[i]);
			}
		}
	}

	ASTNode::ASTNode(NodeType::Type node_type, ReturnType::Type return_type)
		: _node_type(node_type)
		, _count(0)
		, _capacity(1)
		, _return_type(return_type)
        , _u({0})
        , _name(nullptr)
	{
		_children = new ASTNode*[_capacity];
		_children[0] = nullptr;
	}

	ASTNode::ASTNode(NodeType::Type node_type, ReturnType::Type return_type, symbol_id_t sid)
		: _node_type(node_type)
		, _count(0)
		, _capacity(1)
		, _return_type(return_type)
        , _u({0})
        , _name(nullptr)
	{
		_u.sid = sid;
		_children = new ASTNode*[_capacity];
		_children[0] = nullptr;
	}

	ASTNode::~ASTNode()
	{
		if(_node_type == NodeType::Literal)
		{
			free(_u.literal.data);
		}

		for(uint32_t i = 0; i < _count; i++)
		{
			delete _children[i];
		}
		delete[] _children;
	}

	void ASTNode::add_child(ASTNode* node)
	{
		_children[_count++] = node;
		// increase space if necessary
		if(_count == _capacity)
		{
			// double capacity
			_capacity *= 2;
			ASTNode** temp = new ASTNode*[_capacity];
			// copy over children to new buffer
			for(uint32_t i = 0; i < _count; i++)
			{
				temp[i] = _children[i];
			}
			// set the rest to null
			for(uint32_t i = _count; i < _capacity; i++)
			{
				temp[i] = nullptr;
			}
			// delete old buffer
			delete[] _children;
			// and set new
			_children = temp;
		}
	}

	void ASTNode::Print() const
	{
		Print(this, 0);
	}

	void ASTNode::PrintNode() const
	{
		printf("%s -> %s (%p)", NodeTypes[_node_type + 1], GetReturnType(_return_type), this);
		switch(_node_type)
		{
		case NodeType::Var:
		case NodeType::ConstVar:
		case NodeType::OutVar:
			printf(", symbol = 0x%x", static_cast<uint32_t>(_u.sid));
			if(_name != nullptr)
			{
				printf(", name = %s", _name);
			}
			break;
		case NodeType::Literal:
			switch(_return_type)
			{
			case ReturnType::Bool:
				printf(", val = %s", *(bool*)_u.literal.data ? "true" : "false");
				break;
			case ReturnType::Int:
				printf(", val = %i", *(int32_t*)_u.literal.data);
				break;
			case ReturnType::Float:
				printf(", val = %f", *(float*)_u.literal.data);
				break;
			default:
				COMPUTE_ASSERT(false);
				break;
			}
			break;
		default:
			break;
		}
	}

	void ASTNode::Print(const ASTNode* in_node, uint32_t indent)
	{
		for(uint32_t i = 0; i < indent; i++)
		{
			printf(" ");
		}

		in_node->PrintNode();

		printf("\n");

		for(uint32_t j = 0; j < in_node->_count; j++)
		{
			Print(in_node->_children[j], indent + 1);
		}
	}

	void ASTNode::PrintDot() const
	{
		printf("digraph AST\n{\n");
		printf(" node [fontsize=12, shape=box];\n");
		printf(" rankdir=LR;\n");

		uint32_t id = 0;

		PrintDot(this, id);

		printf("}\n");
	}

	void ASTNode::PrintDot(const ASTNode* in_node, uint32_t& id)
	{
		printf(" node%i [label=\"", id);
		in_node->PrintNode();
		printf("\"];\n");

		const uint32_t my_id = id;

		for(uint32_t j = 0; j < in_node->_count; j++)
		{
			++id;
			printf(" node%i -> node%i;\n", my_id, id);

			PrintDot(in_node->_children[j], id);
		}
	}

}
