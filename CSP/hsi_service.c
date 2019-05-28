#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

#include "csp/csp.h"

#include "M6P.h"
#include "HYPSO.h"

#include "magic_interface.h"

void hsi_service_task(void *arg)
{
	csp_socket_t *socket;
	csp_conn_t *conn;
	csp_packet_t *packet;

	// Bind file service socket
	socket = csp_socket(CSP_SO_NONE);
	csp_bind(socket, HSI_RGB_PORT);
	csp_listen(socket, 5);


	fprintf(stdout, "HSI service start\n");

	// Serve connections
	while (1)
	{
		if ((conn = csp_accept(socket, 1000)) == NULL)
		{
			continue;
		}

		// New connection accepted, we expect a new request packet
		packet = csp_read(conn, 100);
		if (packet == NULL)
		{
			csp_close(conn);
			continue;
		}

		//printf("Communication success: %s ee\n", packet->data);

		switch (packet->data[0])
		{
		case 1:
		{
			fprintf(stdout, "Command 1: Capture cube\n");

      MHandle h = create_magic();
      initialize_magic(h, 10, 1024, 1280, 500, 6, None);
      run_magic(h);
      printf("Captured image\n");
			break;
		}

		default:
		{
			//packet->data[packet->length] = '\0';
			printf("Received something: %s\n", packet->data);
			break;
		}
		}
		csp_close(conn);
	}
}
