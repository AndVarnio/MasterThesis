#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "csp/csp.h"

#include "M6P.h"
#include "HYPSO.h"

#include "cli_hsi.h"

int cli_hsi_init(char *args)
{
	csp_packet_t *packet = csp_buffer_get(32);
	packet->data[0] = 1;

	packet->length = strlen(packet->data);
	packet->data[packet->length - 1] = '\0';

	csp_conn_t *conn;
	conn = csp_connect(CSP_PRIO_NORM, 4, HSI_RGB_PORT, 1000, CSP_O_CRC32);
	csp_send(conn, packet, 1000);
	csp_close(conn);

	return 0;
}

void cli_hsi_init_cmds(cli_cmd *c_hsi)
{
	cli_cmd *c_hsi_init =
		cli_add("info", "",
			"Cli hsi init.", cli_hsi_init, c_hsi);
}
