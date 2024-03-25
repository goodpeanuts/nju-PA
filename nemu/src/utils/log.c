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

#include <common.h>

extern uint64_t g_nr_guest_inst;

#ifndef CONFIG_TARGET_AM
FILE *log_fp = NULL;
#ifdef CONFIG_MTRACE
FILE *mtrace_fp = NULL;
#endif

void init_log(const char *log_file) {
  log_fp = stdout;
  if (log_file != NULL) {
    FILE *fp = fopen(log_file, "w");
    Assert(fp, "Can not open '%s'", log_file);
    log_fp = fp;
  }
  Log("Log is written to %s", log_file ? log_file : "stdout");
}

#ifdef CONFIG_MTRACE
void init_mtrace(const char *mtrace_file) {
  mtrace_fp = stdout;
  if (mtrace_file != NULL) {
    FILE *fp = fopen(mtrace_file, "w");
    Assert(fp, "Can not open '%s'", mtrace_file);
    mtrace_fp = fp;
  }
  Log("mtrace is written to %s", mtrace_file ? mtrace_file : "stdout");
}

bool mtrace_addr_enable(paddr_t addr) {
  if (addr >= strtoul(CONFIG_MTRACE_MEM_START, NULL, 16) && addr <= strtoul(CONFIG_MTRACE_MEM_END, NULL, 16)) 
    return true;
  return false;
}
#endif

bool log_enable() {
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_inst >= CONFIG_TRACE_START) &&
         (g_nr_guest_inst <= CONFIG_TRACE_END), false);
}
#endif
