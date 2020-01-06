#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "mydhcp.h"
#include "mydhcpd.h"

extern struct client *resend;

void insert_client(struct client *head, struct client *t){
  t->fp = head;
  t->bp = head->bp;
  head->bp->fp = t;
  head->bp = t;
}

void remove_client(struct client *t){
  if(t->fp != t){
    t->fp->bp = t->bp;
    t->bp->fp = t->fp;
    t->fp = t->bp = NULL;
  }
}

struct client *pop_client(struct client *cfhead){
  struct client *p;

  if(cfhead->fp != cfhead){
    p = cfhead->fp;
    p->fp->bp = cfhead;
    cfhead->fp = p->fp;
    p->fp = p->bp = NULL;
    return p;
  }

  return NULL;
}

struct client *search_client(struct client *chead, struct sockaddr_in *from){
  struct client *p;

  if(chead->fp != NULL){
    for(p = chead->fp; p != chead; p = p->fp){
      if(p->ip == from->sin_addr.s_addr && p->port == from->sin_port){
        return p;
      }
    }
  }

  return NULL;
}

struct client *create_client(struct client *chead, struct client *cfhead, 
		struct sockaddr_in *from){
  struct client *new;

  new = pop_client(cfhead);
  new->ip = from->sin_addr.s_addr;
  new->port = from->sin_port;
  insert_client(chead, new);

  return new;
}

void delete_client(struct client *cfhead, struct client *t){
  remove_client(t);
  memset(t, 0, sizeof(struct client));
  insert_client(cfhead, t);
}

void init_clientlist(struct client *chead, struct client *cfhead, 
		struct client list[], int len){
  chead->fp = chead->bp = chead;
  cfhead->fp = cfhead->bp = cfhead;

  for(int i = 0; i < len; i++){
    memset(&list[i], 0, sizeof(struct client));
    insert_client(cfhead, &list[i]);
  }
}

void insert_alloc(struct alloc *head, struct alloc *t){
  t->fp = head;
  t->bp = head->bp;
  head->bp->fp = t;
  head->bp = t;
}

struct alloc *pop_alloc(struct alloc *afhead){
  struct alloc *p;

  if(afhead->fp != afhead){
    p = afhead->fp;
    p->fp->bp = afhead;
    afhead->fp = p->fp;
    p->fp = p->bp = NULL;
    return p;
  }

  return NULL;
}

void init_alloclist(struct alloc *ahead, struct alloc *afhead, 
		struct alloc list[], int len, int *ttl, char *filename){
  FILE *fp;
  char addr[128], netmask[128];

  if((fp = fopen(filename, "r")) == NULL){
    perror("fopen");
    exit(1);
  }

  fscanf(fp, "%d", ttl);

  ahead->fp = ahead->bp = ahead;
  afhead->fp = afhead->bp = afhead;

  for(int i = 0; i < len; i++){
    fscanf(fp, "%s%s", addr, netmask);
    inet_aton(addr, &list[i].addr);
    inet_aton(netmask, &list[i].netmask);
    insert_alloc(afhead, &list[i]);
  }

  fclose(fp);
}

dhcph_t get_dhcph_type(struct dhcph *recv){
  if(recv->type == 1){
    return DISCOVER;
  }else if(recv->type == 3 && recv->code == 2){
    return REQ_ALLOC;
  }else if(recv->type == 3 && recv->code == 3){
    return REQ_EXT_ALLOC;
  }else if(recv->type == 5){
    return RELEASE;
  }else{
    return UNKNOWN;
  }
}

struct client *check_resend(struct client *chead, long current){
  struct client *p;
  long delay, last;
  
  if(chead->fp == chead){
    return NULL;
  }

  for(p = chead->fp; p != chead; p = p->fp){
    last = p->last.tv_sec;
    delay = current - last;
    if(delay >= 10){
      if(p->state == WAIT_REQ || p->state == WAIT_RESEND){
        return p;
      }
    } 
  }

  return NULL;
} 

event_t wait_event(int s, state_t *state, fd_set *rdfds, 
		struct dhcph *recv, struct client *chead, 
		struct sockaddr_in *from){
  int ret, fromlen;
  dhcph_t type;
  struct client *cp;
  struct timeval current, timeout;
  
  FD_ZERO(rdfds);
  FD_SET(s, rdfds);

  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  while((ret = select(s+1, rdfds, NULL, NULL, &timeout)) < 1){
    if(ret == 0){
      gettimeofday(&current, NULL);
      if((resend = check_resend(chead, current.tv_sec)) != NULL){
	*state = resend->state;
        return RECV_TIMEOUT;
      }
    }

    if(ret < 0){
      perror("select");
      exit(1);
    }
  }

  fromlen = sizeof(struct sockaddr_in);

  if(recvfrom(s, recv, sizeof(struct dhcph), 0, 
	(struct sockaddr *)from, &fromlen) < 0){
    perror("recvfrom");
    exit(1);
  }

  cp = search_client(chead, from);
  *state = cp != NULL ? cp->state : INIT;
  type = get_dhcph_type(recv);

  switch(type){
    case DISCOVER:
	    return RECV_DISCOVER;
    case REQ_ALLOC:
	    if(cp != NULL && 
	       recv->ttl <= cp->ttl && 
	       recv->addr == cp->ap->addr.s_addr && 
	       recv->netmask == cp->ap->netmask.s_addr){
	      return RECV_REQ_ALLOC_OK;
	    }else{
	      return RECV_REQ_ALLOC_NG;
	    }
    case REQ_EXT_ALLOC:
	    if(cp != NULL && 
	       recv->ttl <= cp->ttl &&
	       recv->addr == cp->ap->addr.s_addr &&
	       recv->netmask == cp->ap->netmask.s_addr){
	      return RECV_REQ_EXT_ALLOC_OK;
	    }else{
	      return RECV_REQ_EXT_ALLOC_NG;
	    }
    case RELEASE:
	    if(cp != NULL){
	      return RECV_RELEASE;
	    }
    default:
	    return RECV_UNKNOWN;
  }
}

void alloc_ip(struct client *cp, struct alloc *afhead){
  struct alloc *ap;
  
  ap = pop_alloc(afhead);
  cp->ap = ap;
}

void recall_ip(struct client *cp, struct alloc *afhead){
  insert_alloc(afhead, cp->ap);
  cp->ap = NULL;
}

void set_lasttime(struct client *cp){
  struct timeval tp;

  gettimeofday(&tp, NULL);
  cp->last.tv_sec = tp.tv_sec;
  cp->last.tv_usec = tp.tv_usec;
}

void print_state_change(struct client *cp, state_t old, state_t new){
  char *id;
  unsigned short int port;
  char *states[6] = {"DUMMY", "INIT", "WAIT_REQ", 
	  "WAIT_RESEND", "IN_USE", "TERMINATE"};
  struct in_addr in;

  in.s_addr = cp->ip;
  id = inet_ntoa(in);
  port = (unsigned short int)cp->port;
  
  printf("## state changed (ID %s:%hu): %s -> %s ##\n", 
		  id, port, states[old], states[new]);
}

void f_act1(int s, int *state, uint16_t ttl, struct client *chead, 
		struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct client *cp;
  struct dhcph reply;

  cp = create_client(chead, cfhead, from);
  alloc_ip(cp, afhead);

  memset(&reply, 0, sizeof(reply));
  reply.type = OFFER_OK;
  if(cp->ap != NULL){
    reply.code = 0;
    reply.ttl = ttl;
    reply.addr = cp->ap->addr.s_addr;
    reply.netmask = cp->ap->netmask.s_addr; 
  }else{
    reply.code = 1;
  }
  
  if(sendto(s, &reply, sizeof(reply), 0, 
		(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  
  set_lasttime(cp);
  print_state_change(cp, INIT, WAIT_REQ);
  *state = cp->state = WAIT_REQ;
}

void f_act2(int s, int *state,uint16_t ttl, struct client *chead, 
	        struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct client *cp;
  struct dhcph reply;
  
  cp = search_client(chead, from);

  memset(&reply, 0, sizeof(reply));
  reply.type = ACK_OK;
  reply.code = 0;

  if(sendto(s, &reply, sizeof(reply), 0, 
	(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  
  set_lasttime(cp);
  print_state_change(cp, cp->state, IN_USE);
  *state = cp->state = IN_USE;
}

void f_act3(int s, int *state, uint16_t ttl, struct client *chead, 
		struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct client *cp;  
  struct dhcph reply;

  cp = search_client(chead, from);
  cp->ttlcounter = ttl;
  
  memset(&reply, 0, sizeof(reply));
  reply.type = ACK_OK;
  reply.code = 0;

  if(sendto(s, &reply, sizeof(reply), 0, 
		(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  
  set_lasttime(cp);
  print_state_change(cp, cp->state, IN_USE);
  *state = cp->state = IN_USE;
}

void f_act4(int s, int *state, uint16_t ttl, struct client *chead, 
		struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct client *cp;

  cp = search_client(chead, from);
  print_state_change(cp, cp->state, TERMINATE);
  *state = TERMINATE;
  recall_ip(cp, afhead);
  delete_client(cfhead, cp);
}

void f_act5(int s, int *state, uint16_t ttl, struct client *chead, 
		struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct dhcph reply;
  struct sockaddr_in to;

  reply.type = OFFER_OK;
  reply.code = 0;
  reply.ttl = ttl;
  reply.addr = resend->ap->addr.s_addr;
  reply.netmask = resend->ap->netmask.s_addr;

  to.sin_family = AF_INET;
  to.sin_addr.s_addr = resend->ip;
  to.sin_port = resend->port;

  if(sendto(s, &reply, sizeof(reply), 0, 
		(struct sockaddr *)&to, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  
  set_lasttime(resend);
  print_state_change(resend, resend->state, WAIT_RESEND);
  *state = resend->state = WAIT_RESEND;
  resend = NULL;
}

void f_act6(int s, int *state, uint16_t ttl, struct client *chead, 
		struct client *cfhead, struct alloc *ahead, 
		struct alloc *afhead, struct sockaddr_in *from){
  struct client *cp;

  cp = search_client(chead, from);
  print_state_change(cp, cp->state, TERMINATE);
  recall_ip(cp, afhead);
  delete_client(cfhead, cp);
  *state = TERMINATE;
}

void f_act7(int s, int *state, uint16_t ttl, struct client *chead,
		struct client *cfhead, struct alloc *ahead,
		struct alloc *afhead, struct sockaddr_in *from){
     print_state_change(resend, resend->state, TERMINATE);
     recall_ip(resend, afhead);
     delete_client(cfhead, resend);
     *state = TERMINATE;
     resend = NULL;
}
