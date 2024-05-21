#include <l4/space.h>

bool is_nil_fpage(L4_fpage_t fpage);
unsigned floorlog2(unsigned value);
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
void L4_unmap(unsigned control);
L4_fpage_t L4_unmap_fpage(L4_fpage_t f);
void L4_unmap_fpages(unsigned n, L4_fpage_t *fpages);
L4_fpage_t L4_flush_fpage(L4_fpage_t f);
void L4_flush_fpages(unsigned count, L4_fpage_t *pages);
L4_fpage_t L4_get_status(L4_fpage_t fpage);
bool L4_was_referenced(L4_fpage_t fpage);
bool L4_was_written(L4_fpage_t fpage);
bool L4_was_executed(L4_fpage_t fpage);

unsigned L4_space_control(L4_thread_id space_specifier, unsigned control,
                          L4_fpage_t kip_area, L4_fpage_t utcb_area,
                          L4_thread_id redirector, unsigned *old_control);
