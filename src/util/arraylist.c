#include "const.h"
#include "arraylist.h"

#include "../mem/vmalloc.h"

struct ArrayList arraylist_new(u64 entry_size) {
	struct ArrayList list;
	list.l = (void *) 0, list.size = 0, list.capacity = 0;
	list.entry_size = entry_size;
	return list;
}

// returns the index of the new item
u64 arraylist_add(struct ArrayList *list) {
	if (list->capacity == 0) {
		list->capacity = 1, list->size = 1;
		list->l = vmalloc(1 * list->entry_size);
		return 0;
	}
	if (list->size + 1 <= list->capacity)
		return list->size++;

	list->capacity *= 2;
	list->l = vrealloc(list->l, list->capacity * list->entry_size);
	return list->size++;
}

bool arraylist_remove(struct ArrayList *list, u64 index) {
	if (index >= list->size)
		return false;

	u64 entry_size = list->entry_size;
	u64 new_size = list->size - 1;

	if (new_size >= list->capacity / 2) {
		for (u64 i = index * entry_size; i < new_size * entry_size; i++)
			((u8 *) list->l)[i] = ((u8 *) list->l)[i + entry_size];

		list->size--;
		return true;
	}

	u64 new_capacity = list->capacity / 2;

	u8 *new_list = vmalloc(new_capacity * entry_size);
	for (u64 i = 0; i < index * entry_size; i++)
		new_list[i] = ((u8 *) list->l)[i];

	for (u64 i = index * entry_size; i < new_size * entry_size; i++)
		new_list[i] = ((u8 *) list->l)[i + entry_size];

	vfree(list->l);
	list->l = new_list;
	list->capacity = new_capacity;
	list->size = new_size;

	return true;
}

void arraylist_free(struct ArrayList *list) {
	vfree(list->l);
	list->capacity = 0, list->size = 0;
}

