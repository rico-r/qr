
#ifndef __QR_H
#define __QR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CorrectionLevel{
	L=1,
	M=0,
	Q=3,
	H=2
} CorrectionLevel;

typedef enum CharacterMode{
	MODE_NUMERIC = 1,
	MODE_ALPHANUMERIC = 2,
	MODE_BYTE = 4,
	MODE_KANJI = 8
} CharacterMode;

typedef struct{
	int size;
	char* data;
	char* ecc;
} QRBlock;

typedef struct{
	int ECcodeword_per_block;
	int block_in_group1;
	int codeword_per_block1;
	int block_in_group2;
	int codeword_per_block2;
} ECInfo;

extern int AntilogTable[256];
extern int LogTable[256];
extern ECInfo ECInfoTable[160];

int get_character_count_size(int version, CharacterMode mode);
const char* get_alignment_list(int version);

char** generate_qr(int* version_dst, char* data, int len, CharacterMode mode, CorrectionLevel clevel);

#ifdef __cplusplus
}
#endif

#endif /* __QR_H */