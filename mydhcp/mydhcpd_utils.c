#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mydhcp.h"
#include "mydhcpd.h"

void init_client(struct client *t){
  memset(t, 0, sizeof(struct client));
  t->fp = t->bp = t;
}

void insert_client(struct client *head, struct client *t){
  t->fp = head;
  t->bp = head->bp;
  head->bp->fp = t;
  head->bp = t;
}

void remove_client(struct client *t){
  t->fp->bp = t->bp;
  t->bp->fp = t->fp;
  t->fp = t->bp = NULL;
  free(t);
}

struct client *search_client(struct client *head, struct sockaddr_in *from){
  struct client *p;
  if(head->fp != NULL){
    for(p = head->fp; p != head; p = p->fp){
      if(p->ip == from->sin_addr.s_addr && p->port == from->sin_port){
        return p;
      }
    }
  }
  return NULL;
}

struct client *create_client(struct client *head, struct sockaddr_in *from){
  struct client *new = (struct client *)malloc(sizeof(struct client));
  memset(new, 0, sizeof(struct client));
  new->ip = from->sin_addr.s_addr;
  new->port = from->sin_port;
  new->status = INIT;
  insert_client(head, new);
  return new;
}

void init_alloc(struct alloc *t){
  memset(t, 0, sizeof(struct alloc));
  t->fp = t->bp = t;
}

void insert_alloc(struct alloc *head, struct alloc *t){
  t->fp = head;
  t->bp = head->bp;
  head->bp->fp = t;
  head->bp = t;
}

struct alloc *remove_alloc(struct alloc *head){
  struct alloc *p = head->fp;
  if(head->fp != head){
    head->fp = p->fp;
    p->fp->bp = head;
    p->fp = p->bp = NULL;
    return p;
  }
  return NULL;
}

int get_rownum(char *filename){
  FILE *fp;
  char buf[256];
  int count = 0;
  if((fp = fopen(filename, "r")) == NULL){
    perror("fopen");
    exit(1);
  }
  while(fgets(buf, sizeof(buf), fp) != NULL){
    count++;
  }
  fclose(fp);
  return count;
}

void init_alloclist(struct alloc *head, struct alloc list[], 
		int len, char *filename){
  FILE *fp;
  int ttl;
  char addr[128], netmask[128];
  if((fp = fopen(filename, "r")) == NULL){
    perror("fopen");
    exit(1);
  }
  fscanf(fp, "%d", &ttl);
  init_alloc(head);
  for(int i = 0; i < len; i++){
    fscanf(fp, "%s%s", addr, netmask);
    inet_aton(addr, &list[i].addr);
    inet_aton(netmask, &list[i].netmask);
    insert_alloc(head, &list[i]);
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

event_t wait_event(int s, state_t *state, fd_set *rdfds, 
		struct timeval *timeout, struct dhcph *recv, 
		struct sockaddr_in *from, struct client *head){
  int ret, fromlen;
  dhcph_t type;
  struct client *cp;

  fromlen = sizeof(struct sockaddr_in);
  FD_ZERO(rdfds);
  FD_SET(s, rdfds);

  if((ret = select(s+1, rdfds, NULL, NULL, timeout)) < 0){
    perror("select");
    exit(1);
  }else if(ret == 0 && *state == WAIT_REQ){
    return RECV_TIMEOUT;
  }

  if(recvfrom(s, recv, sizeof(struct dhcph), 0, 
	(struct sockaddr *)from, &fromlen) < 0){
    perror("recvfrom");
    exit(1);
  }

  cp = search_client(head, recv);
  *state = cp != NULL ? cp->state : INIT;

  type = get_dhcph_type(recv);
  switch(type){
    case DISCOVER:
	    return RECV_DISCOVER;
    case REQ_ALLOC:
	    if(cp != NULL && 
	       recv->ttl <= cp->ttl && 
	       recv->ip == cp->ap->addr.s_addr && 
	       recv->netmask == cp->ap->netmask.s_addr){
	      return RECV_REQ_ALLOC_OK;
	    }else{
	      return RECV_REQ_ALLOC_NG;
	    }
    case REQ_EXT_ALLOC:
	    if(cp != NULL && 
	       recv->ttl <= cp->ttl &&
	       recv->ip == cp->ap->addr.s_addr &&
	       recv->netmask == cp->ap->netmask.s_addr){
	      return RECV_REQ_EXT_ALLOC_OK;
	    }else{
	      return RECV_REQ_EXT_ALLOC_NG;
	    }
    case RELEASE:
	    return RECV_RELEASE;
    case default:
	    return RECV_UNKNOWN;
  }
}

void alloc_ip(struct client *cp, struct alloc *head){
  struct alloc *ap = remove_alloc(head);
  cp->ap = ap;
}

void recall_ip(struct client *cp, struct alloc *head){
  insert_alloc(head, cp->ap);
}

void f_act1(int s, uint16_t ttl, struct client *ch, 
		struct alloc *ah, struct sockaddr_in *from){
  struct client *cp;
  struct dhcph reply = {0};

  cp = create_client(ch, from);
  alloc_ip(cp, ah);

  reply.type = OFFER;
  if(cp->ap != NULL){
    reply.code = 0;
    reply.ttl = ttl;
    reply.ip = cp->ap.addr.s_addr;
    reply.netmask = cp->ap.netmask.s_addr; 
  }else{
    reply.code = 1;
  }
  
  if(sendto(s, &reply, sizeof(struct dhcph), 0, 
	(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  cp->state = WAIT_REQ;
}

void f_act2(int s, uint16_t ttl, struct client *ch,
		struct alloc *ah, struct sockaddr_in *from){
  struct dhcph reply = {0};

  reply.type = ACK;
  reply.code = 0;

  if(sendto(s, &reply, sizeof(struct dhcph), 0, 
	(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
  cp->state = IN_USE;
}

void f_act3(int s, uint16_t ttl, struct client *ch,
		struct alloc *ah, struct sockaddr_in *from){
  struct client *cp;  
  struct dhcph reply = {0};

  cp = search_client(ch, from);
  cp->ttlcounter = ttl;

  reply.type = ACK;
  reply.code = 0;

  if(sendto(s, &reply, sizeof(struct dhcph), 0, 
	(struct sockaddr *)from, sizeof(struct sockaddr_in)) < 0){
    perror("sendto");
    exit(1);
  }
}

void f_act4(int s, uint16_t ttl, struct client *ch,
		struct alloc *ah, struct sockaddr_in *from){
  struct client *cp;

  cp = search_client(ch, from);
  recall_ip(cp, ah);
  remove_client(cp);
}

void f_act5(int s, uint16_t ttl, struct client *ch,
		struct alloc *ah, struct sockaddr_in *from){
  struct client *cp;
  struct dhcph reply = {0};

  reply.type = OFFER;
  reply.code = 
}

void f_act6(int s, uint16_t ttl, struct client *ch, 
		struct alloc *ah, struct sockaddr_in *from){
  struct client *cp;

  cp = search_client(ch, from);
  remove_client(cp);
}
