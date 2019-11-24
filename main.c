//61940047 oyama yasuhiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void getargs(int*, char *[], char *);

int main(){
  int ac;
  int fd;
  int status;
  char *av[16];
  char buf[256];

  for(;;){
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    fprintf(stderr, "%s$ ", cwd);

    if(fgets(buf, sizeof(buf), stdin) == NULL){
      if(ferror(stdin)){
        perror("fgets");
	exit(1);
      }
      exit(0);
    }
    buf[strlen(buf) - 1] = '\0';
    
    memset(av, 0, sizeof(av));
    getargs(&ac, av, buf);

    if(strcmp(av[0], "exit") == 0){
      exit(0);
    }else if(strcmp(av[0], "cd") == 0){
      chdir(av[1]);
    }else{
      if((fd = fork()) == 0){
        execvp(av[0], av);
      }else if(fd > 0){
        wait(&status);
      }
    }
  }
}
