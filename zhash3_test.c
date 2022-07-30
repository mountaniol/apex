#include "zhash3.h"

/* Create and release an empty zhash table */
void basic_test(void)
{
	ztable_t *zt = zcreate_hash_table();
	if (NULL == zt) {
		DE("[TEST] Failed to allocate zhash table\n");
		abort();
	}

	/* The table must be empty - 0 number of entries */
	if (zt->entry_count > 0) {
		DE("[TEST] Number of entries must bw 0, but it is %d\n", zt->entry_count);
		abort();
	}

	/* The entries must be NULL pointer of entries */
	if (NULL != zt->entries) {
		DE("[TEST] Pointer of entries must be NULL, but it is %p\n", zt->entries);
		abort();
	}

	/* Free the table */
	zfree_hash_table(zt);
	DD("[TEST] Successfully finished basic test\n");
}

int main(void)
{
	basic_test();
	return 0;
}
