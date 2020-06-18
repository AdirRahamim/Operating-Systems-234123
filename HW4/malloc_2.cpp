#include <unistd.h>
#include <cstring>

#define MAX 100000000


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata dummy = {0, false, nullptr, nullptr};

size_t _num_free_blocks(){
    size_t counter = 0;
    MallocMetadata* curr = (dummy.next);
    while (curr != nullptr){
        if(curr->is_free){
            counter++;
        }
        curr = curr->next;
    }
    return counter;
}

size_t _num_free_bytes(){
    size_t counter = 0;
    MallocMetadata* curr = (dummy.next);
    while (curr != nullptr){
        if(curr->is_free){
            counter += curr->size;
        }
        curr = curr->next;
    }
    return counter;
}

size_t _num_allocated_blocks(){
    size_t counter = 0;
    MallocMetadata* curr = (dummy.next);
    while (curr != nullptr){
        counter++;
        curr = curr->next;
    }
    return counter;
}

size_t _num_allocated_bytes(){
    size_t counter = 0;
    MallocMetadata* curr = (dummy.next);
    while (curr != nullptr){
        counter += curr->size;
        curr = curr->next;
    }
    return counter;
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
}

//Helper functions
void* _allocate_new_block(size_t size){
    if(size == 0 || size > MAX){
        return nullptr;
    }

    //Allocate new space on heap
    void* sbrk_res = sbrk(size + _size_meta_data());
    if(sbrk_res == (void*)-1){
        return nullptr;
    }
    //Find the place to insert
    MallocMetadata* curr = &dummy;
    while(curr != nullptr){
        if(curr->next == nullptr){
            break;
        }
        curr = curr->next;
    }

    MallocMetadata new_meta = MallocMetadata{size, false, nullptr, curr};
    std::memcpy(sbrk_res, &new_meta, _size_meta_data());
    curr->next = (MallocMetadata*)sbrk_res;

    return (void*)((size_t)sbrk_res + _size_meta_data());
}


void* smalloc(size_t size){
    if(size == 0 || size > MAX){
        return nullptr;
    }

    MallocMetadata* curr = &dummy;
    while(curr != nullptr){
        if(curr->size >= size && curr->is_free){
            //Found proper block!
            curr->is_free = false;
            return (void*)((size_t)curr+_size_meta_data());
        }
        curr = curr->next;
    }
    //Need to allocate new one
    return _allocate_new_block(size);

}

void*scalloc(size_t num, size_t size){
    if(num == 0){
        return nullptr;
    }
    void* smalloc_res = smalloc(num*size);
    if(smalloc_res == nullptr){
        return nullptr;
    }

    std::memset(smalloc_res, 0, size*num);
    return smalloc_res;
}

void sfree(void* p){
    if(p==nullptr){
        return;
    }
    //Get pointer to Meta data
    MallocMetadata* meta = (MallocMetadata*)((size_t)p - _size_meta_data());
    if(meta->is_free){
        return;
    }
    meta->is_free = true;
}

void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > MAX){
        return nullptr;
    }
    if(oldp == nullptr){
        return smalloc(size);
    }

    MallocMetadata* meta = (MallocMetadata*)((size_t)oldp - _size_meta_data());
    if(meta->size >= size){
        return oldp;
    }

    //Current block size is smaller, find new one and realse
    void* smalloc_res = smalloc(size);
    if(smalloc_res == nullptr){
        return nullptr;
    }

    std::memcpy(smalloc_res, oldp, meta->size);
    sfree(oldp);
    return smalloc_res;
}