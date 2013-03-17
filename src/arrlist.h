#pragma once

#include <stdlib.h>

typedef struct arrlist
{
	struct arrlist *next;
	unsigned char *data;
	size_t size, written;
}ArrList;

ArrList *arrlist_create(size_t size);
void arrlist_destroy(ArrList *al);
size_t arrlist_linear(ArrList *al, unsigned char **out);
