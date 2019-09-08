/* Taken from lv1dumper by flukes1 */

#include <stdint.h>
#include <stdbool.h>
#include "hvcall_341.h"
#include "payload_341.h"
#include <ppu-lv2.h>


static u64 peekq(u64 addr)
{
   lv2syscall1(6, addr);
   return_to_user_prog(u64);
}

static void pokeqn(u64 addr, u64 val)
{
    lv2syscall2(813, addr, val);
}

int lv1_insert_htab_entry341(uint64_t htab_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted_flag,
                          uint64_t flags, uint64_t * hpte_index, uint64_t * hpte_evicted_v, uint64_t * hpte_evicted_r)
{
    INSTALL_HVSC_REDIRECT(0x9E);    // redirect to hvcall 158

    // call lv1_insert_htab_entry
    uint64_t ret = 0, ret_hpte_index = 0, ret_hpte_evicted_v = 0, ret_hpte_evicted_r = 0;
    __asm__ __volatile__("mr %%r3, %4;" "mr %%r4, %5;" "mr %%r5, %6;" "mr %%r6, %7;" "mr %%r7, %8;" "mr %%r8, %9;"
                         SYSCALL(HVSC_SYSCALL_341) "mr %0, %%r3;" "mr %1, %%r4;" "mr %2, %%r5;" "mr %3, %%r6;":"=r"(ret),
                         "=r"(ret_hpte_index), "=r"(ret_hpte_evicted_v), "=r"(ret_hpte_evicted_r)
                         :"r"(htab_id), "r"(hpte_group), "r"(hpte_v), "r"(hpte_r), "r"(bolted_flag), "r"(flags)
                         :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                         "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    REMOVE_HVSC_REDIRECT();

    *hpte_index = ret_hpte_index;
    *hpte_evicted_v = ret_hpte_evicted_v;
    *hpte_evicted_r = ret_hpte_evicted_r;
    return (int) ret;
}


int lv1_undocumented_function_114_341(uint64_t start, uint64_t page_size, uint64_t size, uint64_t * lpar_addr)
{
    INSTALL_HVSC_REDIRECT(0x72);    // redirect to hvcall 114

    // call lv1_undocumented_function_114
    uint64_t ret = 0, ret_lpar_addr = 0;
    __asm__ __volatile__("mr %%r3, %2;" "mr %%r4, %3;" "mr %%r5, %4;" SYSCALL(HVSC_SYSCALL_341) "mr %0, %%r3;"
                         "mr %1, %%r4;":"=r"(ret), "=r"(ret_lpar_addr)
                         :"r"(start), "r"(page_size), "r"(size)
                         :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                         "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    REMOVE_HVSC_REDIRECT();

    *lpar_addr = ret_lpar_addr;
    return (int) ret;
}

void lv1_undocumented_function_115_341(uint64_t lpar_addr)
{
    INSTALL_HVSC_REDIRECT(0x73);    // redirect to hvcall 115

    // call lv1_undocumented_function_115
    __asm__ __volatile__("mr %%r3, %0;" SYSCALL(HVSC_SYSCALL_341)
                         :      // no return registers
                         :"r"(lpar_addr)
                         :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                         "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    REMOVE_HVSC_REDIRECT();
}
