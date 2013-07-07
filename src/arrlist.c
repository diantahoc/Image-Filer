#include "arrlist.h"
#include <string.h>

ArrList *arrlist_create(size_t size)
{
	ArrList *al;
       
	al = (ArrList*) malloc(sizeof(ArrList));
	al->data = (unsigned char*)malloc(size);
	al->size = size;
	al->written = 0;
	al->next = NULL;
	
	return al;
}

void arrlist_destroy(ArrList *al)
{
	if(!al)
		return;
	if(al->next)
		arrlist_destroy(al->next);

	if(al->data)
		free(al->data);
	free(al);
}

size_t arrlist_linear(ArrList *al, unsigned char **out)
{
	unsigned char *result;
	ArrList *iter;
	size_t i, size;

	if(!al) {
		*out = NULL;
		return 0;
	}

	size = 0;

	for(iter = al;iter;iter = iter->next)
		size += iter->written;
	
	result = (unsigned char*)malloc(size);
	i = 0;

	for(iter = al;iter;iter = iter->next) {
		memcpy(result + i, iter->data, iter->written);
		i += iter->written;
	}

	*out = result;
	return size;
}
