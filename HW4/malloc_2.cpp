#include <unistd.h>

#define MAX 100000000


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata dummy = {0, false, nullptr, nullptr};

size_t _num_free_blocks(){

}

size_t _num_free_bytes(){

}

size_t _num_allocated_blocks(){

}

size_t _num_allocated_bytes(){

}

size_t _num_meta_data_bytes(){

}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

//Helper functions
void* _allocate_new_block(size_t size){
    if(size == 0 || size > MAX){
        return NULL;
    }

    //Allocate new space on heap
    void* sbrk_res = sbrk(size);
    if(sbrk_res == (void*)-1){
        return NULL;
    }
    //Find the place to inset
    MallocMetadata* curr = &dummy;
    
}


void* smalloc(size_t size){
    if(size == 0 || size > MAX){
        return NULL;
    }

    MallocMetadata* curr = &dummy;
    while(curr != nullptr){
        if(curr->size >= size && curr->is_free){
            //Found proper block!
            curr->is_free = false;
            return (void*)((size_t)curr+_size_meta_data())
        }
        cuur = curr->next;
    }
    //Need to allocate new one


}

void*scalloc(size_t num, size_t size){

}

void sfree(void* p){

}

void* srealloc(void* oldp, size_t size){

}

