#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "mydhcp.h"
//#include "mydhcpc.h"

/*struct proctable ptab[] = {

}*/

int main(int ac, char *av[]){
  int s;
  char *host;
  in_port_t port;
  struct sockaddr_in myskt, to;
  struct dhcph send;
  
  if(ac != 2){
    fprintf(stderr, "give a host name\n");
    exit(1);
  }
  
  host = av[1];
  port = 51230;
  
  memset(&send, 0, sizeof(send));
  send.type = DISCOVER;

  if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }

  to.sin_family = AF_INET;
  inet_aton(host, &to.sin_addr);
  to.sin_port = htons(port);
  
  if(sendto(s, &send, sizeof(send), 0, (struct sockaddr *)&to, sizeof(to)) < 0){
    perror("sendto");
    exit(1);
  }
  /*
  for(;;){
    event = wait_event();
    for(pt = ptab; pt->status; pt++){
      if(pt->status == status && pt->event == event){
        (*pt->func)();
	break;
      }
    }
    if(pt->status == 0){
      printf();
    }
  }*/
}
