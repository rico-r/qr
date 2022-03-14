#ifndef __BITFIELD_H
#define __BITFIELD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	int i;
	char* v;
}BitField;

void BitField_init(BitField* f, int size_in_bytes);
void BitField_put(BitField* f, int value, int size);
void BitField_destroy(BitField*);

#ifdef __cplusplus
}
#endif

#endif /* __BITFIELD_H */