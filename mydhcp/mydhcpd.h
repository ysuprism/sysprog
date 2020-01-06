#include <stdint.h>
#include <arpa/inet.h>

typedef enum{
  INIT = 1,
  WAIT_REQ,
  WAIT_RESEND,
  IN_USE,
  TERMINATE
}state_t;

typedef enum{
  RECV_DISCOVER = 1,
  RECV_REQ_ALLOC_OK,
  RECV_REQ_ALLOC_NG,
  RECV_TIMEOUT,
  RECV_REQ_EXT_ALLOC_OK,
  RECV_REQ_EXT_ALLOC_NG,
  RECV_RELEASE,
  TTL_TIMEOUT,
  RECV_UNKNOWN
}event_t;

struct client{
  struct client *fp;
  struct client *bp;
  state_t state;
  int ttlcounter;
  in_addr_t ip;
  in_port_t port;
  struct alloc *ap;
  struct timeval last;
  uint16_t ttl;
};

struct proctable{
  state_t state;
  event_t event;
  void (*func)(int, state_t *, uint16_t, struct client *, struct client *, 
		  struct alloc *, struct alloc *, struct sockaddr_in *);
};

struct alloc{
  struct alloc *fp;
  struct alloc *bp;
  struct in_addr addr;
  struct in_addr netmask;
};
