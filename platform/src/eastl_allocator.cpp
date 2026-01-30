#include <cstddef>
#include <cstdlib>
#include <snmalloc/snmalloc.h>

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return std::malloc(size);
    //return snmalloc::alloc(size);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return std::aligned_alloc(alignment, size);
    //return snmalloc::alloc_aligned(alignment, size)
}
