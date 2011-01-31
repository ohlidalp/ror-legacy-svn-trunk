#include "memoryWrapper.h"
#include "nedmalloc.h"

unsigned long MemoryWrapper::memory_used = 0;

std::map<void *, unsigned long> MemoryWrapper::allocations;

void *MemoryWrapper::allocBytes(size_t count)
{
	void* ptr = nedalloc::nedmalloc(count);

	memory_used += count;
	allocations[ptr] = count;

	return ptr;
}

void *MemoryWrapper::callocBytes(size_t no, size_t size)
{
	void* ptr = nedalloc::nedcalloc(no, size);

	memory_used += size;
	allocations[ptr] = size;

	return ptr;
}

void MemoryWrapper::deallocBytes(void* ptr)
{
	if(!ptr)
		return;

	// get the memory size of this and substract it again
	if(allocations.find(ptr) != allocations.end())
	{
		memory_used -= allocations[ptr];
		allocations.erase(ptr);
	}

	nedalloc::nedfree(ptr);
}

void* MemoryWrapper::allocBytesAligned(size_t align, size_t count)
{
	const size_t SIMD_ALIGNMENT = 16;
	void* ptr = align ? nedalloc::nedmemalign(align, count) : nedalloc::nedmemalign(SIMD_ALIGNMENT, count);

	memory_used += count;
	allocations[ptr] = count;
	
	return ptr;
}

void MemoryWrapper::deallocBytesAligned(size_t align, void* ptr)
{
	if (!ptr)
		return;

	// get the memory size of this and substract it again
	if(allocations.find(ptr) != allocations.end())
	{
		memory_used -= allocations[ptr];
		allocations.erase(ptr);
	}

	nedalloc::nedfree(ptr);
}

unsigned long MemoryWrapper::getMemoryAllocated()
{
	return memory_used;
}
