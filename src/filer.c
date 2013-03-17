#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrlist.h"
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#include "lodepng.h"

/*
 * Contains the file names to operate on and all flags that steer behaviour.
 */
struct params_t
{
	const char *src;
	const char *dest;
	int decode;
	int help;
};

/*
 * Returns a lowercase copy of an input string. The returned pointer must be
 * explicitly freed afterwards.
 */
static char *tolowers(const char *str)
{
	char *res;
	int i, n;

 	if(!str)
		return NULL;	
	
	n = strlen(str)+1;
	res = malloc(n);

	for(i=0;i<n;++i)
		res[i] = tolower(str[i]);
	return res;
}

/*
 * Writes a single block of data into a newly created file.
 */
static void dump_to_file(const void *data, size_t len, const char *filename)
{
	FILE *f;

	f = fopen(filename, "wb");

	if(!f) return;

	fwrite(data, 1, len, f);
	fclose(f);
}

/*
 * Loads the binary contents of a file into memory and returns its size.
 */
static size_t load_file(unsigned char **_into, const char *filename)
{
	const size_t bufsize = 4096;
	FILE *f;
	ArrList *buf;
	ArrList *iter;
	size_t size, chunk;

	if(!_into)
		return 0;

	f = fopen(filename, "rb");

	if(!f) {
		*_into = NULL;
		return 0;
	}

	/*
	 * Read file contents into a buffer; expand it every time a section has
	 * been filled completely.
	 */
	iter = buf = arrlist_create(bufsize);
	size = 0;

	do {
		if(iter->written == iter->size) {
			iter->next = arrlist_create(bufsize);
			iter = iter->next;
		}

		chunk = fread(iter->data, 1, bufsize, f);
		size += chunk;
		iter->written += chunk;
	} while(chunk);

	fclose(f);
	size = arrlist_linear(buf, _into);
	arrlist_destroy(buf);

	return size;
}

/*
 * Calculates the dimensions of an almost square-shaped image able to store at
 * least len bytes of data when constructed with the given bytes per pixel.
 */
static void find_dimensions(size_t *_w, size_t *_h,
		size_t len, size_t bpp)
{
	size_t w, h;

	/* Determine the amount of pixels needed */
	len = (len + bpp - 1) * bpp / bpp;
	len /= bpp;

	w = h = (size_t) sqrt(len);
	while(w * h < len)
		if(h < w)
			++h;
		else
			++w;

	*_w = w;
	*_h = h;
}

#define COPY(X, S) memcpy(out + off, X, S); off += S;

/*
 * Converts raw data to an image, stores its data in the first three parameters,
 * and returns its size in memory.
 */
static size_t to_pixels(unsigned char **_out, size_t *_w, size_t *_h,
		const unsigned char *data, size_t len)
{
	unsigned char *out;
	size_t off, outlen, w, h;

	find_dimensions(&w, &h, len + sizeof(size_t), 4);
	outlen = w * h * 4;

	out = malloc(outlen);
	off = 0;

	COPY(&len, sizeof(size_t));		/* PL size */
	COPY(data, len);			/* Payload */
	memset(out + off, 0, outlen - off);	/* Padding */

	*_out = out;
	*_w = w;
	*_h = h;
	return outlen;
}

/*
 * Converts encoded image data to raw data.
 */
static size_t to_data(unsigned char **_out, const unsigned char *pixels,
		size_t len)
{
	unsigned char *out;
	size_t outlen;

	outlen = *((size_t*)pixels);
	out = malloc(outlen);
	memcpy(out, pixels + sizeof(size_t), outlen);

	*_out = out;
	return outlen;
}

/*
 * Transforms an encoded PNG file into the raw data it contains.
 */
static int decode(unsigned char **result, size_t *reslen, const char *srcname)
{
	size_t w, h;
	unsigned char *img;

	if(lodepng_decode32_file(&img, &w, &h, srcname)) {
		printf("Failed to open source picture %s.\n", srcname);
		return 0;
	}

	*reslen = to_data(result, img, w*h);
	free(img);
	return 1;
}

/*
 * Transforms a file into pixel data that can then be written to a PNG file.
 */
static int encode(unsigned char **result, size_t *reslen, const char *srcname)
{
	unsigned char *src;
	unsigned char *img;
	size_t imglen, srclen, w, h;
	unsigned errorcode;

	srclen = load_file(&src, srcname);

	if(!src) {
		printf("Failed to open source file %s.\n", srcname);
		goto error;
	}

	imglen = to_pixels(&img, &w, &h, src, srclen);
	free(src);
	errorcode = lodepng_encode32(result, reslen, img, w, h);
	free(img);

	if(errorcode) {
		printf("Image conversion failed: %s\n",
				lodepng_error_text(errorcode));
		goto error;
	}

	return 1;
error:
	*result = NULL;
	*reslen = 0;
	return 0;
}

#define check(A) do{if(A) return EXIT_FAILURE;}while(0)

static void printUsage();
static int parse_params(struct params_t*, int, char**);

int main(int argc, char *argv[])
{
	struct params_t params;
	unsigned char *result;
	size_t reslen, w, h;

	if(!parse_params(&params, argc, argv)) {
		printf("Not enough arguments. Type filer --help for info.\n");
		return EXIT_FAILURE;
	}

	if(params.help) {
		printUsage();
		return EXIT_SUCCESS;
	}

	result = NULL;

	if(params.decode)
		check(!decode(&result, &reslen, params.src));
	else
		check(!encode(&result, &reslen, params.src));

	dump_to_file(result, reslen, params.dest);

	free(result);
	return EXIT_SUCCESS;
}

/*
 * filer [--help] <source> <result>
 */
static int parse_params(struct params_t *params, int argc, char **argv)
{
	int i, len;
	char *str;

	if(!params || argc < 2)
		return 0;
	
	if(!strcmp(argv[1], "--help")) {
		params->src = params->dest = NULL;
		params->decode = 0;
		params->help = 1;
		return 1;
	}

	params->decode = params->help = 0;

	if(argc < 3)
		return 0;

	params->src = argv[1];
	params->dest = argv[2];

	/* Determine whether to encode or decode */
	str = tolowers(argv[1]);
	len = strlen(argv[1]);
	params->decode = (len > 4 && !strcmp(str + (len - 4), ".png"));

	free(str);
	return 1;
}

/*
 * Dropping indenting here so we can conform to the 80 character limit in both
 * code and executable.
 */
static void printUsage()
{
	printf(
"Usage: filer [--help] <source> <result>\n"
"All arguments after result are ignored.\n\n"

"Converts between png images and files.\n\n"

"--help\t\t"	"Shows this list. If used, this must be the first argument;\n"
	"\t\t"	"also, it will prevent any action from taking place.\n"
"\n"
"source\t\t"	"The file to be encoded. If the file extension is .png, filer\n"
	"\t\t"	"will treat it as an encoded image and decode it instead.\n"
"\n"
"result\t\t"	"The file the result will be stored in.\n"
	);
}

#undef check
#undef COPY
#undef LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#undef LODEPNG_NO_COMPILE_ERROR_TEXT
