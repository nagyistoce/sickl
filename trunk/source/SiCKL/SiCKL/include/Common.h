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
				delete _counter;
			}
		}

		void Assign(const RefCounted& right)
		{
			(*right._counter)++;
			memcpy(this, &right, sizeof(T));
		}

		mutable int32_t* _counter;
	};
}

#define REF_COUNTED(X) \
public: \
	X& operator=(const X& right) \
	{ \
		if(this != &right) \
		{ \
			RefCounted<X>::operator=(right); \
		} \
		return *this; \
	} \
private: \
	void Delete(); \
	friend class RefCounted<X>; \
public:
