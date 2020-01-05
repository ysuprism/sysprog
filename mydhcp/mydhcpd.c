#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mydhcp.h"
#include "mydhcpd.h"

void init_client(struct client *);
void insert_client(struct client *, struct client *);
void remove_client(struct client *);
struct client *search_client(struct client *, struct sockaddr_in *);
struct client *create_client(struct client *, struct sockaddr_in *);
void init_alloc(struct alloc *);
void insert_alloc(struct alloc *, struct alloc *);
struct alloc *remove_alloc(struct alloc *);
int get_rownum(char *);
void init_alloclist(struct alloc *, struct alloc [], int, char *);
dhcph_t get_dhcph_type(struct dhcph *);
event_t wait_event(int, state_t *, fd_set *, struct timeval *, struct dhcph *,
		struct sockaddr_in *, struct client *);
void alloc_ip(struct client *, struct alloc *);
void recall_ip(struct client *, struct alloc *);
void f_act1(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);
void f_act2(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);
void f_act3(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);
void f_act4(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);
void f_act5(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);
void f_act6(int, uint16_t, struct client *, struct alloc *, 
		struct sockaddr_in *);

int main(int ac, char *av[]){
  if(ac != 2){
    fprintf(stderr, "give a config file\n");
    exit(1);
  }

  struct proctable ptab[] = {
	  {INIT, RECV_DISCOVER, f_act1},
	  {WAIT_REQ, RECV_REQ_ALLOC_OK, f_act2},
	  {WAIT_REQ, RECV_REQ_ALLOC_NG, f_act4},
	  {WAIT_REQ, RECV_TIMEOUT, f_act3},
	  {RESEND, RECV_REQ_ALLOC_OK, f_act2},
	  {RESEND, RECV_REQ_ALLOC_NG, f_act4},
	  {RESEND, RECV_TIMEOUT, f_act4},
	  {IN_USE, RECV_REQ_EXT_ALLOC_OK, f_act5},
	  {IN_USE, RECV_REQ_EXT_ALLOC_NG, f_act4},
	  {IN_USE, RECV_RELEASE, f_act4},
	  {IN_USE, TTL_TIMEOUT, f_act4}
	  {0, 0, NULL}
  };
  struct proctable *pt;
  struct client clientlist;
  event_t event;
  state_t state;
  int s, len;
  char *filename;
  in_port_t myport = 51230;
  uint16_t ttl = 20;
  struct sockaddr_in myskt, from;
  struct dhcph recv = {0};
  struct timeval timeout;
  fd_set rdfds;
  
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  filename = av[1];
  len = get_rownum(filename) - 1;
  struct alloc alloclist, list[len];
  init_alloclist(&alloclist, list, len, filename);

  if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }
  
  memset(&myskt, 0, sizeof(myskt));
  myskt.sin_family = AF_INET;
  myskt.sin_port = htons(myport);
  myskt.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(s, (struct sockaddr *)&myskt, sizeof(myskt)) < 0){
    perror("bind");
    exit(1);
  }

  for(;;){
    event = wait_event(s, &state, &rdfds, &timeout, &recv, sizeof(recv));
    for(pt = ptab; pt->state; pt++){
      if(pt->state == state && pt->event == event){
        (*pt->func)(s);
	break;
      }
    }
    if(pt->state == 0){
      fprintf(stderr, "unknown message\n");
      continue;
    }
  }
}
