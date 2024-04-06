/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "trace.h"

#define MEPC 0x341
#define MCAUSE 0x342
#define MTVEC 0x305

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
#ifdef CONFIG_ETRACE
  if (etrace_fp != NULL) {
    fprintf(etrace_fp, "[%#x] Interrupt NO: %#x\n", epc, NO);
    fflush(etrace_fp);
  }
#endif
  cpu.csr[MEPC] = epc;       // SR[mepc] <- PC
  cpu.csr[MCAUSE] = NO;        // SR[mcause] <- 一个描述失败原因的号码

  switch (cpu.csr[MCAUSE]) {
  case 1:   cpu.csr[MEPC] += 4;  break; // EVENT_YIELD
  default:                        break;
  }

  return cpu.csr[MTVEC];      // PC <- SR[mtvec]
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
