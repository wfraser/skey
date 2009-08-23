#include <stdio.h>

int main()
{
	unsigned long long mask;
	int i, j;

	for (i = 0; i < 6; i++) {
		mask = 0;
		for (j = 0; j < 11; j++) {
			if (!(i == 5 && j > 8))
				mask += (1ULL << (11*i + j));
		}
		printf("%d: %llx\n", i, mask);
	}
	return 0;
}
