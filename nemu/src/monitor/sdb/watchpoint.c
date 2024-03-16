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

#include "sdb.h"

#define NR_WP 32

// typedef struct watchpoint {
//   int NO;
//   struct watchpoint *next;
//   char expr[128];
//   word_t val;
//   /* TODO: Add more members if necessary */
// } WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(char* e) {
  bool flag = true;
  word_t v = expr(e, &flag);
    if (strlen(e) > 127) {
    Log("Expression too long.\n");
    return NULL;
  }
  if (!flag) {
    Log("Invalid expression.\n");
    return NULL;
  }
  if (free_ == NULL) {
    Log("No enough watchpoints.\n");
    return NULL;
  }
  WP *wp = free_;
  strcpy(wp->expr, e);
  wp->val = v;
  free_ = free_->next;
  wp->next = head;
  head = wp;
  // Log("Set watchpoint %d: %s val = %u\n", wp->NO, wp->expr)
  return wp;
}

bool diff_wp_val(WP* wp) {
  bool flag = true;
  word_t v = expr(wp->expr, &flag);
  if (flag == false) {
    Log("Invalid expression.\n");
    return false;
  }
  if (v != wp->val) {
    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
    printf("Old value = 0x%x\n", wp->val);
    printf("New value = 0x%x\n", v);
    wp->val = v;
    return true;
  }
  return false;
}

bool refresh_wp() {
  WP *p;
  for (p = head; p != NULL; p = p->next) {
    if (diff_wp_val(p)) {
      return true;
    }
  }
  return false;
}

void show_wp() {
  WP *p;
  printf("Num\tValue\t\tExpr\n");
  for (p = head; p != NULL; p = p->next) {
    printf("%d\t0x%x\t%s\n", p->NO, p->val, p->expr);
  }
}

void free_wp(WP* wp) {
  WP *p;
  if (wp == head) {
    head = head->next;
  } else {
    for (p = head; p != NULL; p = p->next) {
      if (p->next == wp) {
        p->next = wp->next;
        break;
      }
    }
  }
  wp->next = free_;
  free_ = wp;
}

bool delete_wp(int NO) {
  WP *p;
  for (p = head; p != NULL; p = p->next) {
    if (p->NO == NO) {
      free_wp(p);
      return true;
    }
  }
  return false;
}

