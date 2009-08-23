#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"

int dict_search(char *word)
{
	int i;
	char *word_copy = (char*) malloc(strlen(word));
	for (i = 0; word[i] > 0; i++) {
		word_copy[i] = word[i];
		if (word[i] <= 'z' && word[i] >= 'a') {
			word_copy[i] += ('A' - 'a');
		} else if (word[i] < 'A' || word[i] > 'z' || (word[i] > 'Z' && word[i] < 'a')) {
			fprintf(stderr, "char out of bounds: %c\n", word[i]);
			free(word_copy);
			return -2;
		}
	}
	word_copy[i] = '\0';

	for (i = 0; i < 2048; i++) {
		if (strcmp(dict[i], word_copy) == 0) {
			free(word_copy);
			return i;
		}
	}

	free(word_copy);
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
	int i, out;

	int temp;
	unsigned char combined[9]; // need two bits form #9
	unsigned long chunks[6];
	char *words[6];

	for (i = 0; i < 6; i++) {
		words[i] = (char*) malloc(5);
	}

	if (argc < 7) {
		fprintf(stderr, "enter s/key: ");
		scanf("%4s %4s %4s %4s %4s %4s", words[0], words[1], words[2], words[3], words[4], words[5]);
	} else {
		for (i = 0; i < 6; i++)
			words[i] = argv[i+1];
	}
	
	for (i = 0; i < 6; i++) {
		temp = dict_search(words[i]);
		if (temp < 0) {
			fprintf(stderr, "unknown word \"%s\" in input\n", words[i]);
			return -1;
		} 
		chunks[i] = (unsigned long) temp;
	}

	out = combine_chunks(11, 6, (void*) combined, chunks);

	/*
	int j;
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

	if (argc < 7) {
		for (i = 0; i < 6; i++) {
			free(words[i]);
		}
	}

	return 0;
}

