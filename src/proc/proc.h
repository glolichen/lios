#ifndef PROCESS_H
#define PROCESS_H

#include "../util/const.h"
#include "elf.h"

// 8 MiB, like Linux
#define STACK_SIZE 0x800000
// highest lower half / "user" memory address
#define LOWER_HALF_MEM_MAX 0x800000000000

// OSTEP P31, xv6 process
struct CPU_Context {
	u64 rip;
	u64 rsp;
	u64 rbx;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 rbp;
};

enum ProcessState {
	PROC_STATE_UNUSED,
	PROC_STATE_EMBRYO,
	PROC_STATE_SLEEPING,
	PROC_STATE_RUNNABLE,
	PROC_STATE_RUNNING,
	PROC_STATE_ZOMBIE
};

struct Process {
	struct ELFSegmentMem segment_mem;

	u8 *mem; // Start of process memory
	u64 sz; // Size of process memory
	u8 *kstack; // Bottom of kernel stack
	
	// for this process
	enum ProcessState state; // Process state
	u64 pid; // Process ID
	struct Process *parent; // Parent process
	void *chan; // If non-zero, sleeping on chan
	u32 killed; // If non-zero, have been killed

	// we should handle these later
	// struct file *ofile[NOFILE]; // Open files
	// struct inode *cwd; // Current directory
	// struct context context; // Switch here to run process
	// struct trapframe *tf; // Trap frame for the
};

struct Process *proc_create();

#endif
