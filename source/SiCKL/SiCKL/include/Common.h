#pragma once

#include <stdint.h>

#define count_of(X) (sizeof(X) / sizeof(X[0]))

namespace SiCKL
{

	// not threadsafe!
	// structs extending RefCounted must implement:
	// void Delete() - destructor
	template<typename T>
	class RefCounted
	{
	public:
		RefCounted()
		{
			_counter = new int32_t;
			*_counter = 1;
		}

		RefCounted(const RefCounted& right)
		{
			this->Assign(right);
		}

		~RefCounted()
		{
			this->Cleanup();
		}

		RefCounted& operator=(const RefCounted& right)
		{
			COMPUTE_ASSERT(this != &right);

			this->Cleanup();
			this->Assign(right);
			return *this;
		}
	protected:
		// returns memory to safe state to memcpy new data to it
		void Cleanup()
		{
            (*_counter)--;
			COMPUTE_ASSERT(*_counter >= 0);
			T* ptr = static_cast<T*>(this);
			COMPUTE_ASSERT(ptr != nullptr);
			if(*_counter == 0)
			{
				// deletes memory
				ptr->Delete();
			//	delete _counter;
			}
		}

		void Assign(const RefCounted& right)
		{
			(*right._counter)++;
			memcpy(this, &right, sizeof(T));
		}
    private:
		mutable int32_t* _counter;
	};
}

// fills out assignment and cleanup method declerations
#define REF_COUNTED(X) \
public: \
	X& operator=(const X& right) \
	{ \
		if(this != &right) \
		{ \
			SiCKL::RefCounted<X>::operator=(right); \
		} \
		return *this; \
	} \
private: \
	void Delete(); \
	friend class SiCKL::RefCounted<X>; \
public:

// provides a typesafe typedef wrapper around a primitive type
#define SAFE_TYPEDEF(NAME, TYPE) \
struct NAME \
{ \
    NAME() \
    : _val(TYPE()) \
    { } \
    NAME(const TYPE in_val) \
    : _val(in_val) \
    { } \
    operator const TYPE&() const \
    { return _val; } \
    operator TYPE&() \
    { return _val; } \
private: \
    TYPE _val; \
}
