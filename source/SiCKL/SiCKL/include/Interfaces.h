#define BEGIN_SOURCE void Parse() { initialize();
#define END_SOURCE finalize(); }

#define BEGIN_OUT_DATA _StartBlock<SiCKL::NodeType::OutData>();
#define OUT_DATA(TYPE, NAME) Out< TYPE > NAME( #NAME );
#define END_OUT_DATA _EndBlock();

#define BEGIN_CONST_DATA _StartBlock<SiCKL::NodeType::ConstData>();
#define CONST_DATA(TYPE, NAME) const Const< TYPE > NAME( #NAME );
#define END_CONST_DATA _EndBlock();

#define BEGIN_MAIN _StartBlock<SiCKL::NodeType::Main>();
#define END_MAIN _EndBlock();

/// Forward declare



template<class BASE>
struct Temp : BASE
{
	Temp(ASTNode* in_node)
		: BASE(temp_symbol, in_node, nullptr) 
	{}
};

// Out template is used for data locations written to by a
// shader program
template<class BASE>
struct Out : public BASE
{
	using BASE::operator=;

	Out(const char* in_name) : BASE(Source::next_symbol(), nullptr, in_name)
	{
		COMPUTE_ASSERT(Source::_current_block->_node_type == NodeType::OutData);

        ASTNode* out = new ASTNode(NodeType::OutVar, get_return_type<BASE>(), this->_id);
		out->_name = in_name;
		Source::_current_block->add_child(out);
	}
};

// Const template is used in parameter list in programs, adds self to the AST
// as a declaration inside a ConstBuffer table
template<typename BASE>
struct Const : public BASE
{
	Const(const char* in_name) : BASE(Source::next_symbol(), nullptr, in_name)
	{
		COMPUTE_ASSERT(Source::_current_block->_node_type == NodeType::ConstData);

        ASTNode* decl = new ASTNode(NodeType::ConstVar, get_return_type<BASE>(), this->_id);
		decl->_name = in_name;
		Source::_current_block->add_child(decl);
	}
};

template<typename BASE, typename PARENT>
struct Member : public BASE
{
	using BASE::operator=;
	Member(uint32_t member_offset, member_id_t mid) : BASE(temp_symbol, nullptr, nullptr)
	{
		// accesses to a Member type 
        this->_id = member_symbol;

		// get our owner
		PARENT* parent = (PARENT*)(((uint8_t*)this) - member_offset);

		// in the case where our parent did not get anything assigned to it, give it a valid symbol
		if(parent->_id == invalid_symbol)
		{
			parent->_id = Source::next_symbol();
		}

		// construct our ASTnode that represents us
		ASTNode* member = new ASTNode(NodeType::Member, get_return_type<BASE>());
		binary_operator(member, *parent, mid);

        this->_node = member;
	}
};
