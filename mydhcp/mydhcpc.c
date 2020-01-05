#include <stdio.h>
#include <stdlib.h>
#include "mydhcp.h"
#include "mydhcpc.h"

int main(){
  struct proctable ptab[] = {
    
  };
  int s;
  struct sockaddr_in myskt, to;

  if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  } 
}
