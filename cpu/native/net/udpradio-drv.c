#include "contiki.h"
#include "contiki-conf.h"

#include "net/packetbuf.h"
#include "net/netstack.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "udpradio-drv.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int sockfd = -1;
static int clientfd = -1;
static char *sockbuf;
static int buflen;

#define MAX_PACKET_SIZE 256

static int
init(void)
{
  sockbuf = malloc(MAX_PACKET_SIZE);
  if(sockbuf == 0) {
    return 1;
  }
  return 0;
}
static int
prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > MAX_PACKET_SIZE) {
    return 0;
  }
  memcpy(sockbuf, payload, payload_len);
  buflen = payload_len;

  return 0;
}
static int
transmit(unsigned short transmit_len)
{
  int sent = 0;
  sent = send(sockfd, sockbuf, buflen, 0);
  if(sent < 0) {
    perror("udpradio send()");
    return RADIO_TX_ERR;
  }
  buflen = 0;
  return RADIO_TX_OK;
}
static int
my_send(const void *payload, unsigned short payload_len)
{
  int ret = -1;

  if(prepare(payload, payload_len)) {
    return ret;
  }

  ret = transmit(payload_len);

  return ret;
}
static int
my_read(void *buf, unsigned short buf_len)
{
  return 0;
}
static int
channel_clear(void)
{
  return 1;
}
static int
receiving_packet(void)
{
  return 0;
}
static int
pending_packet(void)
{
  return 0;
}
static int
set_fd(fd_set *rset, fd_set *wset)
{
  FD_SET(sockfd, rset);
  return 1;
}
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  if(FD_ISSET(sockfd, rset)) {
    int bytes = read(sockfd, sockbuf, MAX_PACKET_SIZE);
    buflen = bytes;
    memcpy(packetbuf_dataptr(), sockbuf, bytes);
    packetbuf_set_datalen(bytes);
    NETSTACK_RDC.input();
  }
}

static const struct select_callback udpradio_sock_callback = { set_fd, handle_fd };

/* Dynamic later. */
#define SRV_PORT    52001

static int
on(void)
{
  struct sockaddr_in srv_addr;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0) {
    perror("udpradio socket()");
    return 0;
  } 

  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = htons(SRV_PORT);
  inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);

  clientfd = connect(sockfd, (struct sockaddr*)&srv_addr, sizeof srv_addr);
  if(clientfd < 0) {
    perror("udpradio connect()");
    return 0;
  }

  select_set_callback(sockfd, &udpradio_sock_callback);
  return 1;
}
static int
off(void)
{
  close(clientfd);
  clientfd = -1;
  sockfd = -1;
  return 1;
}

const struct radio_driver udpradio_driver =
{
  init,
  prepare,
  transmit,
  my_send,
  my_read,
  channel_clear,
  receiving_packet,
  pending_packet,
  on,
  off,
};
