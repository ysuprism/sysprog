#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include "mydhcp.h"
#include "mydhcpd.h"

void insert_client(struct client *, struct client *);
void remove_client(struct client *);
struct client *pop_client(struct client *);
struct client *search_client(struct client *, struct sockaddr_in *);
struct client *create_client(struct client *, struct client *, 
		struct sockaddr_in *);
void delete_client(struct client *, struct client *);
void init_clientlist(struct client *, struct client *, struct client [], int);
void insert_alloc(struct alloc *, struct alloc *);
void remove_alloc(struct alloc *);
struct alloc *pop_alloc(struct alloc *);
void init_alloclist(struct alloc *, struct alloc *, struct alloc [],
	       	int, int *, char *);
dhcph_t get_dhcph_type(struct dhcph *);
event_t wait_event(int, state_t *, fd_set *, struct timeval *, 
		struct dhcph *, struct client *, struct sockaddr_in *);
void alloc_ip(struct client *, struct alloc *);
void recall_ip(struct client *, struct alloc *);
void f_act1(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);
void f_act2(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);
void f_act3(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);
void f_act4(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);
void f_act5(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);
void f_act6(int, state_t *, uint16_t, struct client *, struct client *, 
		struct alloc *, struct alloc *, struct sockaddr_in *);

struct client chead, *resend;

struct proctable ptab[] = {
	{INIT, RECV_DISCOVER, f_act1},
        {WAIT_REQ, RECV_REQ_ALLOC_OK, f_act2},
        {WAIT_REQ, RECV_REQ_ALLOC_NG, f_act6},
        {WAIT_REQ, RECV_TIMEOUT, f_act5},
        {RESEND, RECV_REQ_ALLOC_OK, f_act2},
        {RESEND, RECV_REQ_ALLOC_NG, f_act6},
        {RESEND, RECV_TIMEOUT, f_act6},
        {IN_USE, RECV_REQ_EXT_ALLOC_OK, f_act3},
        {IN_USE, RECV_REQ_EXT_ALLOC_NG, f_act6},
        {IN_USE, RECV_RELEASE, f_act4},
        {IN_USE, TTL_TIMEOUT, f_act4},
        {0, 0, NULL}
};

void sigvtalrm_handler(int sig){
  struct client *p;

  if(chead.fp != &chead){
    for(p = chead.fp; p != &chead; p = p->fp){
      if(p->state == IN_USE && p->ttlcounter > 0){
        p->ttlcounter--;
      }
    }
  }  
}

int main(int ac, char *av[]){
  struct proctable *pt;
  struct client cfhead, clist[100];
  struct alloc ahead, afhead, alist[100];
  event_t event;
  state_t state;
  char *filename;
  int s;
  in_port_t myport;
  int ttl;
  struct sockaddr_in myskt, from;
  struct dhcph recv;
  struct timeval timeout;
  struct sigaction act;
  fd_set rdfds;
  
  if(ac != 2){
    fprintf(stderr, "give a config file\n");
    exit(1);
  }

  if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }
  
  filename = av[1];
  myport = 51230;
  ttl = 20;
  resend = NULL;

  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  memset(&myskt, 0, sizeof(myskt));
  myskt.sin_family = AF_INET;
  myskt.sin_port = htons(myport);
  myskt.sin_addr.s_addr = htonl(INADDR_ANY);

  memset(&act, 0, sizeof(act));
  sigemptyset(&act.sa_mask);
  act.sa_handler = sigvtalrm_handler;
  act.sa_flags = 0;
  act.sa_flags |= SA_RESTART;

  init_clientlist(&chead, &cfhead, clist, 100);
  init_alloclist(&ahead, &afhead, alist, 100, &ttl, filename);

  if(bind(s, (struct sockaddr *)&myskt, sizeof(myskt)) < 0){
    perror("bind");
    exit(1);
  }

  if(sigaction(SIGVTALRM, &act, NULL) < 0){
    perror("sigaction");
    exit(1);
  }

  for(;;){
    printf("-- wait for event --\n");
    event = wait_event(s, &state, &rdfds, &timeout, &recv, &chead, &from);
    for(pt = ptab; pt->state; pt++){
      if(pt->state == state && pt->event == event){
        (*pt->func)(s, &state, ttl, &chead, 
			&cfhead, &ahead, &afhead, &from);
	break;
      }
    }
    if(pt->state == 0){
      fprintf(stderr, "unknown message\n");
      continue;
    }
  }
}
