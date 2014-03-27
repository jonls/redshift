/* service.c -- Interprocess communication service
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "server.h"
#include "service.h"



#define BUFFER_SIZE  2048

extern int temp_day;
extern int temp_night;
extern float brightness_day;
extern float brightness_night;


typedef struct _service_linkedlist_t {
	struct {
		char *buffer;
		int length;
	} value;
	int key;
	struct _service_linkedlist_t *next;
	struct _service_linkedlist_t *prev;
} service_linkedlist_t;


static service_linkedlist_t *edge;
static pthread_mutex_t list_mutex;



static char*
service_interpret(char *message)
{
	char* rc = NULL;
	int r = 1;

	if (strstr(message, "GET ") == message) {
		rc = malloc(10 * sizeof(char));
		if (rc == NULL) return NULL;
	}

	if (!strcmp(message, "GET TEMP DAY"))
		snprintf(rc, 10, "%i", temp_day);
	else if (!strcmp(message, "GET TEMP NIGHT"))
		snprintf(rc, 10, "%i", temp_night);
	else if (!strcmp(message, "GET BRIGHTNESS DAY"))
		snprintf(rc, 10, "%.6f", brightness_day);
	else if (!strcmp(message, "GET BRIGHTNESS NIGHT"))
		snprintf(rc, 10, "%.6f", brightness_night);
	else if (strstr(message, "SET TEMP DAY ") == message)
		r = sscanf(message + strlen("SET TEMP DAY "), "%i", &temp_day);
	else if (strstr(message, "SET TEMP NIGHT ") == message)
		r = sscanf(message + strlen("SET TEMP NIGHT "), "%i", &temp_night);
	else if (strstr(message, "SET BRIGHTNESS DAY ") == message)
		r = sscanf(message + strlen("SET BRIGHTNESS DAY "), "%f", &brightness_day);
	else if (strstr(message, "SET BRIGHTNESS NIGHT ") == message)
		r = sscanf(message + strlen("SET BRIGHTNESS NIGHT "), "%f", &brightness_night);
	else {
		if (rc != NULL) free(rc);
		rc = NULL;
		fprintf(stderr, _("Unrecognized IPC command: %s\n"), message);
	}

	if (r != 1) {
		fprintf(stderr, _("Malformated value in IPC command: %s\n"), message);
	}

	return rc;
}


static char*
service_callback(int socket_fd, char *message, size_t length)
{
	service_linkedlist_t *node = edge;
	char *rc = NULL;
	size_t rc_size = 0;
	size_t rc_ptr = 0;
	size_t i;

	do {
		node = node->next;
		if (node->key == socket_fd)
			break;
	} while (node != edge);

	if (message == NULL) {
		if (node == edge)
			return NULL;
		pthread_mutex_lock(&list_mutex);
		node->prev->next = node->next;
		node->next->prev = node->prev;
		pthread_mutex_unlock(&list_mutex);
		if (node->value.buffer != NULL)
			free(node->value.buffer);
		free(node);
		return NULL;
	}

	if (node == edge) {
		node = malloc(sizeof(service_linkedlist_t));
		if (node == NULL) {
			perror("malloc");
			close(socket_fd);
			return NULL;
		}
		node->key = socket_fd;
		node->value.buffer = malloc(BUFFER_SIZE);
		node->value.length = 0;
		pthread_mutex_lock(&list_mutex);
		node->prev = edge->next;
		edge->prev = node;
		node->next = edge;
		node->prev->next = node;
		pthread_mutex_unlock(&list_mutex);
	}

	for (i = 0; i < length; i++) {
		if (message[i] == '\n') {
			char *r;
			int n;
			node->value.buffer[node->value.length] = '\0';
			r = service_interpret(node->value.buffer);
			if (r != NULL) {
				n = strlen(r);
				if (rc == NULL) {
					rc_size = n;
					rc = malloc((rc_size + 1) * sizeof(char));
					if (rc == NULL) {
						perror("malloc");
						close(socket_fd);
						return NULL;
					}
				} else if (rc_ptr + n > rc_size) {
					rc_size = rc_ptr + n;
					rc = realloc(rc, (rc_size + 1) * sizeof(char));
					if (rc == NULL) {
						perror("realloc");
						close(socket_fd);
						return NULL;
					}
				}
				memcpy(rc + rc_ptr, r, n * sizeof(char));
				rc_ptr += n;
				free(r);
			}
			node->value.length = 0;
		} else {
			node->value.buffer[node->value.length++] = message[i];
			node->value.length %= BUFFER_SIZE;
		}
	}

	if (rc != NULL)
		rc[rc_ptr] = '\0';
	return rc;
}


int
service_start(void)
{
	pthread_mutex_init(&list_mutex, NULL);

	edge = malloc(sizeof(service_linkedlist_t));
	if (edge == NULL) {
		perror("malloc");
		return -1;
	}

	edge->next = edge;
	edge->prev = edge;
	edge->value.buffer = NULL;
	edge->value.length = 0;
	edge->key = -1;

	return server_start(service_callback);
}


void
service_close(void)
{
	server_close();

	if (edge != NULL) {
		free(edge);
		edge = NULL;
	}

	pthread_mutex_destroy(&list_mutex);
}

