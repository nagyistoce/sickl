// C
#include <string.h>
#include <stdint.h>

// local
#include "SiCKL.h"

namespace SiCKL
{
    namespace Internal
    {
        template<uint32_t SIZE>
        struct CharStack
        {
            CharStack()
                : _count(0)
            { }

            void push(char c)
            {
                SICKL_ASSERT(_count < SIZE);
                
                _buffer[_count++] = c;
            }

            char pop()
            {
                SICKL_ASSERT(_count > 0);
                
                return _buffer[--_count];
            }

            uint32_t count() const
            {
                return _count;
            }
    
        private:
            char _buffer[SIZE];
            uint32_t _count;
        };
        
        const char newline = '\n';
        struct StringBuffer
        {
            StringBuffer()
            {
                const size_t count = 64;
                _buffer = new char[count];
                COMPUTE_ASSERT(_buffer != nullptr);
                _allocated = count;
                ::memset(_buffer, 0x00, count);
                _length = 0;
                _tail = _buffer;
            }
            
            ~StringBuffer()
            {
                delete[] _buffer;
                _buffer = nullptr;
                _tail = nullptr;
                _length = 0;
                _allocated = 0;
            }
            
            StringBuffer& operator<<(const char* in_string)
            {
                while(*in_string)
                {
                    if(_length == (_allocated - 1))
                    {
                        GrowBuffer();
                    }
                    *_tail = *in_string;
                    ++_tail;
                    ++_length;
                    ++in_string;
                }
                return *this;
            }
            
            StringBuffer& operator<<(const char in_char)
            {
                if(_length == (_allocated - 1))
                {
                    GrowBuffer();
                }
                *_tail = in_char;
                ++_tail;
                ++_length;
                return *this;
            }
            
            // prints the name of a symbol id
            StringBuffer& operator<<(const symbol_id_t& id)
            {
                CharStack<8> stack;
                
                // convert from number to ASCII letter
                auto letter = [](int32_t i) -> char
                {
                    return char('a' + i);
                };  
                
                symbol_id_t x = id;
                
                stack.push(letter(x % 26));
                x = x - x % 26;
                
                while(x > 0)
                {
                    x = x / 26;
                    stack.push(letter((x - 1) % 26));
                    if((x % 26) == 0)
                    {
                        x = x - 26;
                    }
                    else
                    {
                        x -= x - x % 26;
                    }
                }
                
                while(stack.count())
                {
                    *this << stack.pop();
                }
                return *this;
            }
            
            StringBuffer& operator<<(const ReturnType_t type)
            {
                switch(type & ~(ReturnType::Buffer1D | ReturnType::Buffer2D))
                {
                case ReturnType::Bool:
                    *this << "bool";
                    break;
                case ReturnType::Int:
                    *this << "int";
                    break;
                case ReturnType::Int2:
                    *this << "int2";
                    break;
                case ReturnType::Int3:
                    *this << "int3";
                    break;
                case ReturnType::Int4:
                    *this << "int4";
                    break;
                case ReturnType::UInt:
                    *this << "uint";
                    break;
                case ReturnType::UInt2:
                    *this << "uint2";
                    break;
                case ReturnType::UInt3:
                    *this << "uint3";
                    break;
                case ReturnType::UInt4:
                    *this << "uint4";
                    break;
                case ReturnType::Float:
                    *this << "float";
                    break;
                case ReturnType::Float2:
                    *this << "float2";
                    break;
                case ReturnType::Float3:
                    *this << "float3";
                    break;
                case ReturnType::Float4:
                    *this << "float4";
                    break;
                default:
                    SICKL_ASSERT(false);
                }
                
                // if it's a buffer add pointer char
                if(type & (ReturnType::Buffer1D | ReturnType::Buffer2D))
                {
                    *this << "*";
                }
                               
                return *this;
            }
            
            operator const char*() const
            {
                return _buffer;
            }
            
        private:
            void GrowBuffer()
            {
                SICKL_ASSERT(_length == (_allocated - 1));
                
                const size_t old_count = _allocated;
                const size_t new_count = old_count * 2;
                char* new_buffer = new char[new_count];
                SICKL_ASSERT(new_buffer != nullptr);
                
                ::memcpy(new_buffer, _buffer, old_count);
                ::memset(new_buffer + old_count, 0x00, old_count);
                
                delete[] _buffer;
                _buffer = new_buffer;
                _allocated = new_count;
                _tail = new_buffer + old_count - 1;
            }
            
            // head of our string
            char* _buffer;
            // our null terminator
            char* _tail;
            size_t _length;
            size_t _allocated;
        };
        
        sickl_int print_code(StringBuffer& sb, const ASTNode* node, size_t indent)
        {
            switch(node->_node_type)
            {
            
            }
            
            return SICKL_SUCCESS;
        }
        
        sickl_int print_kernel_source(StringBuffer& out_buffer, const ASTNode& in_root)
        {
            // main, const data and out data            
            SICKL_ASSERT(in_root._count == 3);
    		const ASTNode* const_data = nullptr;
            const ASTNode* out_data = nullptr;
            const ASTNode* main = nullptr;
    
            // get the three important nodes here
            for(uint32_t i = 0; i < in_root._count; i++)
            {
                switch(in_root._children[i]->_node_type)
                {
                case NodeType::ConstData:
                    const_data = in_root._children[i];
                    break;
                case NodeType::OutData:
                    out_data = in_root._children[i];
                    break;
                case NodeType::Main:
                    main = in_root._children[i];
                    break;
                default:
                    SICKL_ASSERT(false);
                    break;
                }
            }
        
            // function decleration
            out_buffer << "__kernel void KernelMain(";
            
            for(size_t i = 0; i < const_data->_count; i++)
            {
                ASTNode* child = const_data->_children[i];
                ReturnType_t type = child->_return_type;
                symbol_id_t sid = child->_u.sid;
                
                out_buffer << newline;
                
                if(type & ReturnType::Buffer1D)
                {
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_length," << newline;
                }
                else if(type & ReturnType::Buffer2D)
                {
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_width" << newline;
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_height" << newline;
                }
                
                out_buffer << "                         ";
                out_buffer << "const ";
                if(type & (ReturnType::Buffer1D | ReturnType::Buffer2D))
                {
                    out_buffer << "__global ";
                }
                out_buffer << type << ' ' << sid << ',';
                
                
            }
            
            for(size_t i = 0; i < out_data->_count; i++)
            {
                ASTNode* child = out_data->_children[i];
                ReturnType_t type = child->_return_type;
                symbol_id_t sid = child->_u.sid;
                
                out_buffer << newline;
                
                if(type & ReturnType::Buffer1D)
                {
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_length," << newline;
                }
                else if(type & ReturnType::Buffer2D)
                {
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_width" << newline;
                    out_buffer << "                         ";
                    out_buffer << ReturnType::UInt << ' ' << sid << "_height" << newline;
                }
                
                out_buffer << "                         ";
                if(type & (ReturnType::Buffer1D | ReturnType::Buffer2D))
                {
                    out_buffer << "__global ";
                }
                out_buffer << type << ' ' << sid;
                if(i < (out_data->_count - 1))
                {
                    out_buffer << ',';
                }
            }
            out_buffer << ')' << newline;
            out_buffer << '{' << newline;
            
            
            out_buffer << '}' << newline;
            
            
        
            return SICKL_SUCCESS;
        }
    }

    

    sickl_int OpenCLCompiler::Build(Source& in_source, OpenCLProgram& out_program)
    {
        in_source.Parse();
        
        Internal::StringBuffer sb;
        ReturnIfError(Internal::print_kernel_source(sb, in_source.GetRoot()));
       
        printf("%s\n", (const char*)sb);
        fflush(stdout);
        
        return SICKL_SUCCESS;
    }
}

