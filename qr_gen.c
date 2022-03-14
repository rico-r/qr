
#include "qr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitfield.h"

void put_detector(char** module, int top, int left){
	char pattern[3][7]={{1,1,1,1,1,1,1},{1,0,0,0,0,0,1},{1,0,1,1,1,0,1}};
	memcpy(&module[top  ][left], pattern[0], 7);
	memcpy(&module[top+1][left], pattern[1], 7);
	memcpy(&module[top+2][left], pattern[2], 7);
	memcpy(&module[top+3][left], pattern[2], 7);
	memcpy(&module[top+4][left], pattern[2], 7);
	memcpy(&module[top+5][left], pattern[1], 7);
	memcpy(&module[top+6][left], pattern[0], 7);
}

void put_timing(char** module, int size){
	size-=8;
	for(int i=6; i<size; i+=2){
		module[6][i]=1;
		module[i][6]=1;
	}
}

void put_alignment(char** module, int version){
	const char* list=get_alignment_list(version);
	char pattern[3][5]={{1,1,1,1,1},{1,0,0,0,1},{1,0,1,0,1}};
	for(int i=1; i<=list[0]; i++){
		for(int j=1; j<=list[0]; j++){
			if(i==1){
				if(j==1||j==list[0])continue;
			}else if(i==list[0]&&j==1) continue;
			memcpy(&module[list[i]-2][list[j]-2], pattern[0], 5);
			memcpy(&module[list[i]-1][list[j]-2], pattern[1], 5);
			memcpy(&module[list[i]  ][list[j]-2], pattern[2], 5);
			memcpy(&module[list[i]+1][list[j]-2], pattern[1], 5);
			memcpy(&module[list[i]+2][list[j]-2], pattern[0], 5);
		}
	}
}

char** gen_pattern_mask(int version){
	int size=17+4*version;
	char** mask=(char**)malloc(size*sizeof(int*));
	for(int y=0; y<size; y++){
		mask[y]=(char*)malloc(size);
		memset(mask[y], 1, size);
	}
	{
		int arr[3][2]={{0,0},{0,size-8},{size-8,0}};
		for(int k=0;k<3;k++){
			int y=arr[k][0], x=arr[k][1];
			for(int i=0;i<(y==0?9:8);i++){
				memset(&mask[y+i][x], 0, x==0?9:8);
			}
			
		}
	}
	for(int i=8; i<size-8; i++){
		mask[6][i]=0;
		mask[i][6]=0;
	}
	{
		const char* list=get_alignment_list(version);
		for(int i=1; i<=list[0]; i++){
			for(int j=1; j<=list[0]; j++){
				if(i==1){
					if(j==1||j==list[0])continue;
				}else if(i==list[0]&&j==1) continue;
				for(int k=-2;k<3;k++)
					memset(&mask[list[i]-k][list[j]-2], 0, 5);
			}
		}
	}
	if(version>6){
		int n=size-11;
		for(int i=0;i<18;i++){
			mask[i/3][n+i%3]=0;
			mask[n+i/6][i%6]=0;
		}
	}
	return mask;
}

void put_data(char**module, char*data, int len, char**mask, int version){
	int size=16+4*version;
	int x=size;
	int y=size;
	int dir=-1;
	len*=8;
	int j=0;
	for(int i=0; i<len; i++){
		if(mask[y][x-j%2]==0){
			i--;
			goto next;
		}
		module[y][x-j%2]=1&(data[i/8]>>(7-i%8));
		next:
		if(j%2){
			y+=dir;
			if(y>size){
				y=size;
				x-=2;
				dir=-1;
			}
			if(y<0){
				y=0;
				x-=2;
				dir=1;
			}
			if(x==6)x--;
		}
		j++;
	}
	return;
}

void apply_mask(char** module, char** pattern_mask, int pattern, int size){
	#define MASK(func) \
		for(y=0; y<size; y++)\
			for(x=0; x<size; x++)\
				if(pattern_mask[y][x]) module[y][x]^=func

	int y, x;
	switch(pattern){
		case 0:MASK((x+y)%2==0); break;
		case 1:MASK(y%2==0); break;
		case 2:MASK(x%3==0); break;
		case 3:MASK((x+y)%3==0); break;
		case 4:MASK((y/2+x/3)%2==0); break;
		case 5:MASK((x*y)%2+(x*y)%3==0); break;
		case 6:MASK(((x*y)%2+(x*y)%3)%2==0); break;
		case 7:MASK(((x+y)%2+(x*y)%3)%2==0); break;
		default:
			fprintf(stderr, "invaild mask pattern %d\n", pattern);
			exit(-9);
	}
	#undef MASK
}

// most significant bit
int msb(int v){
	int i=31;
	while(!(v>>i))i--;
	return i;
}

//golay code
int encode(int v, int gen, int m){
	int e = 2 << m-1;
	v <<= m;
	while(v>e){
		v^=gen<<(msb(v)-m);
	}
	return v;
}

void put_format_info(char**module, int version, CorrectionLevel correction_level, int mask_pattern){
	const int INFO_MASK = 0b101010000010010;
	int size = 16+4*version;
	int fmt = correction_level<<3|mask_pattern;
	int v = INFO_MASK^(fmt<<10|encode(fmt, 0b10100110111, 10));

	module[size-7][8]=1;
	for(int i=0; i<15; i++){
		int bit=(v>>i)&1;
		//top
		if(i<8)module[8][size-i]=bit;
		else if(i<9)module[8][7]=bit;
		else module[8][14-i]=bit;
		//left
		if(i<6)module[i][8]=bit;
		else if(i<8)module[i+1][8]=bit;
		else module[size-14+i][8]=bit;
	}
}

void put_version_code(char** module, int version){
	int v=version<<12|encode(version, 0b1111100100101, 12);
	int n=6+4*version;
	for(int i=0;i<18;i++){
		int x=(v>>i)&1;
		module[i/3][n+i%3]=x;//top-right
		module[n+i%3][i/3]=x;//bottom-left
	}
}

void put_encoded_number(BitField* bit, char* data, int len){
	int v=0;
	for(int i=0; i<len; i++){
		v=v*10+data[i]-'0';
		if(i%3==2){
			BitField_put(bit, v, 10);
			v=0;
		}
	}
	if(len%3!=0)
		BitField_put(bit, v, 1+len%3*3);
}

void put_encoded_alpnum(BitField* bit, char* data, int len){
	int v=0;
	char table[]={36, 0, 0, 0, 37, 38, 0, 0, 0, 0, 39, 40, 0, 41, 42, 43, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 44, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
	for(int i=0; i<len; i++){
		v=v*45+table[data[i]-' '];
		if(i%2==1){
			BitField_put(bit, v, 11);
			v=0;
		}
	}
	if(len%2!=0)
		BitField_put(bit, v, 6);
}

void put_encoded_byte(BitField* bit, char* data, int len){
	for(int i=0; i<len; i++){
		BitField_put(bit, data[i], 8);
	}
}

void get_generator(char* dst, int x){
	char* a=malloc(x+1);
	a[0]=0; a[1]=0;
	for(int n=1; n<x; n++){
		dst[0]=(a[0]+n)%255;
		dst[n+1]=0;
		for(int i=1; i<=n; i++){
			dst[i]=LogTable[AntilogTable[(a[i]+n)%255]^AntilogTable[a[i-1]]];
		}
		memcpy(a, dst, n+1);
	}
	// alternate version of above code
	// a[0]=1; a[1]=1;
	// for(int n=1; n<x; n++){
	// 	dst[0]=AntilogTable[(LogTable[a[0]]+n)%255];
	// 	dst[n+1]=1;
	// 	for(int i=1; i<=n; i++){
	// 		dst[i]=a[i-1]^AntilogTable[(LogTable[a[i]]+n)%255];
	// 	}
	// 	memcpy(a, dst, n+2);
	// }
	free(a);
}

// bch code
void encode2(char* dst, const char* msg, const char* gen, int a, int b){
	char* rem=calloc(a+b, 1);
	memcpy(rem, msg, a);
	
	for(int n=a; n>0; n--){
		int u=LogTable[rem[a-n]];
		int j=a+b-n;
		for(int i=0; i<=b; i++){
			rem[j-i]^=AntilogTable[(u+gen[i])%255];
		}
	}
	memcpy(dst, &rem[a], b);
	free(rem);
}

int get_capacity_for(int size, CharacterMode mode){
	switch(mode){
		case MODE_NUMERIC: return size/10*3+(size%10-1)/3;
		case MODE_ALPHANUMERIC: return size/11*2+(size%11-1)/5;
		case MODE_BYTE: return size/8;
		case MODE_KANJI: return size/13;
		default:
		fprintf(stderr, "unsupported mode %d\n", mode);
		exit(54);
	}
}

int get_minimum_version_for(int* cap, int len, CharacterMode mode, CorrectionLevel clevel){
	int n;
	for(int v=1; v<=40; v++){
		ECInfo info=ECInfoTable[(v-1)*4+((clevel^3)+2)%4];
		int c=info.block_in_group1*info.codeword_per_block1
			+ info.block_in_group2*info.codeword_per_block2;
		n=get_capacity_for(c*8-4-get_character_count_size(v, mode), mode);
		if(n>=len){
			*cap=c;
			return v;
		}
	}
	fprintf(stderr, "data is too big, maximum is %d for this mode", n);
	exit(38);
}

// evaluate pattern score
int evaluate_pattern(char** module, int size){
	int score=0;
	int total=0;
	char row_last, col_last;
	int row_count, col_count;
	for(int i=0; i<size; i++){
		for(int j=0; j<size; j++){
			char x=module[i][j];
			total+=x;
			if(i>0&&j>0){
				if(x==module[i-1][j]
				&& x==module[i][j-1]
				&& x==module[i-1][j-1])
					score+=3;
			}
			if(j==0){
				row_count=1;
				row_last=module[i][0];
				col_count=1;
				col_last=module[0][i];
				continue;
			}
			if(module[i][j]==row_last){
				row_count++;
			}else{
				if(row_count>4) score+=row_count-2;
				row_count=1;
				row_last=module[i][j];
			}
			if(module[j][i]==col_last){
				col_count++;
			}else{
				if(col_count>4) score+=col_count-2;
				col_count=1;
				col_last=module[j][i];
			}
		}
		if(row_count>4) score+=row_count-2;
		if(col_count>4) score+=col_count-2;
	}
	total=total*100/(size*size);
	total=total<50?(50-total):(total-50);
	score+=total/5*10;
	return score;
}

void free_grid(char** v, int size){
	for(int i=0; i<size; i++){
		free(v[i]);
	}
	free(v);
}

char** generate_qr(int* version_dst, char* data, int len, CharacterMode mode, CorrectionLevel clevel){
	int capacity=0;
	int version=get_minimum_version_for(&capacity, len, mode, clevel);
	*version_dst=version;
	
	BitField bit;
	BitField_init(&bit, capacity);
	BitField_put(&bit, (int)mode, 4);
	BitField_put(&bit, len, get_character_count_size(version, mode));
	switch(mode){
		case MODE_NUMERIC: put_encoded_number(&bit, data, len); break;
		case MODE_ALPHANUMERIC: put_encoded_alpnum(&bit, data, len); break;
		case MODE_BYTE: put_encoded_byte(&bit, data, len); break;
		default:
		fprintf(stderr, "unsupported mode %d\n", mode);
		exit(-1);
	}
	BitField_put(&bit, 0, 4);
	
	if(bit.i%8)BitField_put(&bit, 0, 8-bit.i%8);
	int n=capacity-bit.i/8;
	for(int i=0; i<n; i++){
		BitField_put(&bit, i%2?17:236, 8);
	}

	ECInfo info=ECInfoTable[(version-1)*4+((clevel^3)+2)%4];
	QRBlock* blocks=(QRBlock*)malloc((info.block_in_group1+info.block_in_group2)*sizeof(QRBlock));
	char* gen=malloc(1+info.ECcodeword_per_block);
	get_generator(gen, info.ECcodeword_per_block);
	n=0;
	int m=0;
	for(int i=0; i<info.block_in_group1; i++){
		char* ecc=(char*)malloc(info.ECcodeword_per_block);
		encode2(ecc, &bit.v[n], gen, info.codeword_per_block1, info.ECcodeword_per_block);
		blocks[m].data=&bit.v[n];
		blocks[m].ecc=ecc;
		blocks[m].size=info.codeword_per_block1;
		m++;
		n+=info.codeword_per_block1;
	}
	for(int i=0; i<info.block_in_group2; i++){
		char* ecc=(char*)malloc(info.ECcodeword_per_block);
		encode2(ecc, &bit.v[n], gen, info.codeword_per_block2, info.ECcodeword_per_block);
		blocks[m].data=&bit.v[n];
		blocks[m].ecc=ecc;
		blocks[m].size=info.codeword_per_block2;
		m++;
		n+=info.codeword_per_block2;
	}
	//arrange the data
	char* result=malloc(capacity+m*info.ECcodeword_per_block);
	n=0;
	int msize=info.codeword_per_block1>info.codeword_per_block2?info.codeword_per_block1:info.codeword_per_block2;
	for(int i=0; i<msize; i++){
		for(int j=0; j<m; j++){
			if(i>=blocks[j].size)
				continue;
			result[n++]=blocks[j].data[i];
		}
	}
	for(int i=0; i<info.ECcodeword_per_block; i++){
		for(int j=0; j<m; j++){
			result[n++]=blocks[j].ecc[i];
		}
	}
	
	int size=17+4*version;
	char** module=(char**)malloc(size*sizeof(int*));
	for(int y=0; y<size; y++){
		module[y]=(char*)malloc(size);
		memset(module[y], 0, size);
	}
	char** mask=gen_pattern_mask(version);
	put_alignment(module, version);
	put_detector(module, 0,0);
	put_detector(module, size-7,0);
	put_detector(module, 0,size-7);
	put_timing(module, size);
	if(version>6){
		put_version_code(module, version);
	}
	
	put_data(module, result, n, mask, version);
	int mask_pattern=0;
	int minPenalty=0x7fffffff;
	for(int i=0; i<8; i++){
		put_format_info(module, version, clevel, i);
		apply_mask(module, mask, i, size);
		int penalty=evaluate_pattern(module, size);
		if(penalty<minPenalty){
			mask_pattern=i;
			minPenalty=penalty;
		}
		apply_mask(module, mask, i, size);
	}
	put_format_info(module, version, clevel, mask_pattern);
	apply_mask(module, mask, mask_pattern, size);
	
	// cleaning
	for(int j=0; j<m; j++){
		free(blocks[j].ecc);
	}
	free_grid(mask, size);
	free(gen);
	BitField_destroy(&bit);
	free(blocks);
	free(result);
	return module;
}