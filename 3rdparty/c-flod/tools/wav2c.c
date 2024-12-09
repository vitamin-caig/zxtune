#include "../backends/wave_format.h"
#include "../flashlib/ByteArray.h"
#include <assert.h>

void doit(char* in_filename, char* out_filename, char* structname) {
	struct ByteArray in_b, *in = &in_b;
	WAVE_HEADER_COMPLETE wh;
	ByteArray_ctor(in);
	ByteArray_open_file(in, in_filename);
	ByteArray_set_endian(in, BAE_LITTLE);
	ByteArray_readMultiByte(in, (void*)&wh, sizeof(WAVE_HEADER_COMPLETE));
	FILE* outfile = fopen(out_filename, "w");
	assert(outfile);
	char buf[1024];
	snprintf(buf, sizeof(buf), 
			"#include \"wave_format.h\"\n"
			"#define FILESIZE %d\n"
			"#define SAMPLERATE %d\n"
			"#define BITS %d\n"
			"#define CHANS %d\n"
			"#define FMTC0 %d\n"
			"#define FMTC1 %d\n"
			"#define DATASIZE %d\n"
			"#define STRUCT_NAME %s\n\n"
			"static const struct {\n"
			"\tWAVE_HEADER_COMPLETE header;\n"
			"\tuint8_t data[FILESIZE - sizeof(WAVE_HEADER_COMPLETE)];\n"
			"} STRUCT_NAME = { \n"
			"\t{\n"
			"\t\t{ { 'R', 'I', 'F', 'F'}, FILESIZE -8, { 'W', 'A', 'V', 'E'} },\n"
			"\t\t{ { 'f', 'm', 't', ' '}, 16, {{FMTC0, FMTC1}}, CHANS, SAMPLERATE, SAMPLERATE * CHANS * BITS/8, CHANS * BITS/8, BITS },\n"
			"\t\t{ { 'd', 'a', 't', 'a' }, DATASIZE }\n"
			"\t},\n"
			"\t{\n\t\t",
			(int) wh.riff_hdr.filesize_minus_8 + 8,
			(int) wh.wave_hdr.samplerate,
			(int) wh.wave_hdr.bitwidth, 
			(int) wh.wave_hdr.channels,
			(int) wh.wave_hdr.format.chars[0],
			(int) wh.wave_hdr.format.chars[1],
			(int) wh.sub2.data_size,
			structname
	);
	fwrite(buf, strlen(buf), 1, outfile);
	size_t processed = 0, datasz = waveheader_get_datasize(&wh);
	while(processed < datasz) {
		fprintf(outfile, "%d,", (uint8_t) ByteArray_readByte(in));
		processed++;
		if(processed % 32 == 0) fprintf(outfile, "\n\t\t");
	}
	fprintf(outfile, "\n\t},\n"
		 "};\n\n"
		 "#undef STRUCT_NAME\n"
		 "#undef DATASIZE\n"
		 "#undef FMTC1\n"
		 "#undef FMTC0\n"
		 "#undef CHANS\n"
		 "#undef BITS\n"
		 "#undef SAMPLERATE\n"
		 "#undef FILESIZE\n\n");
	fclose(outfile);
	ByteArray_close_file(in);
	
}

int main(int argc, char**argv) {
	union { char c[2]; short s;} endian_test = {1, 0};
	assert(endian_test.s == 1); // too lazy to read values in right endian for big endian sys.
	assert(argc == 3);
	int startarg = 1;
	char *in_filename = argv[startarg];
	if(access(in_filename, R_OK) == -1) {
		perror(in_filename);
		return 1;
	}
	char *out_filename = argv[startarg + 1];
	char* cp;
	int to_c = (cp = strrchr(out_filename, '.')) && cp == out_filename + strlen(out_filename) - 2 && cp[1] == 'c';
	assert(to_c);
	char struct_name[256];
	char *st_start = strrchr(out_filename, '/');
	if(st_start) st_start++;
	else st_start = out_filename;
	size_t l = snprintf(struct_name, sizeof struct_name, "%s", st_start);
	struct_name[l-2] = 0;
	doit(in_filename, out_filename, struct_name);
	return 0;
}
