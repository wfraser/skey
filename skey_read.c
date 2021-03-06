/*
 * S/Key Reader
 *
 * by William R. Fraser 8/22/2009
 *
 * Reads in a 6-word RFC-2289-style OTP and outputs the corresponding
 * hexadecimal hash string.
 *
 * usage: skey_read <word1> <word2> <word3> <word4> <word5> <word6>
 *
 * If run with fewer than 6 arguments, you will be promped to type the six
 * words at the terminal.
 *
 * For restrictions regarding usage and distribution, see license at the end of
 * this file.
 */

#include <stdio.h>
#include <string.h>
#include "dict.h"
#include "version.h"

int dict_search(char *word)
{
	int i;
	char word_copy[5];
	for (i = 0; i < 5 && word[i] > 0; i++) {
		word_copy[i] = word[i];
		if (word[i] <= 'z' && word[i] >= 'a') {
			word_copy[i] += ('A' - 'a');
		} else if (word[i] < 'A' || word[i] > 'z' || (word[i] > 'Z' && word[i] < 'a')) {
			fprintf(stderr, "char out of bounds: %c\n", word[i]);
			return -2;
		}
	}
	word_copy[i] = '\0';

	for (i = 0; i < 2048; i++) {
		if (strcmp(dict[i], word_copy) == 0) {
			return i;
		}
	}

	return -1;
}

int combine_chunks(int chunkbits, int num_chunks, void *combined, unsigned long chunks[])
{
	int in, out, bits_in, bits_out;
	unsigned long curr_chunk;

	out = 0;
	bits_out = 7;
	*((unsigned char *)combined + out) = 0;
	for (in = 0; in < num_chunks; in++) {
		curr_chunk = chunks[in];
		for (bits_in = chunkbits - 1; bits_in >= 0; bits_in--)
		{
			*((unsigned char *)combined + out) |= (curr_chunk & (1 << bits_in)) >> bits_in << bits_out;
			bits_out--;
			if (bits_out == -1) {
				out++;
				bits_out = 7;
				*((unsigned char *)combined + out) = 0;
			}
		}
	}

	return out;
}

int main(int argc, char **argv)
{
	int i, j, out;

	int temp;
	unsigned char combined[9]; // need two bits form #9
	unsigned long chunks[6];
	char words[6][5];

	if (argc == 2 && (
			strcmp(argv[1], "--help") == 0
			|| strcmp(argv[1], "-h") == 0
	)) {
		fprintf(stderr, "s/key read v%u.%u", VERSION_MAJOR,
				VERSION_RELEASE);
		if (VERSION_BUILD != 0)
			fprintf(stderr, ".%u", VERSION_BUILD);
		fprintf(stderr, " (c) 2009 by William R. Fraser\n");
		fprintf(stderr, "usage: %s [<word1> <word2> <word3> <word4> "
				"<word5> <word6>]\n", argv[0]);
		return -1;
	}

	if (argc < 7) {
		fprintf(stderr, "enter s/key: ");
		scanf("%4s %4s %4s %4s %4s %4s", words[0], words[1], words[2], words[3], words[4], words[5]);
	} else {
		for (i = 0; i < 6; i++) {
			for (j = 0; j < 5 && argv[i+1][j]; j++)
				words[i][j] = argv[i+1][j];
			words[i][j] = '\0';
		}
	}
	
	for (i = 0; i < 6; i++) {
		temp = dict_search(words[i]);
		if (temp < 0) {
			fprintf(stderr, "unknown word \"%s\" in input\n", words[i]);
			return -2;
		} 
		chunks[i] = (unsigned long) temp;
	}

	out = combine_chunks(11, 6, (void*) combined, chunks);

	/*
	for (i = 0; i < 6; i++) {
		printf("chunk %d: %lu:\t", i, chunks[i]);
		for (j = 10; j >= 0; j--) {
			printf("%lu%s", (chunks[i] & (1 << j)) >> j, (((i*11 + (11-j)) % 8) == 0) ? " " : "" );
		}
		printf("\n");
	}

	printf("\ncombined:");
	for (i = 0; i <= out; i++) {
		printf(" ");
		for (j = 7; j >= 0; j--) {
			printf("%d", (combined[i] & (1 << j)) >> j);
		}
	}
	printf("\n");
	*/

	for (i = 0; i < 8; i++) {
                printf("%.2x", combined[i]);
        }
	printf("\n");

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

THIS SOFTWARE IS PROVIDED BY WILLIAM R  FRASER ''AS IS'' AND ANY
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
