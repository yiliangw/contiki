#include "contiki.h"
#include "linkaddr.h"
#include "random.h"
#include "net/rime/rime.h"

#include <stdio.h>
#include <stdlib.h>

PROCESS(example_linuxudp_radio_process, "Linux UDP Radio Example");
AUTOSTART_PROCESSES(&example_linuxudp_radio_process);

static void my_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    printf("Heard from %d.%d: '%s'\n", from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

static void my_sent()
{
    printf("Sent a message.\n");
}

static const struct broadcast_callbacks cbs = {my_recv, my_sent};
static struct broadcast_conn conn;

static int nodeid = -1;

PROCESS_THREAD(example_linuxudp_radio_process, ev, data)
{
    static struct etimer et;
    FILE *f;
    linkaddr_t addr;

    PROCESS_EXITHANDLER(broadcast_close(&conn);)

    PROCESS_BEGIN();

    /* Update linkaddr according to nodeid.cfg */
    f = fopen("nodeid.cfg", "r");
    if (fscanf(f, "%d", &nodeid) == EOF) {
        printf("Invalid nodeid\n");
        nodeid = 0;
    }

    addr.u8[0] = nodeid & 0xff;
    addr.u8[1] = nodeid >> 8;
    linkaddr_set_node_addr(&addr);

    printf("Update node address to: %d.%d\n", linkaddr_node_addr.u8[1], linkaddr_node_addr.u8[0]);
    
    broadcast_open(&conn, 128, &cbs);

    while(1) {

        etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        packetbuf_copyfrom("Hello", 6);

        broadcast_send(&conn);
    }

    PROCESS_END();
}
