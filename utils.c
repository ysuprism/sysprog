#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "token.h"

token gettoken(char *token, int len){
  int i;
  char c, *p;

  c = getchar();
  p = token;

  while(c == ' '){
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
    fd = open(filename, O_RDONLY);
    close(0);
    dup(fd);
    close(fd);
  }else if(mode == 1){
    fd = open(filename, O_WRONLY|O_CREAT, 0644);  
    close(1);
    dup(fd);
    close(fd);
  }else if(mode == 2){
    fd = open(filename, O_WRONLY|O_APPEND);
    close(1);
    dup(fd);
    close(fd);
  }
}

void getpaths(int *pc, char *pv[], int len, char *path){
  *pc = 0;

  for(;;){
    if(*path == '\0'){
      return;
    }
    if(*pc == len){
      return;
    }
    pv[(*pc)++] = path;
    while(*path && *path != ':'){
      path++;
    }
    if(*path == '\0'){
      return;
    }
    *path++ = '\0';
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
