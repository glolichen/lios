#ifndef GPT_H
#define GPT_H

#include "nvme.h"
#include "../const.h"

struct __attribute__((packed)) Partition {
	// values are INCLUSIVE!
	u64 first_lba, last_lba;
};

struct Partition gpt_read(void);

#endif
