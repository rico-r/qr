
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //used for test if given path is directory
#include <sys/stat.h> //used for test if given path is readable
#include <string.h>

#include "qr.h"

#include <jpeglib.h>
#include <png.h>

#define QUITE_ZONE 4

void png_version(char *version){
	int i = png_access_version_number();
	sprintf(version, "%d.%d.%d", i / 10000, (i / 100) % 100, i % 100);
}

void write_png(char** data, int version, int zoom_factor, FILE* file){
	int size = 17+4*version;
	int width = (2*QUITE_ZONE+size)*zoom_factor;
	int start = QUITE_ZONE*zoom_factor,
		end = (QUITE_ZONE+size)*zoom_factor;
	char row[width];
	int y, x, i;
	
	char png_version_str[9];
	png_version(png_version_str);
	png_structp png = png_create_write_struct(png_version_str, 0, 0, 0);
	png_infop info = png_create_info_struct(png);
	png_set_IHDR(png,info, width, width, 8, PNG_COLOR_TYPE_GRAY, 0,0,0);
	png_init_io(png, file);
	png_write_info(png, info);
	
	memset(row, 255, width);
	for(i=0; i<QUITE_ZONE*zoom_factor; i++)
		png_write_row(png, (png_bytep)row);
	
	for(y=0; y<size; y++){
		for(x=start; x<end; x++){
			row[x]=255*(1-data[y][x/zoom_factor-QUITE_ZONE]);
		}
		for(i=0; i<zoom_factor; i++)
			png_write_row(png, (png_bytep)row);
	}
	
	memset(row, 255, width);
	for(i=0; i<QUITE_ZONE*zoom_factor; i++)
		png_write_row(png, (png_bytep)row);
	
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);
}

void write_jpeg(char** data, int version, int zoom_factor, FILE* file){
	int size = 17+4*version;
	int width = (2*QUITE_ZONE+size)*zoom_factor;
	int start = QUITE_ZONE*zoom_factor,
		end = (QUITE_ZONE+size)*zoom_factor;
	unsigned char *row = malloc(width * sizeof(unsigned char));
	int y, x, i;
	
	struct jpeg_compress_struct jpeg;
	struct jpeg_error_mgr jerr;
	jpeg.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&jpeg);
	jpeg_stdio_dest(&jpeg, file);
	jpeg.image_width = width;
	jpeg.image_height = width;
	jpeg.input_components = 1;
	jpeg.in_color_space = JCS_GRAYSCALE;
	jpeg_set_defaults(&jpeg);
	jpeg_set_quality(&jpeg, 80, TRUE);
	jpeg_start_compress(&jpeg, TRUE);
	
	memset(row, 255, width);
	for(i=0; i<QUITE_ZONE*zoom_factor; i++)
		jpeg_write_scanlines(&jpeg, &row, 1);
	
	for(y=0; y<size; y++){
		for(x=start; x<end; x++){
			row[x]=255*(1-data[y][x/zoom_factor-QUITE_ZONE]);
		}
		for(i=0; i<zoom_factor; i++)
			jpeg_write_scanlines(&jpeg, &row, 1);
	}
	
	memset(row, 255, width);
	for(i=0; i<QUITE_ZONE*zoom_factor; i++)
		jpeg_write_scanlines(&jpeg, &row, 1);
	
	jpeg_finish_compress(&jpeg);
	fclose(file);
	jpeg_destroy_compress(&jpeg);
}

void print_module(char** data, int version, FILE* file){
	int size=17+4*version;
	int x, y;
	const char *white = "\e[47m  ";
	const char *black = "\e[m  ";
	for(x=0; x<QUITE_ZONE; x++){
		for(y=-QUITE_ZONE; y<size+QUITE_ZONE; y++){
			fprintf(file, "%s", white);
		}
		putchar('\n');
	}
	for(y=0; y<size; y++){
		for(x=0; x<QUITE_ZONE; x++){
			fprintf(file, "%s", white);
		}
		for(x=0; x<size; x++){
			fprintf(file, "%s", data[y][x] == 1 ? black : data[y][x] == 2 ? "[]" : white);
		}
		for(x=0; x<QUITE_ZONE; x++){
			fprintf(file, "%s", white);
		}
		printf("\e[m\n");
	}
	for(x=0; x<QUITE_ZONE; x++){
		for(y=-QUITE_ZONE; y<size+QUITE_ZONE; y++){
			fprintf(file, "%s", white);
		}
		putchar('\n');
	}
	fprintf(file, "%s", "\e[m");
}

void append(char** dst, int* len1, char* a, int len2){
	*dst = realloc(*dst, (*len1)+len2);
	memcpy(&(*dst)[*len1], a, len2);
	*len1 += len2;
}

int checkFile(const char* path, int read){
	char* msg;
	if(!access(path, F_OK)){
		struct stat st;
		stat(path, &st);
		if(S_ISDIR(st.st_mode)){
			msg="is directory";
			goto err;
		}
		if(read){
			if(access(path, R_OK)){
				msg="file unreadable";
				goto err;
			}
		}else{
			if(access(path, W_OK)){
				msg="file unwritable";
				goto err;
			}
		}
	}else{
		if(read){
			msg="no such file or directory";
			goto err;
		}
	}
	return 0;
	
	err:
	fprintf(stderr, "\"%s\" %s\n", path, msg);
	return 1;
}

int read_file(char** dst, int* length, FILE* file){
	fseek(file, 0, SEEK_END);
	int len=ftell(file);
	fseek(file, 0, SEEK_SET);
	if(len==-1){
		while(!feof(file)){
			char one = fgetc(file);
			append(dst, length, &one, 1);
		}
	}else{
		char* r=(char*) malloc(len);
		int readed = fread(r, len, 1, file);
		if(readed != len) {
			// Error occour while reading file
		}
		append(dst, length, r, len);
		free(r);
	}
	return 0;
}

int read_path(char** dst, int* length, char* path){
	if(checkFile(path, 1)) return 1;
	FILE* f=fopen(path, "rb");
	read_file(dst, length, f);
	fclose(f);
	return 0;
}

int get_supported_mode(const char* data, int len){
	int mode=15;
	char mod[256]={4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 6, 6, 4, 4, 4, 4, 6, 6, 4, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
	for(int i=0; i<len; i++){
		mode &= mod[data[i]];
		if(mode==MODE_BYTE) break;
	}
	return mode;
}

#define USAGE \
"usage: %s [-d data] [-f src] [-o output] [-auto|-alp|-num|-byte]\n"\
"       [-l|-m|-q|-h] [-png|-jpeg|-print]\n"\
"read text from stdin and output QR code image to stdout.\n"\
"--help  print this help\n"\
"-d data use data instead of stdin\n"\
"-f src  read data from src instead of stdin,\n"\
"        if multiple data source is used\n"\
"        the data will be combined in order\n"\
"        they appear\n"\
"-o dst  output to dst instead of stdout,\n"\
"        if multiple occurred the last\n"\
"        one is used\n"\
"-png    output png file (default)\n"\
"-jpeg   output jpeg file\n"\
"-print  print generated QR into terminal (experimental)\n"\
"supported mode:\n"\
"   -auto  select automatically based on input (default).\n"\
"   -num   numeric mode (number 0-9).\n"\
"   -alp   alphanumeric mode (A-Z0-9 $%%/*+-:)\n"\
"   -byte  byte mode\n"\
"if multiple mode occourred the last one is used.\n"\
"if the data can not encoded with user\n"\
"selected mode, mode is selected automatically.\n"\
"supported correction level:\n"\
"   -l  ~7%% recovered\n"\
"   -m  ~15%% recovered (default)\n"\
"   -q  ~25%% recovered\n"\
"   -h  ~30%% recovered\n"

typedef enum {
	PNG,
	JPEG,
	PRINT
} OutputFormat;

int main(int argc, char** argv){
	char* data = malloc(0);
	int len = 0;
	int prefered_mode = 0;
	OutputFormat out_format = PNG;
	CorrectionLevel clevel = M;
	FILE* output = stdout;

	for(int i=1; i<argc; i++){
		char* v=argv[i];
		if(!strcmp(v, "-f")){
			if(read_path(&data, &len, argv[++i])){
				goto usage;
			}
		}else if(!strcmp(v, "-d")){
			i++;
			append(&data, &len, argv[i], strlen(argv[i]));
		}else if(!strcmp(v, "-o")){
			if(checkFile(argv[++i], 0)){
				goto usage;
			}
			output = fopen(argv[i], "wb");
		}else if(!strcmp(v, "-png")) out_format=PNG;
		else if(!strcmp(v, "-jpeg")) out_format=JPEG;
		else if(!strcmp(v, "-print")) out_format=PRINT;
		else if(!strcmp(v, "-l")) clevel=L;
		else if(!strcmp(v, "-m")) clevel=M;
		else if(!strcmp(v, "-q")) clevel=Q;
		else if(!strcmp(v, "-h")) clevel=H;
		else if(!strcmp(v, "-num")) prefered_mode=MODE_NUMERIC;
		else if(!strcmp(v, "-alp")) prefered_mode=MODE_ALPHANUMERIC;
		else if(!strcmp(v, "-byte")) prefered_mode=MODE_BYTE;
		else if(!strcmp(v, "-auto")) prefered_mode=0;
		else if(!strcmp(v, "-help")) goto usage;
		else if(!strcmp(v, "--help")) goto usage;
		else{
			fprintf(stderr, "unknown option %s\n", v);
			goto usage;
		}
	}
	
	if(len==0){
		read_file(&data, &len, stdin);
	}
	
	int mode=get_supported_mode(data, len);
	if((mode&prefered_mode)!=0) mode=prefered_mode;
	else if(mode&MODE_NUMERIC) mode=MODE_NUMERIC;
	else if(mode&MODE_ALPHANUMERIC) mode=MODE_ALPHANUMERIC;
	else if(mode&MODE_BYTE) mode=MODE_BYTE;

	int version;
	char** module=generate_qr(&version, data, len, mode, clevel);
	switch(out_format){
		case PNG:
			write_png(module, version, 10, output);
			break;
		case JPEG:
			write_jpeg(module, version, 10, output);
			break;
		case PRINT:
			print_module(module, version, output);
			break;
	}
	fclose(output);
	return 0;

	usage:
	fprintf(stderr, USAGE, argv[0]);
	return -1;
}
