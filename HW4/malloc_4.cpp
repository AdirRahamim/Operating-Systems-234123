#include <unistd.h>
#include <cstring>

#define MAX 100000000
#define MMAP_THRESHOLD 131072
#include <sys/mman.h>

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata dummy = {0, false, nullptr, nullptr};
MallocMetadata mmap_list_dummy = {0, false, nullptr, nullptr}; //mmap can be used or free, no unallocated blocks

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
    curr = mmap_list_dummy.next;
    while(curr != nullptr){
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
    curr = mmap_list_dummy.next;
    while(curr != nullptr){
        counter+=curr->size;
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

bool _check_split(size_t old_size, size_t new_size){
    return ((old_size - new_size) >= 128+_size_meta_data());
}

void* _split_block(MallocMetadata* block, size_t size){
    size_t new_block_size = block->size - size - _size_meta_data(); //New block size
    MallocMetadata new_block_meta = MallocMetadata{new_block_size, true, block->next, block};

    //Confugure the current block
    block->size = size;
    block->is_free = false;

    //Copy meta of new block to right place
    MallocMetadata* new_add = (MallocMetadata*)((size_t)block + size + _size_meta_data());
    memcpy(new_add, &new_block_meta, _size_meta_data());

    block->next = new_add;

    if(new_add->next != nullptr){
        new_add->next->prev = new_add;
    }
    return (void*)((size_t)block+_size_meta_data());
}

void* _mmap_alloc(size_t size){
    MallocMetadata* curr = &mmap_list_dummy;
    while(curr->next != nullptr){
        curr = curr->next;
    }
    //Reach end of list, allocate and add to list
    void* address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANONYMOUS, -1, 0);
    if(address == MAP_FAILED){
        return nullptr;
    }
    MallocMetadata meta = MallocMetadata{size, false, nullptr, curr};

    memcpy(address, &meta, _size_meta_data());
    curr->next = (MallocMetadata*)address;
    return (void*)((size_t)address + _size_meta_data());
}

void* smalloc(size_t size){
    if(size == 0 || size > MAX){
        return nullptr;
    }

    //Make sure first program break is aligned
    void* program_break = sbrk(0);
    int cur = (uintptr_t)program_break;
    if(cur % 8 != 0){
        //Need to align first address
        void* sbrk_res = sbrk(8 - cur%8);
        if(sbrk_res == (void*)-1){
            return nullptr;
        }
    }

    //align the size
    if((size + _size_meta_data()) % 8 != 0){
        size += (8 - ((size + _size_meta_data()) % 8));
    }

    if(size >= MMAP_THRESHOLD){
        return _mmap_alloc(size);
    }

    MallocMetadata* curr = &dummy;
    while(curr != nullptr){
        if(curr->size >= size && curr->is_free){
            //Found proper block!
            curr->is_free = false;
            if(_check_split(curr->size, size)){
                return _split_block(curr, size);
            }
            return (void*)((size_t)curr+_size_meta_data());
        }
        curr = curr->next;
    }
    //Need to allocate new one
    //Challenge 3 - check if last block is free
    curr = &dummy;
    while(curr->next != nullptr){
        curr = curr->next;
    }
    if(curr->is_free and curr->next = nullptr){
        //Last block is free - enlarge it
        void* res = sbrk(size - curr->size);
        if(res == (void*)-1){
            return nullptr;
        }
        curr->size = size;
        curr->is_free = false;
        return (void*)((size_t)curr + _size_meta_data());
    }
    return _allocate_new_block(size);

}

void* scalloc(size_t num, size_t size){
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

void _free_mmap(MallocMetadata* meta){
    MallocMetadata* next = meta->next;
    MallocMetadata* prev = meta->prev;
    if(munap(meta, meta->size+_size_meta_data()) == -1){
        return;
    }
    //munap succeed
    prev->next = next;
    if(next != nullptr){
        next->prev = prev;
    }
}

void _combine_blocks(MallocMetadata* meta){
    //check next block
    if(meta->next != nullptr and meta->next->is_free){
        meta->size += meta->next->size + _size_meta_data();
        meta->next = meta->next->next;
        if(meta->next->next != nullptr){
            meta->next->next->prev = meta;
        }
    }
    //try to combine with prev
    if(meta->prev!= nullptr and meta->prev->is_free){
        meta->prev->size += meta->size + _size_meta_data();
        meta->prev->next = meta->next;
        if(meta->next!= nullptr){
            meta->next->prev = meta->prev;
        }
    }
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
    //Check if mmap
    if(meta->size >= MMAP_THRESHOLD){
        _free_mmap(meta);
        return;
    }
    //Need to check prev and next block
    meta->is_free = true;
    _combine_blocks(meta);
}

void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > MAX){
        return nullptr;
    }
    if(oldp == nullptr){
        return smalloc(size);
    }

    //align the size
    if((size + _size_meta_data()) % 8 != 0){
        size += (8 - ((size + _size_meta_data()) % 8));
    }


    MallocMetadata* meta = (MallocMetadata*)((size_t)oldp - _size_meta_data());
    if(meta->size >= MMAP_THRESHOLD){
        void* mmap_res = _mmap_alloc(size);
        if(mmap_res == nullptr){
            return nullptr;
        }
        size_t min_size = size > meta->size ? size : meta->size;
        memcpy(mmap_res, (void*)((size_t)meta + _size_meta_data()), min_size);
        _free_mmap(meta);
        return mmap_res;
    }

    //priority a - try to reuse same block
    if(meta->size >= size){
        if(_check_split(meta->size, size)){
            return _split_block(meta, size);
        }
        return oldp;
    }

    //priority b - try to merge lower address
    if(meta->prev->is_free){
        if(meta->size + meta->prev->size >= size){
            meta->prev->is_free = false;
            meta->prev->next = meta->next;
            if(meta->next != nullptr){
                meta->next->prev = meta->prev;
            }
            meta->prev->size += meta->size + _size_meta_data();
            memcpy((void*)((size_t)meta->prev + _size_meta_data()), (void*)((size_t)meta+_size_meta_data()), meta->size);

            if(_check_split(meta->prev->size, size)){
                _split_block(meta->prev, size);
            }
            return meta->prev;
        }
    }

    //Option c - try to merge with next one
    if(meta->next != nullptr){
        if(meta->next->is_free and meta->size + meta->next->size >= size){
            meta->is_free = false;
            meta->next = meta->next->next;
            if(meta->next->next != nullptr){
                meta->next->next = meta;
            }
            meta->size += meta->next->size + _size_meta_data();

            if(_check_split(meta->size, size)){
                _split_block(meta, size);
            }
            return meta;
        }
    }

    //option c - try to merge two neighbors
    if(meta->prev!= nullptr and meta->next!= nullptr){
        if(meta->prev->is_free and meta->next->is_free and meta->size+meta->prev->size+meta->next->size >= size){
            meta->prev->is_free = false;
            meta->prev->next = meta->next->next;
            if(meta->next->next != nullptr){
                meta->next->next->prev = meta->prev;
            }
            meta->prev->size += meta->size + meta->next->size + 2*_size_meta_data();

            memcpy((void*)((size_t)meta->prev + _size_meta_data()), (void*)((size_t)meta+_size_meta_data()), meta->size);
            if(_check_split(meta->prev->size, size)){
                _split_block(meta->prev, size);
            }
            return meta->prev;
        }

        //option d - try to find different block or allocate - smalloc will take care of it
        void* smalloc_res = smalloc(size);
        if(smalloc_res == nullptr){
            return nullptr;
        }
        memcpy(smalloc_res, oldp, meta->size);
        sfree(oldp);
        return smalloc_res;
    }
}