#include <string.h>
#include <ctype.h>

void getargs(int *ac, char *av[], char *buf){
  *ac = 0;
  char *p = buf;

  for(;;){
    while(isblank(*p)){
      p++;
    }
    if(*p == '\0'){
      return;
    }
    av[(*ac)++] = p;
    while(*p && !isblank(*p)){
      p++;
    }
    if(*p == '\0'){
      return;
    }
    *p++ = '\0';
  } 
}
