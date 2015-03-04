#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

int
hashtable_set(void *table[], int nel, void *item, int (*hash)(const void *),
	int (*compare)(const void *, const void *));

void *
hashtable_get(void *table[], int nel, void *item, int (*hash)(const void *),
	int (*compare)(const void *, const void *));
	
#endif /* __HASHTABLE_H__ */
