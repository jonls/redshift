/* server.c -- Interprocess communication server
   This file is part of Redshift.

   Redshift is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Redshift is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Redshift.  If not, see <http://www.gnu.org/licenses/>.

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "server.h"



#ifndef SOCKET_MODE
#define SOCKET_MODE  0700
#endif

#ifndef SOCKET_BACKLOG
#define SOCKET_BACKLOG  5
#endif

#ifndef SOCKET_BUFFER_SIZE
#define SOCKET_BUFFER_SIZE  2024
#endif

#define MAX_SOCKET_PATH  4096



static int server_fd = -1;
static char path[MAX_SOCKET_PATH];
static pthread_t master_thread;
static volatile int running = 0;
static volatile int running_slaves = 0;
static pthread_mutex_t slave_mutex;
static pthread_cond_t slave_cond;
static socket_callback_func *callback_function;



static void*
slave_loop(void *data)
{
	/* Ordered half-duplex communication between server and client. */

	int socket_fd = (int)(long)data;
	char *buffer = malloc(SOCKET_BUFFER_SIZE);
	ssize_t got;
	char *response;
	int nresponse;
	ssize_t sent;

	if (buffer == NULL) {
		perror("malloc");
	} else {
		while (running == 1) {
			got = recv(socket_fd, buffer, SOCKET_BUFFER_SIZE, 0);
			if (got == -1) {
				if (errno != EINTR) perror("recv");
				break;
			}
			if (got == 0)
			  	break;

			response = callback_function(socket_fd, buffer, (size_t)got);
			if (response != NULL) {
				nresponse = strlen(response);
				while (nresponse > 0) {
					sent = send(socket_fd, response,
						    nresponse * sizeof(char), 0);
					if (sent == -1) break;
					response += sent;
					nresponse -= sent;
				}
			}
		}
	}

	callback_function(socket_fd, NULL, 0);

	close(socket_fd);
	if (buffer != NULL) free(buffer);

	pthread_mutex_lock(&slave_mutex);
	running_slaves--;
	pthread_cond_signal(&slave_cond);
	pthread_mutex_unlock(&slave_mutex);

	return NULL;
}


static void*
master_loop(void *data)
{
	int socket_fd;
	pthread_t _slave_thread; /* Do not know if this is necessary, the man page does not specify. */

	(void) data;

	running = 1;
	while (running == 1) {
		socket_fd = accept(server_fd, NULL, NULL);
		if (socket_fd == -1) {
			switch (errno) {
			case EINTR:
				break;

			case ECONNABORTED:
			case EINVAL:
				running = -1;
				break;

			default:
				perror("accept");
				break;
			}
			continue;
		}

		pthread_mutex_lock(&slave_mutex);
		running_slaves++;
		pthread_mutex_unlock(&slave_mutex);

		errno = pthread_create(&_slave_thread, NULL, slave_loop, (void*)(long)socket_fd);
		if (errno) {
			perror("pthread_create");
			close(socket_fd);
			running = -1;
		}
	}

	pthread_mutex_lock(&slave_mutex);
	while (running_slaves > 0)
		pthread_cond_wait(&slave_cond, &slave_mutex);
	pthread_mutex_unlock(&slave_mutex);

	return NULL;
}


int
server_start(socket_callback_func *callback)
{
	struct sockaddr_un address;
	int r;

	callback_function = callback;

	address.sun_family = AF_UNIX;
	snprintf(path, MAX_SOCKET_PATH,
		 "/dev/shm/.redshift-socket-%i-%s", getuid(),
		 getenv("DISPLAY") ? getenv("DISPLAY") : "");
	strcpy(address.sun_path, path);
	unlink(path);

	/* Create mutex and condition for slave counter. */
	pthread_mutex_init(&slave_mutex, NULL);
	pthread_cond_init(&slave_cond, NULL);

	/* Create domain socket. */
	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		return -1;
	}

	/* Restrict access to socket to only the owner.
	   Since file does not exists yet the permissions should
	   be set atomically with the creation of the file. */
	r = fchmod(server_fd, SOCKET_MODE);
	if (r == -1) {
		perror("chmod");
		close(server_fd);
		server_fd = -1;
		return -1;
	}

	/* Bind socket to a filename. */
	r = bind(server_fd, (struct sockaddr*)(&address), sizeof(address));
	if (r == -1) {
		perror("bind");
		close(server_fd);
		server_fd = -1;
		unlink(path);
		return -1;
	}

	/* Start accepting connections. */
	r = listen(server_fd, SOCKET_BACKLOG);
	if (r == -1) {
		perror("listen");
		close(server_fd);
		server_fd = -1;
		unlink(path);
		return -1;
	}

	/* Start server thread. */
	errno = pthread_create(&master_thread, NULL, master_loop, NULL);
	if (errno) {
		perror("pthread_create");
		close(server_fd);
		server_fd = -1;
		unlink(path);
		return -1;
	}

	return 0;
}


void
server_close(void)
{
	/* Cooperative shutdown of service. */

	if (server_fd != -1) {
		shutdown(server_fd, SHUT_RDWR);
		close(server_fd);
		unlink(path);
	}

	if (running) {
		running = 0;
		errno = pthread_join(master_thread, NULL);
		if (errno) perror("pthread_join");
	}

	server_fd = -1;

	pthread_mutex_destroy(&slave_mutex);
	pthread_cond_destroy(&slave_cond);
}

