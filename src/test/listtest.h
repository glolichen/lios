#ifndef LISTTEST_H
#define LISTTEST_H

#include "../util/arraylist.h"

void check_arraylist(struct ArrayList *al, const char *where);

void test_growth(void);
void test_remove_front(void);
void test_remove_back(void);
void test_remove_middle(void);
void test_cycles(void);
void test_duplicates(void);
void test_capacity_boundaries(void);
void test_random(void);

void arraylist_torture_test(void);

#endif
