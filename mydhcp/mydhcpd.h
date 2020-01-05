typedef enum{
  INIT = 1,
  WAIT_REQ,
  RESEND,
  IN_USE,
  TERMINATE
}state_t;

typedef enum{
  RECV_DISCOVER = 1,
  RECV_REQ_ALLOC_OK,
  RECV_REQ_ALLOC_NG,
  RECV_REQ_EXT_ALLOC_OK,
  RECV_REQ_EXT_ALLOC_NG,
  RECV_RELEASE,
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
  uint16_t ttl;
};

struct proctable{
  state_t state;
  event_t event;
  void (*func)(int, struct client *, struct sockaddr_in *);
};

struct alloc{
  struct alloc *fp;
  struct alloc *bp;
  struct in_addr addr;
  struct in_addr netmask;
};
