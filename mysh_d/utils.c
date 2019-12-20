#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include "token.h"

token gettoken(char *token, int len){
  int i;
  char c, *p;

  c = getchar();
  p = token;

  while(c == ' ' || c == '\t'){
    c = getchar();
  }

  switch(c){
    case EOF:
	    return TKN_EOF;
    case '\n':
	    return TKN_EOL;
    case '&':
	    return TKN_BG;
    case '|':
	    return TKN_PIPE;
    case '<':
	    return TKN_REDIR_IN;
    case '>':
	    if((c = getchar()) == '>'){
	      return TKN_REDIR_APPEND;
	    }
	    ungetc(c, stdin);
	    return TKN_REDIR_OUT;
    default:
	    break;
  }

  ungetc(c, stdin);

  for(i = 0; i < len - 1; i++){
    c = getchar();
    if(c != EOF && c != '\n' && c != '&' && c != '<' && 
       c != '>' && c != '|' && !isblank(c)){
      *p++ = c;
    }else{
      break;
    }
  }

  ungetc(c, stdin);

  *p = '\0';
  return TKN_NORMAL;
}

void redir(char *filename, int mode){
  int fd;

  if(mode == 0){
    if((fd = open(filename, O_RDONLY)) < 0){
      perror(filename);
      return;
    }
    close(0);
    dup(fd);
    close(fd);
  }else if(mode == 1){
    if((fd = open(filename, O_WRONLY|O_CREAT, 0644)) < 0){
      perror(filename);
      return;
    } 
    close(1);
    dup(fd);
    close(fd);
  }else if(mode == 2){
    if((fd = open(filename, O_WRONLY|O_APPEND)) < 0){
      perror(filename);
      return;
    }
    close(1);
    dup(fd);
    close(fd);
  }
}

void getpaths(int *pc, char *pv[], int len, char *pbuf){
  *pc = 0;

  for(;;){
    if(*pbuf == '\0'){
      return;
    }
    if(*pc == len){
      return;
    }
    pv[(*pc)++] = pbuf;
    while(*pbuf && *pbuf != ':'){
      pbuf++;
    }
    if(*pbuf == '\0'){
      return;
    }
    *pbuf++ = '\0';
  } 
}

int existfile(int pc, char *pv[], char *filename){
  for(int i = 0; i < pc; i++){
    char str[512];
    FILE *fp;
    memset(str, 0, sizeof(str));
    strncpy(str, pv[i], sizeof(str) - 2);
    str[strlen(str)] = '/';
    strncat(str, filename, sizeof(str) - strlen(str) - 1);
    if((fp = fopen(str, "r")) != NULL){
      fclose(fp);
      return i;
    } 
  }
  return -1;
}

void sigchld_handler(){
  pid_t child_pid = 0;

  do{
    int status;
    child_pid = waitpid(-1, &status, WNOHANG);
  }while(child_pid > 0);
}

void set_sigaction(){
  struct sigaction act, act2;

  memset(&act, 0, sizeof(act));
  sigemptyset(&act.sa_mask);
  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  act.sa_flags |= SA_RESTART;

  memset(&act2, 0, sizeof(act2));
  sigemptyset(&act2.sa_mask);
  act2.sa_handler = sigchld_handler;
  act2.sa_flags = 0;
  act2.sa_flags |= SA_RESTART;

  if(sigaction(SIGINT, &act, NULL) < 0){
    perror("sigaction");
    exit(1);
  }
  
  if(sigaction(SIGTTOU, &act, NULL) < 0){
    perror("sigaction");
    exit(1);
  }

  if(sigaction(SIGCHLD, &act2, NULL) < 0){
    perror("sigaction");
    exit(1);
  }
}

void setup_term(){
  struct termios t;
  tcgetattr(0, &t);
  t.c_lflag &= ~ECHOCTL;
  tcsetattr(0, TCSANOW, &t);
}
