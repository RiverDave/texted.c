#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct heap{

   void* buff;
   int next_used;
   size_t size; //in bytes
};

struct heap H;

void* mem_alloc(size_t size){

    if(H.size < size){
        return NULL;
    }

    for(int i = 0 ; i < H.size ; i++){
        if(&H.buff[i] == NULL)
            return &H.buff[i];
    }

    return NULL;
}

void mem_free(void* ptr){
    assert(H.size > 0);
}

int main(void){


    void* mem = malloc(sizeof(char) * 10);
    free(mem);



    return 0;
}
