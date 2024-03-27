#include "ftrace.h"
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

char *elf_file = NULL; 

char current_func[BUF_SIZE] = "_start";
char* ptr_current_func = current_func;

size_t ft_cnt = 0;
size_t ft_tab = 1;
Ftrace_record* ft_record = NULL;

size_t ret_info_cnt = 0;
RetInfo* ret_info = NULL;

size_t func_info_cnt = 0;
FunctionInfo* function_info = NULL; 

bool isfunc(paddr_t func_addr) {
  for (int i = 0; i < func_info_cnt; i++) {
    if (function_info[i].address == func_addr) {
      return true;
    }
  }
  return false;
}

bool isret(paddr_t addr) {
  for (size_t i = 0; i < ret_info_cnt; i++) {
    if (ret_info[i].address == addr) {
      return true;
    }
  }
  return false;
}

char* get_func_name(paddr_t func_addr) {
  for (int i = 0; i < func_info_cnt; i++) {
    if (function_info[i].address == func_addr) {
      return function_info[i].name;
    }
  }
  return "Unknown Call";
}

char* get_ret_name(paddr_t addr) {
  for (size_t i = 0; i < ret_info_cnt; i++) {
    if (ret_info[i].address == addr) {
      return ret_info[i].name;
    }
  }
  return "Unknown Ret";
}

void add_ret_info(paddr_t addr, char* name) {
  resize_buf((void**)&ret_info, sizeof(RetInfo), &ret_info_cnt);

  ret_info[ret_info_cnt].address = addr;
  ret_info[ret_info_cnt].name = name;
  ret_info_cnt++;
}


void init_ftrace(const char *ftrace_file) {
  int fd;
  if (ftrace_file != NULL) {
    fd = open(ftrace_file, O_RDONLY);
    assert(fd != -1);
  }
  Elf32_Ehdr ehdr;

  int n = read(fd, &ehdr, sizeof(ehdr));
  assert(n == sizeof(ehdr));

  lseek(fd, ehdr.e_shoff, SEEK_SET);

  Elf32_Shdr shdr;
  char *strtab = NULL;
  for (int i = 0; i < ehdr.e_shnum; i++) {
    n = read(fd, &shdr, sizeof(shdr));
    assert(n == sizeof(shdr));
    if (shdr.sh_type == SHT_STRTAB) {
      lseek(fd, shdr.sh_offset, SEEK_SET);
      strtab = malloc(shdr.sh_size);
      n = read(fd, strtab, shdr.sh_size);
      assert(n == shdr.sh_size);
      break;
    }
  }

  function_info = malloc(sizeof(FunctionInfo) * 32);
  lseek(fd, ehdr.e_shoff, SEEK_SET);
  for (int i = 0; i < ehdr.e_shnum; i++) {
    n = read(fd, &shdr, sizeof(shdr));
    assert(n == sizeof(shdr));
    if (shdr.sh_type == SHT_SYMTAB) {
      Elf32_Sym sym;
      lseek(fd, shdr.sh_offset, SEEK_SET);
      for (int j = 0; j < shdr.sh_size / sizeof(sym); j++) {
        n = read(fd, &sym, sizeof(sym));
        assert(n == sizeof(sym));
        if (sym.st_info == 0x12) {
          if (func_info_cnt++ % 32 == 0) {
            function_info = realloc(function_info, sizeof(FunctionInfo) * (func_info_cnt + 32));
          }
          function_info[func_info_cnt - 1].address = sym.st_value;
          function_info[func_info_cnt - 1].name = strdup(strtab + sym.st_name);
          // printf("Function %s at 0x%x\n", strtab + sym.st_name, sym.st_value);
        }
      }
      break;
    }
  }

  free(strtab);
  close(fd);
}

void ftrace(int type, vaddr_t pc_addr, vaddr_t func_addr) {
  if (type == ret && isret(func_addr) == false) {
    return;
  }

  if (type == call) {
    if (isfunc(func_addr) == false) {
      return;
    }
    add_ret_info(pc_addr + 4, strdup(ptr_current_func));
    ptr_current_func = get_func_name(func_addr);
  }

  resize_buf((void**)&ft_record, sizeof(Ftrace_record), &ft_cnt);

  if (ft_cnt != 0) {
    if (type == call && ft_record[ft_cnt - 1].type == call) {
      ft_tab++;
    } else if (type == ret && ft_record[ft_cnt - 1].type == ret) {
      ft_tab--;
    }
  };
  ft_record[ft_cnt].type = type;
  ft_record[ft_cnt].pc_addr = pc_addr;
  ft_record[ft_cnt].func_addr = func_addr;
  ft_record[ft_cnt].tab = ft_tab;
  ft_cnt ++;
}

void ftrace_output() {
    printf("\n [Function info:] \n");
    for (int i = 0; i < func_info_cnt; i++) {
        printf("Function %s at 0x%x\n", function_info[i].name, function_info[i].address);
    }
    printf("\n [Call & Ret info:] \n");
    for (int i = 0; i < ft_cnt; i++) {
        printf("0x%x: %*s", ft_record[i].pc_addr, (int)ft_record[i].tab, "");
        if (ft_record[i].type == call) {
        printf("[Call]  %s@0x%x\n", get_func_name(ft_record[i].func_addr), ft_record[i].func_addr);
        // printf("[Call]  0x%x\n", ft_record[i].func_addr);
        } else {
        printf("[Return] %s@0x%x\n", get_ret_name(ft_record[i].func_addr), ft_record[i].func_addr);
        // printf("[Return] 0x%x\n", ft_record[i].func_addr);
        }
    }

    printf("\n [Ret info:] \n");
    for (int i = 0; i < ret_info_cnt; i++) {
        printf("Return %s at 0x%x\n", ret_info[i].name, ret_info[i].address);
    }
}
