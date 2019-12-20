//61940047 oyama yasuhiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "extern.h"

void call_help(int ac, char *av[]){
  if(ac == 1){
    help();
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}

void call_init(int ac, char *av[]){
  if(ac == 1){
    init();
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}

void call_print_buf(int ac, char *av[]){
  print_buf(ac, av);
}

void call_print_hash(int ac, char *av[]){
  print_hash(ac, av);
}

void call_print_free(int ac, char *av[]){
  if(ac == 1){
    print_free();
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}

void call_getblk(int ac, char *av[]){
  if(ac == 1){
    fprintf(stderr, "too few arguments\n");
  }else if(ac == 2){
    char *endp;
    int blkno = (int)strtol(av[1], &endp, 10);
    if(*endp == 0){
      getblk(blkno);
    }else{
      fprintf(stderr, "av[1]: give a number\n");
    }
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}

void call_brelse(int ac, char *av[]){
  if(ac == 1){
    fprintf(stderr, "too few arguments\n");
  }else if(ac == 2){
    char *endp;
    int blkno = (int)strtol(av[1], &endp, 10);
    if(*endp == 0){
      struct buf_header *p = search_hash(blkno);
      if(p != NULL){
        brelse(p);
      }
    }else{
      fprintf(stderr, "av[1]: give a number\n");
    }
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}

void call_set_stat(int ac, char *av[]){
  if(ac < 3){
    fprintf(stderr, "too few arguments\n");  
  }else if(ac > 8){
    fprintf(stderr, "too many arguments\n");  	  
  }else{
    char *endp;
    int blkno = (int)strtol(av[1], &endp, 10);
    if(*endp == 0){
      for(int i = 2; i < ac; i++){
	if(strlen(av[i]) != 1){
	  fprintf(stderr, "av[%d]: invalid argument\n", i);
	}else{
          for(int j = 0; j < 6; j++){
            if(*av[i] == stat_char[j]){
              set_stat(blkno, stat_bit[j]);
	      break;
            }
	    if(j == 5){      
	      fprintf(stderr, "av[%d]: invalid argument\n", i);
	    }
          }
	}
      }
    }else{
      fprintf(stderr, "av[%d]: give a number\n", 1);
    }
  }
}

void call_reset_stat(int ac, char *av[]){
  if(ac < 3){
    fprintf(stderr, "too few arguments\n");
  }else if(ac > 8){
    fprintf(stderr, "too many arguments\n");  
  }else{
    char *endp;
    int blkno = (int)strtol(av[1], &endp, 10);
    if(*endp == 0){
      for(int i = 2; i < ac; i++){
	if(strlen(av[i]) != 1){
	  fprintf(stderr, "av[%d]: invalid argument\n", i);
	}else{
          for(int j = 0; j < 6; j++){
            if(*av[i] == stat_char[j]){
              reset_stat(blkno, stat_bit[j]);
	      break;
            }
	    if(j == 5){
	      fprintf(stderr, "av[%d]: invalid argument\n", i);
	    }
          }
	}
      }
    }else{
      fprintf(stderr, "av[%d]: give a number\n", 1);
    }
  }
}

void call_quit(int ac, char *av[]){
  if(ac == 1){
    quit();
  }else{
    fprintf(stderr, "too many arguments\n");
  }
}
