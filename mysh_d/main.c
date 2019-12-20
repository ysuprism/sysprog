//61940047 oyama yasuhiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "token.h"

#define CWDLEN 256
#define BUFLEN 512
#define ARGNUM 16
#define ARGLEN BUFLEN / ARGNUM
#define FILELEN 64
#define PATHNUM 128
#define PATHLEN 128
#define PBUFLEN PATHNUM * PATHLEN
#define MAXPIPE 20

extern char **environ;

token gettoken(char *token, int len);
void redir(char *filename, int mode);
void getpaths(int *pc, char *pv[], int len, char *pbuf);
int existfile(int pc, char *pv[], char *filename);
void set_sigaction();
void setup_term();

int main(){
  int pc, fd, fd2, pfd[MAXPIPE][2];
  char *path, *pv[PATHNUM], pbuf[PBUFLEN];

  path = getenv("PATH");
  if(strncpy(pbuf, path, PBUFLEN - 1) < 0){
    perror("strncpy");
    exit(1);
  }
  pbuf[PBUFLEN - 1] = '\0';
  getpaths(&pc, pv, PATHNUM, pbuf);
  
  set_sigaction();
  setup_term();

  if((fd2 = open("/dev/tty", O_RDWR)) < 0){
    perror("open");
    exit(1);
  }

  for(;;){
    int pipec, err, bg;
    char cwd[CWDLEN];
    pid_t pgid, fgid;
    token ret;

    pipec = bg = pgid = 0;
    memset(cwd, 0, CWDLEN);
    
    getcwd(cwd, CWDLEN);
    fprintf(stderr, "%s$ ", cwd);
    
    while(pipec < MAXPIPE + 1){
      int ac, redir_in, redir_out, redir_append;
      char buf[BUFLEN], *av[ARGNUM], filename[2][FILELEN];
      
      ac = err = redir_in = redir_out = redir_append = 0;
      memset(buf, 0, BUFLEN);
      for(int i = 0; i < ARGNUM; i++){
        av[i] = &buf[ARGLEN * i];
      }

      while(ac < ARGNUM){
        ret = gettoken(av[ac], ARGLEN);

        if(ret == TKN_NORMAL){
          ac++;
        }

	if(ret == TKN_REDIR_IN){
	  memset(filename[0], 0, FILELEN);
          ret = gettoken(filename[0], FILELEN);
	  if(ret == TKN_NORMAL){
	    redir_in = 1;
	  }else{
	    redir_in = 0;
	    err = 1;
	    fprintf(stderr, "syntax error: unexpected token\n");
	    break;
	  }
        }

	if(ret == TKN_REDIR_OUT){
	  memset(filename[1], 0, FILELEN);
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    if(redir_append){
	      redir_append = 0;
	    }
	    redir_out = 1;
	  }else{
	    redir_out = 0;
	    err = 1;
	    fprintf(stderr, "syntax error: unexpected token\n");
	    break;
	  }
	}
	
	if(ret == TKN_REDIR_APPEND){
	  memset(filename[1], 0, FILELEN);
	  ret = gettoken(filename[1], FILELEN);
	  if(ret == TKN_NORMAL){
	    if(redir_out){
	      redir_out = 0;
	    }
	    redir_append = 1;
	  }else{
	    redir_append = 0;
	    err = 1;
	    fprintf(stderr, "syntax error: unexpected token\n");
	    break;
	  }
	}
	
	if(ret == TKN_BG){
	  ret = gettoken(av[ac], ARGLEN);
	  if(ret == TKN_EOL){
	    bg = 1;
	  }else{
	    err = 1;
	    fprintf(stderr, "syntax error: unexpected token\n");
	    break;
	  }
	}

	if(ret == TKN_PIPE || ret == TKN_EOL){
	  if(ret == TKN_PIPE && pipec == MAXPIPE){
	    err = 1;
	    fprintf(stderr, "error: too many pipes\n");
	  }
	  av[ac] = NULL;
          break;
        }
      }
      
      if(err){
        break;
      }

      if(ret == TKN_PIPE){
	if(av[0] == NULL){
	  err = 1;
	  fprintf(stderr, "syntax error: no command\n");
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
	  }

	  if(redir_append){
	    redir(filename[1], 2);
	  }

	  int i;
	  if((i = existfile(pc, pv, av[0])) < 0){
	    fprintf(stderr, "%s: no such command\n", av[0]);
	    exit(1);
	  }

	  char str[512];
	  memset(str, 0, sizeof(str));

	  strncpy(str, pv[i], sizeof(str) - 2);
	  str[strlen(str)] = '/';
	  strncat(str, av[0], sizeof(str) - strlen(str) - 1);

	  signal(SIGINT, SIG_DFL);

          if(execve(str, av, environ) < 0){
	    perror("execve");
	    exit(1);
	  }
	}
	if(fd > 0){
	  if(!pipec){
	    pgid = fd;
	  }

	  if(setpgid(fd, pgid) < 0){
	    perror("setpgid");
	  }

	  pipec++;
	}
      }
      
      if(ret == TKN_EOL){
	if(av[0] == NULL && pipec){
	  fprintf(stderr, "> ");
	  continue;
	}else if(av[0] == NULL){
	  err = 1;
	  fprintf(stderr, "syntax error: no command\n");
	  break;
	}

	if(strcmp(av[0], "exit") == 0){
	  close(fd2);
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
	    }

	    if(redir_in){
	      redir(filename[0], 0);
	    }

	    if(redir_out){
	      redir(filename[1], 1);
	    }
	    
	    if(redir_append){
	      redir(filename[1], 2);
	    }

	    int i;
	    if((i = existfile(pc, pv, av[0])) < 0){
	      fprintf(stderr, "%s: no such command\n", av[0]);
	      exit(1);
	    }

	    char str[512];
	    memset(str, 0, sizeof(str));

	    strncpy(str, pv[i], sizeof(str) - 2);
	    str[strlen(str)] = '/';
	    strncat(str, av[0], sizeof(str) - strlen(str) - 1);
            
	    signal(SIGINT, SIG_DFL);

	    if(execve(str, av, environ) < 0){
	      perror("execve");
	      exit(1);
	    }
	  }

	  if(fd > 0){
	    if(pgid == 0){
	      pgid = fd;
	    }
	    if(setpgid(fd, pgid) < 0){
	      perror("setpgid");
	    }

            fgid = bg ? getpgrp() : pgid;
	    if(tcsetpgrp(fd2, fgid) < 0){
	      perror("tcsetpgrp");
	    }

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

    if(ret != TKN_EOL){
      char tmp[BUFLEN];
      if(fgets(tmp, BUFLEN, stdin) < 0){
        fprintf(stderr, "failed to read remaining characters in stdin\n");
      }
    }

    if(!bg){
      int j = err ? pipec : pipec + 1;
      int status[j];

      for(int i = 0; i < j; i++){
        wait(&status[i]);
      }

      fgid = getpgrp();
      if(tcsetpgrp(fd2, fgid) < 0){
        perror("tcsetpgrp");
      }
    }
  }
}
