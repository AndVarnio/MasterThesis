#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "csp/csp.h"

#include "M6P/M6P.h"
#include "HYPSO.h"

#include "cli/cli_hsi.h"

int cli_hsi_init(char *args)
{
	double exposureMs;
	int rows;
	int columns;
	int frames;
	double fps;
	int cube;

	if (sscanf(args, "%lf %i %i %i %lf %i", &exposureMs, &rows, &columns, &frames, &fps, &cube) != 6) {
		printf("%s: Invalid input.\n", __func__);
		return -EINVAL;
	}

	int csp_id;
	csp_id = atoi(args);

	csp_packet_t *packet = csp_buffer_get(36);
	memcpy(&(packet->data[1]), &exposureMs, 8);
	memcpy(&(packet->data[3]), rows, 4);
	memcpy(&(packet->data[4]), columns, 4);
	memcpy(&(packet->data[5]), frames, 4);
	memcpy(&(packet->data[6]), &fps, 8);
	memcpy(&(packet->data[8]), cube, 4);

	packet->length = 36;

	csp_conn_t *conn;
	conn = csp_connect(CSP_PRIO_NORM, csp_id, OPU_HSI_PORT, 1000, CSP_O_CRC32);
	if (csp_send(conn, packet, 1000) != 1) {
		csp_buffer_free(packet);
	}
	csp_close(conn);

	return 0;
}

int cli_hsi_capture(char *args)
{

	int csp_id;
	csp_id = atoi(args);

	csp_packet_t *packet = csp_buffer_get(24);

	csp_conn_t *conn;
	conn = csp_connect(CSP_PRIO_NORM, csp_id, OPU_HSI_PORT, 1000, CSP_O_CRC32);
	if (csp_send(conn, packet, 1000) != 1) {
		csp_buffer_free(packet);
	}
	csp_close(conn);

	return 0;
}


void cli_hsi_init_cmds(cli_cmd *c_hsi)
{
	cli_cmd *c_hsi_init =
		cli_add("init", "<CSP ID>",
			"Cli hsi init.", cli_hsi_init, c_hsi);

	cli_cmd *c_hsi_capture =
		cli_add("capture", " ...", " ... ", cli_hsi_capture, c_hsi);
}
