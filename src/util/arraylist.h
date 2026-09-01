#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stdbool.h>
#include "const.h"

struct ArrayList {
	void *l;
	u64 size, capacity;
	u64 entry_size;
};

struct ArrayList arraylist_new(u64 entry_size);
u64 arraylist_add(struct ArrayList *list);
bool arraylist_remove(struct ArrayList *list, u64 index);
void arraylist_free(struct ArrayList *list);

#endif
