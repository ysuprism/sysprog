//61940047 oyama yasuhiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "token.h"

#define CWDLEN 256
#define BUFLEN 512
#define ARGNUM 16
#define ARGLEN 32
#define FILENUM 2
#define FILELEN 64
#define PATHNUM 256

extern char **environ;

token gettoken(char *token, int len);
void redir(char *filename, int mode);
void getpaths(int *pc, char *pv[], int len, char *path);
int existfile(int pc, char *pv[], char *filename);
void set_sigaction();

int main(){
  int before, err, count, pc, len, fd, pfd[2];
  char *path, *pv[PATHNUM];

  len = sizeof(pv) / sizeof(pv[0]);
  path = getenv("PATH");
  getpaths(&pc, pv, len, path);
  
  set_sigaction();

  for(;;){
    char cwd[CWDLEN];
    getcwd(cwd, CWDLEN);
    fprintf(stderr, "%s$ ", cwd);
    
    before = count = 0;

    for(;;){
      int ac;
      char buf[BUFLEN];
      char *av[ARGNUM];
      char filename[FILENUM][FILELEN];
      int redir_in, redir_out, redir_append;
      token ret;
      
      ac = err = 0;
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
	    fprintf(stderr, "invalid file name\n");
	    err = 1;
	    break;
	  }
        }else if(ret == TKN_REDIR_OUT){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_out = 1;
	  }else{
	    fprintf(stderr, "invalid file name\n");
	    err = 1;
	    break;
	  }
	}else if(ret == TKN_REDIR_APPEND){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_append = 1;
	  }else{
	    fprintf(stderr, "invalid file name\n");
	    err = 1;
	    break;
	  }
	}else if(ret == TKN_PIPE || ret == TKN_EOL){
	  av[ac] = NULL;
          break;
        }
      }
      
      if(err){
	char tmp[BUFLEN];
        fgets(tmp, BUFLEN, stdin);	
        break;
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
          
	  int i;
	  if((i = existfile(pc, pv, av[0])) < 0){
	    fprintf(stderr, "no such a command\n");
	    exit(1);
	  }
          
	  char str[512];
	  memset(str, 0, sizeof(str));
	  strncpy(str, pv[i], sizeof(str) - 2);
	  str[strlen(str)] = '/';
	  strncat(str, av[0], sizeof(str) - strlen(str) - 1);
          if(execve(str, av, environ) < 0){
	    perror("execve");
	    exit(1);
	  }
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
	      close(pfd[0]);
	      close(pfd[1]);
	    }
	    if(redir_in){
	      redir(filename[0], 0);
	    }
	    if(redir_out){
	      redir(filename[1], 1);
	    }else if(redir_append){
	      redir(filename[1], 2);
	    }

	    int i;
	    if((i = existfile(pc, pv, av[0])) < 0){
	      fprintf(stderr, "no such a command\n");
	      exit(1);
	    }

	    char str[512];
	    memset(str, 0, sizeof(str));
	    strncpy(str, pv[i], sizeof(str) - 2);
	    str[strlen(str)] = '/';
	    strncat(str, av[0], sizeof(str) - strlen(str) - 1);
	    if(execve(str, av, environ) < 0){
	      perror("execve");
	      exit(1);
	    }
	  }
	  if(fd > 0){
	    break;
	  }
	}
      }
    }

    if(before){
      close(pfd[0]);
      close(pfd[1]);
    }
    if(err){
      count -= 1;
    }
    if(count != -1){
      int status[count+1];
      for(int i = 0; i < count + 1; i++){
        wait(&status[i]);
      }
    }
  }
}
