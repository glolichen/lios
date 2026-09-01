// NOTE: this file is generated (almost) entirely by ChatGPT

#include <stdbool.h>
#include "../util/arraylist.h"
#include "listtest.h"

#include "../io/output.h"

#define TEST_SIZE 10000
#define RANDOM_OPS 100000

u32 reference[TEST_SIZE];
u64 reference_size = 0;

u32 rng_state = 0x12345678;

u32 rng(void)
{
	rng_state ^= rng_state << 13;
	rng_state ^= rng_state >> 17;
	rng_state ^= rng_state << 5;

	return rng_state;
}

void check_arraylist(struct ArrayList *al, const char *where)
{
	if (al->size != reference_size) {
		vga_printf(
			"\nFAIL [%s]: size mismatch: "
			"arraylist=%u reference=%u\n",
			where,
			al->size,
			reference_size
		);

		for (;;)
			;
	}

	u32 *data = (u32 *) al->l;

	for (u64 i = 0; i < reference_size; i++) {
		if (data[i] != reference[i]) {
			vga_printf(
				"\nFAIL [%s]: index %u mismatch: "
				"arraylist=%u reference=%u\n",
				where,
				i,
				data[i],
				reference[i]
			);

			vga_printf("ArrayList contents:\n");

			for (u64 j = 0; j < al->size; j++)
				vga_printf(
					"  [%u] = %u\n",
					j,
					data[j]
				);

			vga_printf("Reference contents:\n");

			for (u64 j = 0; j < reference_size; j++)
				vga_printf(
					"  [%u] = %u\n",
					j,
					reference[j]
				);

			for (;;)
				;
		}
	}
}

void test_growth(void)
{
	vga_printf("\n=== TEST: growth ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 i = 0; i < 10000; i++) {
		u64 index = arraylist_add(&al);

		if (index != reference_size) {
			vga_printf(
				"FAIL: add returned index %u, expected %u\n",
				index,
				reference_size
			);

			for (;;)
				;
		}

		u32 value = (u32) i * 37 + 17;

		((u32 *) al.l)[index] = value;
		reference[reference_size++] = value;

		check_arraylist(&al, "growth");
	}

	vga_printf("growth: PASS (%u elements)\n", al.size);
}

// WARN: this is REALLY slow
// LiOS isn't unoptimized and very very slow...
void test_remove_front(void)
{
	vga_printf("\n=== TEST: remove front ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 i = 0; i < 5000; i++) {
		u64 index = arraylist_add(&al);
		u32 value = (u32) (i * 7919);

		((u32 *) al.l)[index] = value;
		reference[reference_size++] = value;
	}

	vga_printf("remove front: populated\n");
	check_arraylist(&al, "remove front setup");

	while (reference_size > 0) {
		arraylist_remove(&al, 0);

		for (u64 i = 1; i < reference_size; i++)
			reference[i - 1] = reference[i];

		reference_size--;

		check_arraylist(&al, "remove front");
	}

	vga_printf("remove front: PASS\n");
}

void test_remove_back(void)
{
	vga_printf("\n=== TEST: remove back ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 i = 0; i < 5000; i++) {
		u64 index = arraylist_add(&al);
		u32 value = (u32) (i * 12345 + 0xdeadbeef);

		((u32 *) al.l)[index] = value;
		reference[reference_size++] = value;
	}

	check_arraylist(&al, "remove back setup");

	while (reference_size > 0) {
		u64 index = reference_size - 1;

		arraylist_remove(&al, index);
		reference_size--;

		check_arraylist(&al, "remove back");
	}

	vga_printf("remove back: PASS\n");
}

void test_remove_middle(void)
{
	vga_printf("\n=== TEST: remove middle ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 i = 0; i < 10000; i++) {
		u64 index = arraylist_add(&al);
		u32 value = (u32) (i * 2654435761U);

		((u32 *) al.l)[index] = value;
		reference[reference_size++] = value;
	}

	check_arraylist(&al, "remove middle setup");

	while (reference_size > 0) {
		u64 index = reference_size / 2;

		arraylist_remove(&al, index);

		for (u64 i = index + 1; i < reference_size; i++)
			reference[i - 1] = reference[i];

		reference_size--;

		check_arraylist(&al, "remove middle");
	}

	vga_printf("remove middle: PASS\n");
}

void test_cycles(void)
{
	vga_printf("\n=== TEST: add/remove cycles ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 round = 0; round < 1000; round++) {
		for (u64 i = 0; i < 100; i++) {
			u64 index = arraylist_add(&al);
			u32 value = (u32) (round * 1000 + i);

			((u32 *) al.l)[index] = value;
			reference[reference_size++] = value;

			check_arraylist(&al, "cycle add");
		}

		for (u64 i = 0; i < 50; i++) {
			u64 index = rng() % reference_size;

			arraylist_remove(&al, index);

			for (u64 j = index + 1; j < reference_size; j++)
				reference[j - 1] = reference[j];

			reference_size--;

			check_arraylist(&al, "cycle remove");
		}
	}

	vga_printf("cycles: PASS\n");
}

void test_duplicates(void)
{
	vga_printf("\n=== TEST: duplicates ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 i = 0; i < 10000; i++) {
		u64 index = arraylist_add(&al);
		u32 value = (u32) ((i * 7) & 7);

		((u32 *) al.l)[index] = value;
		reference[reference_size++] = value;

		check_arraylist(&al, "duplicates add");
	}

	while (reference_size > 0) {
		u64 index = rng() % reference_size;

		arraylist_remove(&al, index);

		for (u64 i = index + 1; i < reference_size; i++)
			reference[i - 1] = reference[i];

		reference_size--;

		check_arraylist(&al, "duplicates remove");
	}

	vga_printf("duplicates: PASS\n");
}

void test_random(void)
{
	vga_printf("\n=== TEST: randomized torture ===\n");

	struct ArrayList al = arraylist_new(4);

	reference_size = 0;

	for (u64 operation = 0; operation < RANDOM_OPS; operation++) {
		u32 r = rng();

		bool can_add = reference_size < TEST_SIZE;
		bool can_remove = reference_size > 0;

		if (!can_remove || (can_add && (r % 100) < 60)) {
			u64 index = arraylist_add(&al);

			if (index != reference_size) {
				vga_printf(
					"\nFAIL [random add]: "
					"returned %u expected %u\n",
					index,
					reference_size
				);

				for (;;)
					;
			}

			u32 value = rng();

			((u32 *) al.l)[index] = value;
			reference[reference_size++] = value;

		} else {
			u64 index = rng() % reference_size;

			arraylist_remove(&al, index);

			for (u64 i = index + 1; i < reference_size; i++)
				reference[i - 1] = reference[i];

			reference_size--;
		}

		check_arraylist(&al, "random");

		if ((operation % 1000) == 0)
			vga_printf(
				"  operation %u, size %u\n",
				operation,
				reference_size
			);
	}

	vga_printf(
		"randomized torture: PASS (%u operations)\n",
		RANDOM_OPS
	);
}

void arraylist_torture_test(void)
{
	vga_printf("\n");
	vga_printf("========================================\n");
	vga_printf("       ARRAYLIST TORTURE TEST\n");
	vga_printf("========================================\n");

	test_growth();
	test_remove_front();
	test_remove_back();
	test_remove_middle();
	test_cycles();
	test_duplicates();
	test_random();

	vga_printf("\n========================================\n");
	vga_printf("       ALL ARRAYLIST TESTS PASSED\n");
	vga_printf("========================================\n");
}
