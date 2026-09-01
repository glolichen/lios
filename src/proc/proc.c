#include "proc.h"
#include "../mem/vmalloc.h"
#include "../util/arraylist.h"

// PID 0: idle process
// PID 1: init process
struct ArrayList proc_table;

void proc_init(void) {
	proc_table = arraylist_new(sizeof(struct Process));
}

struct Process *proc_create() {
	u64 new_index = arraylist_add(&proc_table);

	struct Process process = { 0 };
	process.pid = new_index;

	((struct Process *) proc_table.l)[new_index] = process;

	return &((struct Process *) proc_table.l)[new_index];
}
