#include <common.h>

extern FILE *ftrace_fp;
extern char *elf_file; 
extern bool ftrace_enable; 

typedef struct {
  char* name;
  paddr_t address;
} FunctionInfo;

typedef struct {
  char* name;
  paddr_t address;
} RetInfo;

enum ftrace_tye {ret, call};

typedef struct {
  paddr_t pc_addr;
  paddr_t func_addr;
  enum ftrace_tye type;
  size_t tab;
} Ftrace_record;

extern FunctionInfo* function_info;
extern size_t func_info_cnt;

extern RetInfo* ret_info;
extern size_t ret_info_cnt;

extern Ftrace_record* ft_record;
extern size_t ft_tab;
extern size_t ft_cnt;
extern char current_func[BUF_SIZE];

bool isfunc(paddr_t func_addr);
bool isret(paddr_t ret_addr);
char* get_func_name(paddr_t func_addr);
char* get_ret_name(paddr_t ret_addr);
void add_ret_info(paddr_t addr, char* name);

void init_ftrace(const char *ftrace_file);
void ftrace(int type, vaddr_t pc_addr, vaddr_t func_addr);
void ftrace_output();

#ifdef CONFIG_MTRACE
extern FILE *mtrace_fp;
void init_mtrace();
bool mtrace_addr_enable(paddr_t addr);

#define mtarce_write(addr, ...) IFDEF(CONFIG_TARGET_NATIVE_ELF, \
  do { \
    if (mtrace_addr_enable(addr)) { \
      fprintf(mtrace_fp, __VA_ARGS__); \
      fflush(mtrace_fp); \
    } \
  } while (0) \
)
#endif

#ifdef CONFIG_DTRACE
extern FILE *dtrace_fp;
void init_dtrace();
#endif

#ifdef CONFIG_ETRACE
extern FILE *etrace_fp;
void init_etrace();
#endif

#ifdef CONFIG_TRACE
void init_trace();
#endif