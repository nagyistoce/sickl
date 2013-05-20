#include "SiCKL.h"

#undef If
#undef ElseIf
#undef Else
#undef EndIf
#undef While
#undef ForInRange

#include <vector>
#include <sstream>
namespace SiCKL
{
	ASTNode* Source::_root = nullptr;
	ASTNode* Source::_current_block = nullptr;
	std::vector<ASTNode*> Source::_block_stack;
	symbol_id_t Source::_next_symbol = 0;

	Source::Source()
		: _my_ast(nullptr)
		, _symbol_count(-1)
	{
		
	}

	Source::~Source()
	{
		if(_my_ast != nullptr)
		{
			delete _my_ast;
		}

		_symbol_count = -1;
	}

	void Source::initialize()
	{
		// reset
		if(_my_ast != nullptr)
		{
			// get rid of the old one
			delete _my_ast;
		}

		_my_ast = nullptr;
		_root = nullptr;
		_current_block = nullptr;
		_block_stack.clear();
		_next_symbol = 0;

		_symbol_count = -1;

		// fill in static data
		_my_ast = new ASTNode(NodeType::Program, ReturnType::Void);
		_root = _my_ast;
		_current_block = _root;
		_block_stack.push_back(_root);
	}

	void Source::finalize()
	{
		_symbol_count = _next_symbol;

		// copy over the node ptr
		_my_ast = _root;
		// reset the static guys
		_root = nullptr;
		_current_block = nullptr;
		_block_stack.clear();
		_next_symbol = 0;
	}

	/// Control Flow Functions

	void Source::_If(const Bool& b)
	{
		ASTNode* if_block = new ASTNode(NodeType::If, ReturnType::Void);
		ASTNode* condition = create_value_node(b);

		if_block->add_child(condition);
		_current_block->add_child(if_block);

		_block_stack.push_back(if_block);
		_current_block = if_block;
	}

	void Source::_ElseIf(const Bool& b)
	{	
		_block_stack.pop_back();
		_current_block = _block_stack.back();

		ASTNode* elseif_block = new ASTNode(NodeType::ElseIf, ReturnType::Void);
		ASTNode* condition = create_value_node(b);

		elseif_block->add_child(condition);
		_current_block->add_child(elseif_block);

		_block_stack.push_back(elseif_block);
		_current_block = elseif_block;
	}

	void Source::_Else()
	{
		_block_stack.pop_back();
		_current_block = _block_stack.back();

		ASTNode* else_block = new ASTNode(NodeType::Else, ReturnType::Void);


		_current_block->add_child(else_block);

		_block_stack.push_back(else_block);
		_current_block = else_block;
	}

	void Source::_While(const Bool& b)
	{
		ASTNode* while_block = new ASTNode(NodeType::While, ReturnType::Void);
		ASTNode* condition = create_value_node(b);

		while_block->add_child(condition);
		_current_block->add_child(while_block);

		_block_stack.push_back(while_block);
		_current_block = while_block;
	}
	
	void Source::_ForInRange( const Int& it, int32_t from, int32_t to )
	{
		COMPUTE_ASSERT(it._id == invalid_symbol);
		// give this guy a symbol id
		it._id = next_symbol();

		ASTNode* forinrange_block = new ASTNode(NodeType::ForInRange, ReturnType::Void);
		
		forinrange_block->add_child(create_value_node(it));
		forinrange_block->add_child(create_literal_node(from));
		forinrange_block->add_child(create_literal_node(to));
	
		_current_block->add_child(forinrange_block);

		_block_stack.push_back(forinrange_block);
		_current_block = forinrange_block;
	}
	


}

