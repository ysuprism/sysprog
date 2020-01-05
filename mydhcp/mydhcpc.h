typedef enum{
  INIT = 1,
  WAIT_OFFER,
  WAIT_ACK,
  IN_USE,
  WAIT_EXT_ACK,
  EXIT
}state;

typedef enum{
  RECV_OFFER_OK,
  RECV_OFFER_NO,
  RECV_ACK,
  TTL/2_PASSED,
  RECV_TIMEOUT,
  RECV_SIGHUP
}event;
