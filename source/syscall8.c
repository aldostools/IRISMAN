/* syscall8.c

Copyright (c) 2010 Hermes <www.elotrolado.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of
  conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list
  of conditions and the following disclaimer in the documentation and/or other
  materials provided with the distribution.
- The names of the contributors may not be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "syscall8.h"

int sys8_disable_all = 0;

#define SYSCALL8_OPCODE_GET_MAMBA			0x7FFF
#define SYSCALL8_OPCODE_GET_VERSION			0x7000
#define SYSCALL8_OPCODE_GET_VERSION2		0x7001

static uint64_t syscall_sk1e(register uint64_t cmd, register uint64_t param1, register  uint64_t param2, register  uint64_t param3)
{
    if(sys8_disable_all) return -1ULL;

#ifdef WITH_SYS8ASM
    __asm__ volatile ("li      11, 0x8\n\t" "sc\n\t":"=r" (cmd), "=r"(param1), "=r"(param2), "=r"(param3)
                      :"r"(cmd), "r"(param1), "r"(param2), "r"(param3)
                      :"%r0", "%r12", "%lr", "%ctr", "%xer", "%cr0", "%cr1", "%cr5", "%cr6", "%cr7", "memory");
    return cmd;
#else
    lv2syscall4(SYSCALL_SK1E, cmd, param1, param2, param3);
    return_to_user_prog(uint64_t);
#endif
}


int sys8_mamba_version(uint16_t *version)
{
    if(sys8_disable_all) return -1;

    lv2syscall4(SYSCALL_MAMBA, SYSCALL8_OPCODE_GET_VERSION2, (uint64_t)(uint32_t)version, 0ULL, 0ULL);
    return_to_user_prog(uint64_t);
}

int sys8_mamba(void)
{
    if(sys8_disable_all) return -1;

    lv2syscall4(SYSCALL_MAMBA, SYSCALL8_OPCODE_GET_MAMBA, 0ULL, 0ULL, 0ULL);
    return_to_user_prog(uint64_t);
}

int sys8_disable(uint64_t key)
{
    return (int) syscall_sk1e(0ULL, key, 0ULL, 0ULL);
}

int sys8_enable(uint64_t key)
{
    return (int) syscall_sk1e(1ULL, key, 0ULL, 0ULL);
}

uint64_t sys8_memcpy(uint64_t dst, uint64_t src, uint64_t size)
{
    return syscall_sk1e(2ULL, dst, src, size);
}

uint64_t sys8_memcpyinstr(uint64_t dst, uint64_t src, uint64_t size)
{
    return syscall_sk1e(12ULL, dst, src, size);
}

int sys8_pokeinstr(uint64_t addr, uint64_t data)
{
    return (int) syscall_sk1e(13ULL, addr, data, 0ULL);
}

uint64_t sys8_memset(uint64_t dst, uint64_t val, uint64_t size)
{
    return syscall_sk1e(3ULL, dst, val, size);
}

uint64_t sys8_call(uint64_t addr, uint64_t param1, uint64_t param2)
{
    return syscall_sk1e(4ULL, addr, param1, param2);
}

uint64_t sys8_alloc(uint64_t size, uint64_t pool)
{
    return syscall_sk1e(5ULL, size, pool, 0ULL);
}

uint64_t sys8_free(uint64_t addr, uint64_t pool)
{
    return syscall_sk1e(6ULL, addr, pool, 0ULL);
}

void sys8_panic(void)
{
    syscall_sk1e(7ULL, 0ULL, 0ULL, 0ULL);
}

int sys8_perm_mode(uint64_t mode)
{
    return (int) syscall_sk1e(8ULL, mode, 0ULL, 0ULL);
}

int sys8_sys_configure(uint64_t mode)
{
    return (int) syscall_sk1e(10ULL, mode, 0ULL, 0ULL);
}


int sys8_lv1_syscall(lv1_reg *in, lv1_reg *out)
{
    return (int) syscall_sk1e(11ULL, (uint64_t) in, (uint64_t) out, 0ULL);
}

uint64_t sys8_path_table(uint64_t addr_table)
{
    return syscall_sk1e(9ULL, addr_table, 0ULL, 0ULL);
}
