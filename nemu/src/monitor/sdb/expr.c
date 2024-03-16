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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory/vaddr.h>

enum {
  TK_NUM, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_EQ, TK_UNEQ, TK_LEFT, TK_RIGHT, TK_AND, TK_DEREF, TK_REG, TK_NOTYPE = 256, 

  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"\\$[$0-9a-z]+", TK_REG},          // register
  {" +", TK_NOTYPE},                    // spaces
  {"\\(", TK_LEFT},                     // left parenthesis
  {"\\)", TK_RIGHT},                    // right parenthesis
  {"0x[0-9A-Fa-f]+", TK_NUM},           // hex number
  {"[0-9]+u?", TK_NUM},                 // number
  {"\\+", TK_PLUS},                     // <expr> "+" <expr>
  {"-", TK_MINUS},                      // <expr> "-" <expr>
  {"\\*", TK_MUL},                      // <expr> "*" <expr>
  {"\\/", TK_DIV},                      // <expr> "/" <expr>
  {"==", TK_EQ},                        // <expr> "==" <expr>
  {"!=", TK_UNEQ},                      // <expr> "!=" <expr>
  {"&&", TK_AND}                        // <expr> "&&" <expr>
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[64];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        
        if (nr_token >= 65536) {
          Log("too many tokens\n");
          return false;
        }

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (rules[i].token_type == TK_NOTYPE) {
          // tokens[nr_token].type = 0;
          break;
        }

        switch (rules[i].token_type) {
          case TK_NUM: tokens[nr_token].type = 0; break;
          case TK_REG: tokens[nr_token].type = 0; break;
          case TK_PLUS: tokens[nr_token].type = 4; tokens[nr_token].str[0] = '+'; break;
          case TK_MINUS: tokens[nr_token].type = 4; tokens[nr_token].str[0] = '-'; break;
          case TK_MUL: tokens[nr_token].type = 3; tokens[nr_token].str[0] = '*'; break;
          case TK_DIV: tokens[nr_token].type = 3; tokens[nr_token].str[0] = '/'; break;
          case TK_EQ: tokens[nr_token].type = 7; tokens[nr_token].str[0] = 'e'; break;
          case TK_UNEQ: tokens[nr_token].type = 7; tokens[nr_token].str[0] = 'u'; break;
          case TK_AND: tokens[nr_token].type = 11; tokens[nr_token].str[0] = 'a'; break;
          case TK_LEFT: tokens[nr_token].type = 1; tokens[nr_token].str[0] = substr_start[0]; break;
          case TK_RIGHT: tokens[nr_token].type = 1; tokens[nr_token].str[0] = substr_start[0]; break;
          // default: TODO();
        }
        
        if (rules[i].token_type == TK_NUM || rules[i].token_type == TK_REG) {
          strncpy(tokens[nr_token].str, substr_start, substr_len);
        } else {
          tokens[nr_token].str[1] = '\0';
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

// 用于区分是否是主运算符模式
bool check_parentness(int p, int q) {
  if (tokens[p].str[0] != '(' || tokens[q].str[0] != ')') {
    return false;
  }
  int cnt = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].str[0] == '(') {
      cnt++;
    } else if (tokens[i].str[0] == ')') {
      cnt--;
    }
    if (cnt < 0 || (cnt == 0 && i < q)) {
      return false;
    }
  }
  return cnt == 0;
}

// 求值函数
word_t eval(int p, int q, bool *success) {
  if (success == false ||  p > q) {
    success = false;
    return 0;
  } 
  else if (p == q) {
    if (tokens[p].str[0] == '0' && tokens[p].str[1] == 'x') {
      return (word_t)strtoul(tokens[p].str, NULL, 16);
    } 
    else if (tokens[p].str[0] == '$') {
      word_t res = isa_reg_str2val(&tokens[p].str[1], success);
      return res;
    } 
    else {
      return (word_t)strtoul(tokens[p].str, NULL, 10);
    }
  } 
  else if (check_parentness(p, q) == true) {
    return eval(p + 1, q - 1, success); 
  } 
  else {
    int op = p;
    int op_type = 0;
    int cnt = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].str[0] == '(') {
        cnt++;
      } else if (tokens[i].str[0] == ')') {
        cnt--;
      } else if (tokens[i].type >= op_type && cnt == 0) { // 这里是<=,确保同等级下先计算前面的优先
        op = i;
        op_type = tokens[i].type;
      }
      if (cnt < 0) {
        *success = false;
        return 0;
      }
    }
    if (cnt != 0) {
      Log("parentness do not match: cnt = %d left = %d right = %d", cnt, p, q);
      *success = false;
      return 0;
    }

    // 解引用
    if (tokens[op].type == 1 && tokens[op].str[0] == 'd') {
      word_t addr = eval(op + 1, q, success);
      if  (addr < 0x80000000 || addr > 0x87ffffff) {
        *success = false;
        Log("address = %x is out of bound of pmem", addr);
        return 0;
      }
      return (word_t)vaddr_read(addr, 4);
    }
    word_t val1 = eval(p, op - 1, success);
    word_t val2 = eval(op + 1, q, success);
    #ifdef CONFIG_EXPR_DEBUG
      Log("p = %d | op = %d | q = %d", p, op,  q);
      Log("%u %s %u ", val1, tokens[op].str, val2);
    #endif
    switch (tokens[op].str[0]) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': if (val2 == 0) {
                  Log("divided by zero");
                  *success = false;
                  return 0;
                } else {
                  return val1 / val2;
                }
      case 'e': return val1 == val2;
      case 'u': return val1 != val2;
      case 'a': return val1 && val2;
      default : *success = false;
    }

  }
  return 0;
}

word_t expr(char *e, bool *success) {
  memset(tokens, 0, sizeof(tokens));
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    // 区分解引用和乘法，仍需更多测试54
    if (tokens[i].str[0] == '*' && (i == 0 || tokens[i - 1].type > 1) ) {
      tokens[i].type = 1;
      tokens[i].str[0] = 'd';
    }
  }

  #ifdef CONFIG_EXPR_DEBUG
  Log("no\ttype\tstr");
  for (int i = 0; i < nr_token; i++) {
    Log("[%d]\t%d\t%s", i, tokens[i].type, tokens[i].str);
  }
  printf("\n");
  #endif

  int p = 0, q = nr_token - 1;
  return (word_t)eval(p, q, success);
}

void test_expr() {
  // 读入文件input
  int total = 0, correct = 0;
  FILE *fp = fopen("in3", "r");
  if (fp == NULL) {
    Log("open file failed");
    return;
  }
  word_t res;
  char buf[65536];
  while (fscanf(fp, "%u %s", &res, buf) != EOF) {
    total++;
    bool flag = true;
    word_t ans = expr(buf, &flag);
    if (flag) {
      if (ans != res) {
        Log("%d wrong answer %u", total, ans);
        printf("%u %s\n", res, buf);
      } else {
        correct++;
      }
    } else {
      Log("%d expr calc failed.", total);
      printf("%u %s\n", res, buf);
    }
  }
  Log("correct rate = %d/%d\n", correct, total);
}

// void test_expr() {
//   int n = 1;
//   char e[5][128] = {"0x12 + 0x26"};
//   for (int i = 0; i < n; i++) {
//     bool flag = true;
//     word_t res = expr(e[i], &flag);
//     if (flag) {
//       printf("eval = %u\n", res);
//       printf("%d expr test passed.\n", flag);
//     } else {
//       printf("expr test failed.\n");
//     }
//   }
// }