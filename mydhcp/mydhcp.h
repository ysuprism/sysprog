#include <stdint.h>
#include <arpa/inet.h>

typedef enum{
  DISCOVER,
  OFFER_OK,
  OFFER_NG,
  REQ_ALLOC,
  REQ_EXT_ALLOC,
  ACK_OK,
  ACK_NG,
  RELEASE,
  UNKNOWN
}dhcph_t;

struct dhcph{
  uint8_t type;
  uint8_t code;
  uint16_t ttl;
  uint32_t addr;
  uint32_t netmask;
};
