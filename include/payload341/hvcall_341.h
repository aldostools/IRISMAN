#ifndef HVCALL_341_H
#define HVCALL_341_H


#define HVSC_SYSCALL_341                    811                    // which syscall to overwrite with hvsc redirect
#define HVSC_SYSCALL_ADDR_341               0x80000000001BB414ULL  // where above syscall is in lv2

#define HV_BASE_341                         0x8000000014000000ULL   // where in lv2 to map lv1
#define HV_SIZE_341                         0x200000                // size of lv1 memory to map/dump

#define HPTE_V_BOLTED			0x0000000000000010ULL
#define HPTE_V_LARGE			0x0000000000000004ULL
#define HPTE_V_VALID			0x0000000000000001ULL
#define HPTE_R_PROT_MASK		0x0000000000000003ULL
#define MM_EA2VA(ea)			((ea) & ~0x8000000000000000ULL)


#define SC_QUOTE_(x) #x
#define SYSCALL(num) "li %%r11, " SC_QUOTE_(num) "; sc;"

#define INSTALL_HVSC_REDIRECT(hvcall) uint64_t original_syscall_code_1 = peekq(HVSC_SYSCALL_ADDR_341); \
	uint64_t original_syscall_code_2 = peekq(HVSC_SYSCALL_ADDR_341 + 8); \
	uint64_t original_syscall_code_3 = peekq(HVSC_SYSCALL_ADDR_341 + 16); \
	uint64_t original_syscall_code_4 = peekq(HVSC_SYSCALL_ADDR_341 + 24); \
	pokeqn(HVSC_SYSCALL_ADDR_341, 0x7C0802A6F8010010ULL);	\
	pokeqn(HVSC_SYSCALL_ADDR_341 + 8, 0x3960000044000022ULL | (uint64_t)hvcall << 32);	\
	pokeqn(HVSC_SYSCALL_ADDR_341 + 16, 0xE80100107C0803A6ULL); \
	pokeqn(HVSC_SYSCALL_ADDR_341 + 24, 0x4e80002060000000ULL);
	
#define REMOVE_HVSC_REDIRECT() pokeqn(HVSC_SYSCALL_ADDR_341, original_syscall_code_1); \
	pokeqn(HVSC_SYSCALL_ADDR_341 + 8, original_syscall_code_2); \
	pokeqn(HVSC_SYSCALL_ADDR_341 + 16, original_syscall_code_3); \
	pokeqn(HVSC_SYSCALL_ADDR_341 + 24, original_syscall_code_4);


int lv1_insert_htab_entry341(uint64_t htab_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted_flag, uint64_t flags, uint64_t *hpte_index, uint64_t *hpte_evicted_v, uint64_t *hpte_evicted_r);

int lv1_undocumented_function_114_341(uint64_t start, uint64_t page_size, uint64_t size, uint64_t *lpar_addr);
void lv1_undocumented_function_115_341(uint64_t lpar_addr);

#endif
