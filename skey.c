/*
 * S/Key OTP Calculator (RFC 2289)
 * 
 * (c) William R. Fraser 2009
 * v1.0.0 7/20/2009 (first release)
 * v1.1.0 9/8/2009 (current release)
 *
 * compile with:
 *	gcc -Wall -o skey -lmhash -lm skey.c
 *
 * usage: skey [otp-<hash>] <rounds> <seed>
 *
 * You will be prompted for your secret password, and then skey will print the
 * one-time password in hexadecimal and six-word form.
 *
 * For restrictions regarding usage and distribution, see license at the end of
 * this file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mhash.h>
#include <string.h>
#include <termios.h>
#include "dict.h"
#include "version.h"

/*
 * Read a password from the terminal, after printing prompt.
 * After calling, secret is allocated to the correct size, and that size is
 * stored in secret_sz.
 * In case of badness, prints a helpful message and returns with secret set to
 * NULL.
 */
void skey_getpass(const char *prompt, char **secret, int *secret_sz)
{
	char c;
	int alloc = 256;
	struct termios old, new;

	*secret = (char*)malloc(alloc);
	*secret_sz = 0;

	printf("%s", prompt);

	/*
	 * Disable echoing input to screen. This stanza is from
	 * http://www.gnu.org/software/libc/manual/html_node/getpass.html
	 * because getpass(3) is missing on many systems, notably Cygwin.
	 */
	tcgetattr(fileno(stdin), &old);
	new = old;
	new.c_lflag &= ~ECHO;
	tcsetattr(fileno(stdin), TCSAFLUSH, &new);
	
	while (!feof(stdin) && !ferror(stdin)) {
		c = getc(stdin);

		if ((*secret_sz) + 1 == alloc) {
			*secret = (char*)realloc(*secret, alloc+=256);
			if (*secret == NULL) {
				perror("error allocating password buffer");
				tcsetattr(fileno(stdin), TCSAFLUSH, &old);
				return;
			}
		}

		if (c == EOF || c == '\n') {
			(*secret)[*secret_sz] = '\0';
			tcsetattr(fileno(stdin), TCSAFLUSH, &old);
			return;
		} else {
			(*secret)[(*secret_sz)++] = c;
		}
	}

	// can't get here
	return;
}

/*
 * fold the hash down to 64 bits
 * After calling, the memory at output will point to 8 bytes.
 */
void hash_finalize(int hash, char *input, char **output)
{
	int i, j, oversize;

	*output = (char*) malloc(8);

	oversize = mhash_get_block_size(hash) - 8;
	for (i = 0; i < oversize; i++) {
		input[(i%8)] ^= input[(i+8)];
	}

	if (hash == MHASH_SHA1) {
		// each group of 4 bytes has the bytes in reverse order
		// wtf.
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 4; j++) {
				(*output)[i*4+j] = input[(i+1)*4-j-1];
			}
		}
	} else {
		for (i = 0; i < 8; i++) {
			(*output)[i] = input[i];
		}
	}
}

/*
 * Run the specified hash on the input the given number of times.
 * After each round, the hash output is folded to 64 bits using hash_finalize().
 */
int do_hash(int hash, int rounds, char *input, size_t input_sz, char **output, size_t *output_sz)
{
	MHASH h;
	char *to_free;

	while (--rounds > -1) {
		h = mhash_init(hash);
		mhash(h, input, input_sz);
		input = (char *) mhash_end(h);
		
		to_free = input;
		hash_finalize(hash, input, &input);
		free(to_free);
		input_sz = 8;
		
	}

	*output = input;	
	*output_sz = 8;

	return 0;
}

/*
 * Convert the hash into a hexidecimal string.
 */
void hash_hex(char *input, size_t input_sz, char **output, size_t *output_sz)
{
	int i;

	*output = (char*) malloc(2 * input_sz + 1);
	
	for (i = 0; i < input_sz; i++) {
		sprintf((*output) + 2*i, "%.2x", (unsigned char) input[i]);
	}

	*output_sz = 2 * input_sz;
}

/*
 * Split a byte stream of inputsz bytes into chunks of chunkbits number of bits.
 * The last chunk may be right-padded with zero bits if there aren't enough
 * input bytes.
 *
 * The chunks[] array is expected to have enough storage allocated.
 *
 * i.e. if asked to convert 2 bytes to 7-bit chunks:
 *	byte 0   byte 1
 *	01101101 01001101
 *	0110110 1010011 0100000
 *	chunk 0 chunk 1 chunk 2
 */
void data_chunk(int chunkbits, int inputsz, void *hash, unsigned long chunks[])
{
	int in, out, bits_in, bits_out, bit;

	in = 0;
	out = 0;
	bits_in = 0;
	bits_out = 0;
	chunks[0] = 0;

	/* dribble bits into buckets, making sure to change the buckets when
	 * they become full */
	while (1) {
		bit = (*((unsigned char *) hash + in) & (1 << (7 - bits_in))) >> (7 - bits_in);
		chunks[out] |= bit << ((chunkbits - 1) - bits_out);
		bits_in++;
		bits_out++;
		if (bits_in == 8) {
			bits_in = 0;
			in++;
			if (in == inputsz) {
				break;
			}
		}
		if (bits_out == chunkbits) {
			bits_out = 0;
			out++;
			chunks[out] = 0;
		}
	}
}

/*
 * Break the hash into 11-bit chunks so we can turn the hash into dictionary
 * words. Additionally, puts a checksum in the last two bits of chunk #5 as per
 * RFC 2289.
 */
void hash_break(void *hash, unsigned long chunks[6])
{
	int i, byte, checksum;

	data_chunk(11, 8, hash, chunks);
	
	checksum = 0;
	for (i = 0; i < 8; i++) {
		byte = *((unsigned char *) hash + i);
		checksum += byte & 0x03;
		checksum += (byte & 0x0C) >> 2;
		checksum += (byte & 0x30) >> 4;
		checksum += (byte & 0xC0) >> 6;
	}

	chunks[5] |= checksum & 3;
}

#if 0
/*
 * Another way of doing this, using unsigned long longs. This method sucks.
 */
void xx_hash_break(unsigned long long int hash, unsigned long chunks[6])
{
	int i, checksum;
	unsigned int check = 0x0A0B;
	
	/* little endian is a pain
	 * determine byte sex by looking down int's skirt */
	if (*((unsigned char *)(&check)) == 0x0B) {
		hash = (hash << 56) | 
        		((hash << 40) & 0x00FF000000000000ULL) |
        		((hash << 24) & 0x0000FF0000000000ULL) |
        		((hash << 8)  & 0x000000FF00000000ULL) |
        		((hash >> 8)  & 0x00000000FF000000ULL) |
        		((hash >> 24) & 0x0000000000FF0000ULL) |
        		((hash >> 40) & 0x000000000000FF00ULL) |
        		(hash >> 56);
	}

	chunks[0] = (hash & 0xff80000000000000ULL) >> 53; /* top 11 bits      */
	chunks[1] = (hash & 0x007ff00000000000ULL) >> 42; /* next 11 bits     */
	chunks[2] = (hash & 0x00000ffe00000000ULL) >> 31; /* ... etc.         */
	chunks[3] = (hash & 0x00000001ffc00000ULL) >> 20;
	chunks[4] = (hash & 0x00000000003ff800ULL) >> 9;
	chunks[5] = (hash & 0x00000000000007ffULL) << 2; /* two left over     */
	
	checksum = 0;
	for (i = 0; i < 64; i += 2) {
		/* RFC 2289's goofy checksum
		 * break the hash into pairs of adjacent bits, sum the values
		 * thus obtained, and use the two least-significant bits */
		checksum += (hash & (3ULL << i)) >> i;
	}
	chunks[5] |= checksum & 3;
}
#endif //0

int main(int argc, char **argv, char **envp)
{
	char *input, *output, *secret, *final, *hashfunc_str;
	int rounds, ret, hashfunc;
	size_t input_sz, output_sz, secret_sz, final_sz;
	unsigned long chunks[6];

	if (argc < 3) {
		fprintf(stderr, "s/key v%u.%u", VERSION_MAJOR, VERSION_RELEASE);
		if (VERSION_BUILD != 0)
			fprintf(stderr, ".%u", VERSION_BUILD);
		fprintf(stderr, " (c) 2009 by William R. Fraser\n");
		fprintf(stderr, "usage: %s [otp-<hash>] <rounds> <seed>\n", argv[0]);
		return 1;
	}
	
	if (argc > 3) {
		if (strcmp(argv[1], "otp-md4") == 0) {
			hashfunc = MHASH_MD4;
		} else if (strcmp(argv[1], "otp-md5") == 0) {
			hashfunc = MHASH_MD5;
		} else if (strcmp(argv[1], "otp-sha1") == 0) {
			hashfunc = MHASH_SHA1;
		} else {
			fprintf(stderr, "%s: unknown hash function specified : %s\n",
				argv[0], hashfunc_str);
			return 1;
		}
		argv[1] = argv[2]; /* shift args */
		argv[2] = argv[3];
	} else {
		hashfunc = MHASH_MD5;
	}
		

	ret = sscanf(argv[1], "%d", &rounds);
	if (ret < 1 || rounds < 0) {
		perror("invalid number of rounds specified");
		return 1;
	}

	input_sz = strlen(argv[2]);

	/* Use our version of getpass() */
	skey_getpass("Secret: ", &secret, &secret_sz);

	/* concatenate secret onto the end of the seed */
	input = (char*) malloc(input_sz + secret_sz + 1);
	strncpy(input, argv[2], input_sz);
	strncpy(input + input_sz, secret, secret_sz);
	input_sz += secret_sz;
	input[input_sz] = '\0';

	/* run the specified number of hash rounds */
	do_hash(hashfunc, rounds + 1, input, input_sz, &output, &output_sz);

	/* get a hexadecimal string */
	hash_hex(output, output_sz, &final, &final_sz);
	final[final_sz] = '\0';

	/* break hash into word chunks */
	hash_break(output, chunks);

	printf("%s\n%s %s %s %s %s %s\n", final, dict[chunks[0]], 
		dict[chunks[1]], dict[chunks[2]], dict[chunks[3]],
		dict[chunks[4]], dict[chunks[5]]); 

	return 0;
}

/*

Copyright (c) 2009, William R. Fraser
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of William R. Fraser nor the names of other
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY William R. Fraser ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLIAM R. FRASER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

vim: sts=8 ts=8 noexpandtab
*/
