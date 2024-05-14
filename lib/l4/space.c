#include <l4/space.h>

bool is_nil_fpage(L4_fpage_t fpage);
L4_fpage_t L4_fpage(unsigned base_address, int fpage_size);
L4_fpage_t L4_fpage_log2(unsigned base_address, int fpage_size_log2);
unsigned L4_address(L4_fpage_t fpage);
unsigned L4_size(L4_fpage_t fpage);
unsigned L4_size_log2(L4_fpage_t fpage);
unsigned L4_rights(L4_fpage_t fpage);
void L4_set_rights(L4_fpage_t *fpage, unsigned access_rights);
L4_fpage_t L4_fpage_add_rights(L4_fpage_t page, unsigned rights);
L4_fpage_t *L4_fpage_add_rights_to(L4_fpage_t *page, unsigned rights);
L4_fpage_t L4_fpage_remove_rights(L4_fpage_t page, unsigned rights);
L4_fpage_t *L4_fpage_remove_right_from(L4_fpage_t *page, unsigned rights);
