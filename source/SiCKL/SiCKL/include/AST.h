#pragma once

#include "Enums.h"

namespace SiCKL
{
	// all the SiCKL::Source::TYPES extend Data
	struct Data
	{
		Data();
		Data(symbol_id_t in_id, struct ASTNode* in_node, const char* in_name);
		~Data();
		mutable symbol_id_t _id;
		struct ASTNode* _node;
		const char* _name;
	};

	struct ASTNode
	{
		ASTNode();
		ASTNode(const ASTNode&);
		ASTNode(NodeType::Type, ReturnType::Type);
		ASTNode(NodeType::Type, ReturnType::Type, symbol_id_t);
		template<typename T>
		ASTNode(NodeType::Type node_type, ReturnType::Type return_type, const T* data_pointer)
			: _node_type(node_type)
			, _count(0)
			, _capacity(1)
			, _return_type(return_type)
			, _u({0})
			, _name(nullptr)
   	{
			COMPUTE_ASSERT(node_type == NodeType::Literal);
			// copy in raw data
			_u.literal.data = malloc(sizeof(T));
			_u.literal.size = sizeof(T);
			memcpy(_u.literal.data, data_pointer, _u.literal.size);


			_children = new ASTNode*[_capacity];
			_children[0] = nullptr;
		}
		~ASTNode();
		void add_child(ASTNode*);

		NodeType::Type _node_type;
		uint32_t _count;
		uint32_t _capacity;
		ASTNode** _children;

		ReturnType::Type _return_type;

		union
		{
			symbol_id_t sid;
			struct
			{
				void* data;
				size_t size;
			} literal;
			struct
			{
				symbol_id_t parent_sid;
				member_id_t mid;
			} member;
		} _u;
		const char* _name;

		void Print() const;
		void PrintNode() const;
		void PrintDot() const;
	private:
		static void Print(const ASTNode*, uint32_t indent);
		static void PrintDot(const ASTNode*, uint32_t& id);
	};

	template<typename T> 
	inline static bool is_primitive()
	{
		return false;
	}
	#define IS_PRIMITIVE(A) template<> inline bool is_primitive< A >() { return true; }
	IS_PRIMITIVE(bool)
	IS_PRIMITIVE(uint8_t)
	IS_PRIMITIVE(int8_t)
	IS_PRIMITIVE(uint16_t)
	IS_PRIMITIVE(int16_t)
	IS_PRIMITIVE(uint32_t)
	IS_PRIMITIVE(int32_t)
	IS_PRIMITIVE(uint64_t)
	IS_PRIMITIVE(int64_t)
	IS_PRIMITIVE(float)
	IS_PRIMITIVE(double)

	template<typename T> 
	inline static ReturnType::Type get_return_type()
	{
		return T::_return_type;
	}

	#define PRIMITIVE_TO_RETURNTYPE(P, RT) template<> inline ReturnType::Type get_return_type< P >() {return (RT);}
	PRIMITIVE_TO_RETURNTYPE(bool, ReturnType::Bool)
	PRIMITIVE_TO_RETURNTYPE(int32_t, ReturnType::Int)
	PRIMITIVE_TO_RETURNTYPE(uint32_t, ReturnType::UInt)
	PRIMITIVE_TO_RETURNTYPE(float, ReturnType::Float)

	/// Data Node Creation

	template<typename TYPE>
	static ASTNode* create_literal_node(const TYPE& val)
	{
		COMPUTE_ASSERT(is_primitive<TYPE>() == true);
		return new ASTNode(NodeType::Literal, get_return_type<TYPE>(), &val);
	}

	template<typename TYPE>
	static ASTNode* create_data_node(const TYPE& val)
	{
		COMPUTE_ASSERT(is_primitive<TYPE>() == false);
		Data& data = *(Data*)&val;
		if(data._id >= 0)
		{
			// a symbol
			return new ASTNode(NodeType::Var, get_return_type<TYPE>(), data._id);
		}
		else
		{
			// make sure the temp data is initialized
			COMPUTE_ASSERT(data._node != nullptr);
			return new ASTNode(*data._node);
		}
		return nullptr;
	}

	template<typename TYPE>
	static ASTNode* create_value_node(const TYPE& val)
	{
		ASTNode* result = nullptr;
		if(is_primitive<TYPE>())
		{
			//make a literal node
			result = create_literal_node<TYPE>(val);
		}
		else
		{
			// if it's not a primitive, it must be a Data
			result = create_data_node<TYPE>(val);

		}
		return result;
	}

	/// Unary and Binary operator methods

	template<typename BASE>
	static void unary_operator(struct ASTNode* root, const BASE& in_base)
	{
		COMPUTE_ASSERT(is_primitive<BASE>() == false);

		ASTNode* child = create_data_node(in_base);
		root->add_child(child);
	}

	template<typename BASE, typename FRIEND>
	static void binary_operator(ASTNode* root, const BASE& in_base, const FRIEND& in_friend)
	{
		ASTNode* left = create_value_node(in_base);
		ASTNode* right = create_value_node(in_friend);

		root->add_child(left);
		root->add_child(right);
	}
}
