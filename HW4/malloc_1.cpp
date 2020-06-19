#include <unistd.h>
#define MAX 100000000

void* smalloc(size_t size){
    if(size == 0 || size >MAX){
        return nullptr;
    }

    void* sbrk_res = sbrk(size);
    if(sbrk_res == (void*)-1){
        return nullptr;
    }

    return sbrk_res;
}
