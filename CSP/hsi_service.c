#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

#include "csp/csp.h"

#include "M6P/M6P.h"
#include "HYPSO.h"

#include "magic_interface.h"

void hsi_service_task(void *arg)
{
	csp_socket_t *socket;
	csp_conn_t *conn;
	csp_packet_t *packet;

	// Bind file service socket
	socket = csp_socket(CSP_SO_NONE);
	csp_bind(socket, OPU_HSI_PORT);
	csp_listen(socket, 5);

	CameraHandle_C camera;
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


		switch (packet->data[0])
		{
		case 1:
		{
			fprintf(stdout, "Command 1: Init camera\n");

			double exposureMs;
			int rows;
			int columns;
			int frames;
			double fps;
			int cube;

			memcpy(&exposureMs, &(packet->data[1]), 8);
			memcpy(&rows, &(packet->data[3]), 4);
			memcpy(&columns, &(packet->data[4]), 4);
			memcpy(&frames, &(packet->data[5]), 4);
			memcpy(&fps, &(packet->data[6]), 8);
			memcpy(&cube, &(packet->data[8]), 4);

      camera = create_camera_handle();
      initialize_camera(camera, exposureMs, rows, columns, frames, fps, cube);

			csp_buffer_free(packet);
			break;
		}

		case 2:
		{
			fprintf(stdout, "Command 2: Capture cube\n");
      run_camera(camera);
      printf("Captured image\n");
			csp_buffer_free(packet);
			break;
		}


		default:
		{
			//packet->data[packet->length] = '\0';
			printf("Received something: %s\n", packet->data);
			csp_buffer_free(packet);
			break;
		}
		}
		csp_close(conn);
	}
}
