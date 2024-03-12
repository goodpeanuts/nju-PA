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

enum {
  TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_EQ, TK_LEFT, TK_RIGHT, TK_NUM, TK_NOTYPE = 256, 

  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},           // minus
  {"\\*", TK_MUL},         // multiply
  {"\\/", TK_DIV},           // divide
  {"==", TK_EQ},        // equal
  {"[0-9]+u?", TK_NUM},   // number
  {"\\(", TK_LEFT},     // left parenthesis
  {"\\)", TK_RIGHT},    // right parenthesis
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
          printf("too many tokens\n");
          return false;
        }

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NUM: tokens[nr_token].type = 100; break;
          case TK_NOTYPE: break;
          case TK_PLUS: tokens[nr_token].type = 2; break;
          case TK_MINUS: tokens[nr_token].type = 2; break;
          case TK_MUL: tokens[nr_token].type = 3; break;
          case TK_DIV: tokens[nr_token].type = 3; break;
          case TK_EQ: tokens[nr_token].type = 7; break;
          case TK_LEFT: tokens[nr_token].type = 1; break;
          case TK_RIGHT: tokens[nr_token].type = 1;break;
          // default: TODO();
        }
        
        if (rules[i].token_type != TK_NOTYPE) {
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          nr_token++;
        }
        
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
    // printf("%s\n", tokens[p].str);
    // printf("%llu\n", strtoull(tokens[p].str, NULL, 10));
    return (word_t)strtoull(tokens[p].str, NULL, 10);
  } 
  else if (check_parentness(p, q) == true) {
    return eval(p + 1, q - 1, success); 
  } 
  else {
    int op = p;
    int op_type = 666;
    int cnt = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].str[0] == '(') {
        cnt++;
      } else if (tokens[i].str[0] == ')') {
        cnt--;
      } else if (tokens[i].type <= op_type && cnt == 0) { // 这里是<=,确保同等级下先计算前面的优先
        op = i;
        op_type = tokens[i].type;
      }
      if (cnt < 0) {
        *success = false;
        return 0;
      }
    }
    if (cnt != 0 || op == p || op == q) {
      *success = false;
      return 0;
    }
    
    word_t val1 = eval(p, op - 1, success);
    word_t val2 = eval(op + 1, q, success);
    // Log("%u %s %u ", val1, tokens[op].str, val2);
    switch (tokens[op].str[0]) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': if (val2 == 0) {
                  *success = false;
                  return 0;
                } else {
                  return val1 / val2;
                }
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
  // debug
  for (int i = 0; i < nr_token; i++) {
    // printf("%s %d\n", tokens[i].str, tokens[i].type);
    // printf("%s", tokens[i].str);
  }
  // printf("\n");
  int p = 0, q = nr_token - 1;
  return (word_t)eval(p, q, success);
}

void test_expr() {
  // 读入文件input
  int total = 0, correct = 0;
  FILE *fp = fopen("inputu", "r");
  if (fp == NULL) {
    printf("open file failed\n");
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
//   char e[5][128] = {"9795306513u/7866u*7u"};
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