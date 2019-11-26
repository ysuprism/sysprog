//61940047 oyama yasuhiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "token.h"

#define CWDLEN 256
#define BUFLEN 512
#define ARGNUM 16
#define ARGLEN 32
#define FILENUM 2
#define FILELEN 64

int gettoken(char *token, int len);
void redir(char *filename, int mode);

int main(){
  int before, count, fd, pfd[2];

  for(;;){
    char cwd[CWDLEN];
    getcwd(cwd, CWDLEN);
    fprintf(stderr, "%s$ ", cwd);
    
    before = count = 0;

    for(;;){
      int ac = 0;
      char buf[BUFLEN];
      char *av[ARGNUM];
      char filename[FILENUM][FILELEN];
      int redir_in, redir_out, redir_append;
      token ret;

      redir_in = redir_out = redir_append = 0;
      for(int i = 0; i < ARGNUM; i++){
        av[i] = &buf[32*i];
      }

      while(ac < ARGNUM){
        ret = gettoken(av[ac], ARGLEN);
        if(ret == TKN_NORMAL){
          ac++;
        }else if(ret == TKN_REDIR_IN){
	  ret = gettoken(filename[0], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_in = 1;
	  }else{
	    fprintf(stderr, "give a filename");
	  }
        }else if(ret == TKN_REDIR_OUT){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_out = 1;
	  }else{
	    fprintf(stderr, "give a filename");
	  }
	}else if(ret == TKN_REDIR_APPEND){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_append = 1;
	  }else{
	    fprintf(stderr, "give a filename");
	  }
	}else if(ret == TKN_PIPE || ret == TKN_EOL){
	  av[ac] = NULL;
          break;
        }
      }

      if(ret == TKN_PIPE){
	if(!before){
	  pipe(pfd);
	}
	if((fd = fork()) == 0){
	  if(before){
	    close(0);
	    dup(pfd[0]);
	  }
	  close(1);
	  dup(pfd[1]);
	  close(pfd[0]);
	  close(pfd[1]);
          execvp(av[0], av);
	}
	if(fd > 0){
	  before = 1;
	  count++;
	}
      }else if(ret == TKN_EOL){
	if(strcmp(av[0], "exit") == 0){
	  exit(0);
	}else if(strcmp(av[0], "cd") == 0){
	  chdir(av[1]);
	  break;
	}else{
	  if((fd = fork()) == 0){
            if(before){
	      close(0);
	      dup(pfd[0]);
	    }
	    close(pfd[0]);
	    close(pfd[1]);
	    if(redir_in){
	      redir(filename[0], 0);
	    }
	    if(redir_out){
	      redir(filename[1], 1);
	    }else if(redir_append){
	      redir(filename[1], 2);
	    }
	    execvp(av[0], av);
	  }
	  if(fd > 0){
	    if(before){
	      close(pfd[0]);
	      close(pfd[1]);
	    }
	    int status[count+1];
	    for(int i = 0; i < count + 1; i++){
	      wait(&status[i]);
	    }
	    break;
	  }
	}
      }
    }
  }
}
