#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "token.h"

int gettoken(char *token, int len){
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
