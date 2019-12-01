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
#define PATHNUM 128
#define PATHLEN 128
#define PBUFLEN PATHNUM * PATHLEN

extern char **environ;

token gettoken(char *token, int len);
void redir(char *filename, int mode);
void getpaths(int *pc, char *pv[], int len, char *pbuf);
int existfile(int pc, char *pv[], char *filename);
void set_sigaction();

int main(){
  int count, pipec, pc, err, fd, pfd[10][2];
  char *path, *pv[PATHNUM], pbuf[PBUFLEN];
  token ret;

  path = getenv("PATH");
  if(strncpy(pbuf, path, sizeof(pbuf) - 1) < 0){
    perror("strncpy");
    exit(1);
  }
  pbuf[PBUFLEN - 1] = '\0';
  getpaths(&pc, pv, PATHNUM, pbuf);
  
  set_sigaction();

  for(;;){
    char cwd[CWDLEN];
    getcwd(cwd, CWDLEN);
    fprintf(stderr, "%s$ ", cwd);
    
    count = pipec = 0;

    for(;;){
      int ac;
      char buf[BUFLEN];
      char *av[ARGNUM];
      char filename[FILENUM][FILELEN];
      int redir_in, redir_out, redir_append;
      
      ac = err = 0;
      redir_in = redir_out = redir_append = 0;
      count++;
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
	    fprintf(stderr, "syntax error: no file name\n");
	    err = 1;
	    break;
	  }
        }else if(ret == TKN_REDIR_OUT){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_out = 1;
	  }else{
	    fprintf(stderr, "syntax error: no file name\n");
	    err = 1;
	    break;
	  }
	}else if(ret == TKN_REDIR_APPEND){
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_append = 1;
	  }else{
	    fprintf(stderr, "syntax error: no file name\n");
	    err = 1;
	    break;
	  }
	}else if(ret == TKN_PIPE || ret == TKN_EOL){
	  av[ac] = NULL;
          break;
        }
      }
      
      if(err){
        break;
      }
      if(ret == TKN_PIPE){
	if(av[0] == NULL){
	  fprintf(stderr, "syntax error: no command\n");
	  err = 1;
	  break;
	}
	pipe(pfd[pipec]);
	if((fd = fork()) == 0){
	  if(pipec){
	    close(0);
	    dup(pfd[pipec - 1][0]);
	  }
	  close(1);
	  dup(pfd[pipec][1]);
	  for(int i = 0; i < pipec + 1; i++){
	    close(pfd[i][0]);
	    close(pfd[i][1]);
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
	    fprintf(stderr, "no such command\n");
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
	  pipec++;
	}
      }else if(ret == TKN_EOL){
	if(av[0] == NULL && pipec){
	  fprintf(stderr, "> ");
	  continue;
	}else if(av[0] == NULL){
	  fprintf(stderr, "syntax error: no command\n");
	  err = 1;
	  break;
	}
	if(strcmp(av[0], "exit") == 0){
	  exit(0);
	}else if(strcmp(av[0], "cd") == 0){
	  chdir(av[1]);
	  break;
	}else{
	  if((fd = fork()) == 0){
            if(pipec){
	      close(0);
	      dup(pfd[pipec-1][0]);
	      for(int i = 0; i < pipec; i++){
	        close(pfd[i][0]);
		close(pfd[i][1]);
	      }
	    }else{
	      if(redir_in){
	        redir(filename[0], 0);
	      }
	    }
	    if(redir_out){
	      redir(filename[1], 1);
	    }else if(redir_append){
	      redir(filename[1], 2);
	    }

	    int i;
	    if((i = existfile(pc, pv, av[0])) < 0){
	      fprintf(stderr, "no such command\n");
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

    if(pipec){
      for(int i = 0; i < pipec; i++){
        close(pfd[i][0]);
	close(pfd[i][1]);
      }
    }
    if(err){
      char c;
      if(ret != TKN_EOL){
        char tmp[BUFLEN];
        if(fgets(tmp, BUFLEN, stdin) < 0){
          fprintf(stderr, "failed to read remaining characters in stdin\n");
        }
      }
      count -= 1;
    }
    if(count){
      int status[count];
      for(int i = 0; i < count; i++){
        wait(&status[i]);
      }
    }
  }
}
