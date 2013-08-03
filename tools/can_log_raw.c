/*
 * Raw CAN frame logger
 *
 * Prints all CAN frames seen by SocketCAN, appending the timestamp and the
 * CAN interface.
 * Optionally takes an argument of the file to which it writes.
 *
 *
 * Author: Alex Layton <awlayton@purdue.edu>
 *
 * Copyright (C) 2013 Purdue University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
 
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>
 
void print_frame(FILE *fd, char interface[], struct timeval *ts, 
		struct can_frame *cf) {
	int i;

	/* Output CAN frame */
	fprintf(fd, "<0x%08x> [%u]", cf->can_id, cf->can_dlc);
	for(i = 0; i < cf->can_dlc; i++) {
		fprintf(fd, " %02x", cf->data[i]);
	}
	/* Output timestamp and CAN interface */
	fprintf(fd, "\t%ld.%06ld\t%s\r\n", ts->tv_sec, ts->tv_usec, interface);

	/* Flush output after each frame */
	fflush(fd);
}

int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_can addr;
	socklen_t addr_len;
	struct can_frame cf;
	can_err_mask_t err_mask;
	struct ifreq ifr;
	struct timeval ts;
	FILE *fo, *fe;

	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		return EXIT_FAILURE;
	}

	/* Listen on all CAN interfaces */
	addr.can_family  = AF_CAN;
	addr.can_ifindex = 0; 

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		return EXIT_FAILURE;
	}

	/* Receive all error frames */
	err_mask = CAN_ERR_MASK;
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
			&err_mask, sizeof(err_mask));

	/* Log to first argument as a file */
	if(argc > 1) {
		fo = fe = fopen(argv[1], "w");
	} else {
		fo = stdout;
		fe = stderr;
	}

	while(1) {
		/* Print received CAN frames */
		addr_len = sizeof(addr);
		if(recvfrom(s, &cf, sizeof(cf), 0,
					(struct sockaddr *)&addr, &addr_len) > 0) {

			/* Find approximate receive time */
			gettimeofday(&ts, NULL);

			/* Find name of receive interface */
			ifr.ifr_ifindex = addr.can_ifindex;
			ioctl(s, SIOCGIFNAME, &ifr);

			/* Print fames to STDOUT, errors to STDERR */
			if(cf.can_id & CAN_ERR_FLAG) {
				print_frame(fe, ifr.ifr_name, &ts, &cf);
			} else {
				print_frame(fo, ifr.ifr_name, &ts, &cf);
			}
		}
	}

	return EXIT_SUCCESS;
}

