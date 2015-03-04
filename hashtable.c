#include "hashtable.h"

#include <stdio.h>
#include <string.h>

int
hashtable_set(void *table[], int nel, void *item, int (*hash)(const void *),
	int (*compare)(const void *, const void *))
{
        int i;

        for (i = hash(item) % nel; table[i] != NULL; i = (i + 1) % nel) {
		if (compare(table[i], item) == 0) {
                        break;
                }
        }

        table[i] = item;

        return 1;
}

void *
hashtable_get(void *table[], int nel, void *item, int (*hash)(const void *),
	int (*compare)(const void *, const void *))
{
        int i;

        for (i = hash(item) % nel; table[i] != NULL; i = (i + 1) % nel) {
                if (compare(table[i], item) == 0) {
                        return table[i];
                }
        }

        return NULL;
}
