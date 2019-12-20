//61940047 oyama yasuhiro

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "buf.h"
#include "command.h"

struct command_table{
  char *cmd;
  void (*func)(int, char *[]);
};

void getargs(int *, char *[]);

struct buf_header hash_head[NHASH] = {};
struct buf_header free_head = {};
struct buf_header buf[12] = {};
const unsigned int stat_bit[6] = {
	STAT_LOCKED, STAT_VALID, STAT_DWR, 
	STAT_KRDWR, STAT_WAITED, STAT_OLD
};
static struct command_table cmd_tbl[] = {
	{"help", call_help},
        {"init", call_init},
        {"buf", call_print_buf},
        {"hash", call_print_hash},
        {"free", call_print_free},
        {"getblk", call_getblk},
        {"brelse", call_brelse},
        {"set", call_set_stat},
        {"reset", call_reset_stat},
        {"quit", call_quit},
	{NULL, NULL}
};

int main(){
  struct command_table *p;
  int ac;
  char *av[16];

  for(int i = 0; i < 16; i++){
    av[i] = (char *)malloc(sizeof(char) * 32);
  }

  init();

  for(;;){
    printf("input a command: ");
    getargs(&ac, av);
    for(p = cmd_tbl; p->cmd; p++){
      if(strcmp(av[0], p->cmd) == 0){
        (*p->func)(ac, av);
	break;
      }
    }
    if(p->cmd == NULL){
      fprintf(stderr, "unknown command\n");
    }
  }
}

void getargs(int *ac, char *av[]){
  char buf[256];
  char *p = buf;
  memset(buf, 0, 256);
  *ac = 0;
  if(fgets(buf, 255, stdin) != NULL){
    while(*p != 0){
      if(*ac == 16){
        break;
      }
      if(*p == 9 || *p == 10 || *p == 32){
        p++;
      }else{
        sscanf(p, "%s", av[*ac]);
        p += strlen(av[(*ac)++]);
      }
    }
  }
}
