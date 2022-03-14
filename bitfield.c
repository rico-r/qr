#include "bitfield.h"

#include <malloc.h>

void BitField_init(BitField* f, int size_in_bytes){
	f->i=0;
	f->v=(char*) calloc(size_in_bytes, 1);
}

void BitField_put(BitField* f, int v, int size){
	unsigned int n1=~0;
	while(size>0){
		int rem=8-f->i%8;
		int min=rem<size?rem:size;
		int n=f->i/8;
		f->v[n]=f->v[n]<<min|(v>>(size-min)&(n1>>(32-min)));
		f->i+=min;
		size-=min;
	}
}

void BitField_destroy(BitField* f) {
	free(f->v);
}