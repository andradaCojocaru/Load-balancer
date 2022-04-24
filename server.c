/* Copyright 2021 Cojocaru Andrada-Ioana */
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "utils.h"
#define HMAX 10000

server_memory* init_server_memory() {
	// initializare server
	server_memory *server;

	server = malloc(sizeof(server_memory));
	DIE(server == NULL, "Eroare alocare memorie");
	server->size = 0;
	server->hmax = HMAX;
	server->buckets = calloc(HMAX, sizeof(linked_list_t *));
	DIE(server->buckets == NULL, "Eroare alocare memorie");

	for (int i = 0; i < HMAX; i++) {
		server->buckets[i] = ll_create(sizeof(struct info));
	}
	return server;
}

int
compare_function_ints(void *a, void *b)
{
	// functie comparare int
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

unsigned int hash_function_keyy(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

void server_store(server_memory* server, char* key, char* value) {
	// stocare server ca hashtable
	unsigned int index;
	ll_node_t *curr, *new;
	unsigned int key_size, value_size;
	key_size = strlen((char *)key) + 1;
	value_size = strlen((char *)value) + 1;
	index = hash_function_keyy(key) % HMAX;
	curr = server->buckets[index]->head;
	new = calloc(1, sizeof(ll_node_t));
	DIE(new == NULL, "Eroare alocare memorie");
	new->data = calloc(1, sizeof(struct info));
	DIE(new->data == NULL, "Eroare alocare memorie");
    ((struct info*)new->data)->key = calloc(1, key_size);
	DIE(((struct info*)new->data)->key  == NULL, "Eroare alocare memorie");
	memcpy(((struct info*)new->data)->key , key, key_size);
	((struct info*)new->data)->value  = calloc(1, value_size);
	DIE(((struct info*)new->data)->value  == NULL, "Eroare alocare memorie");
	memcpy(((struct info*)new->data)->value , value, value_size);
	if (curr == NULL) {
			server->buckets[index]->head = new;
			return;
	}

	while (curr != NULL) {
		if (compare_function_ints(((struct info*)curr->data)->key, key) == 0) {
			break;
		}
		curr = curr->next;
	}
	if (curr == NULL) {
		new->next = server->buckets[index]->head;
		server->buckets[index]->head = new;
		server->buckets[index]->size++;
	} else {
		memcpy(((struct info*)curr->data)->value , value,
			sizeof(*(char *)value));
		free(((struct info*)new->data)->key);
		free(((struct info*)new->data)->value);
		free(new->data);
		free(new);
		return;
	}
}

void server_remove(server_memory* server, char* key) {
	// stergere element din server
	int index = hash_function_keyy(key) % server->hmax, n = 0;
	ll_node_t *curr;
	curr = server->buckets[index]->head;

	while (curr != NULL) {
		if (compare_function_ints(((struct info*)curr->data)->key, key) == 0) {
			break;
		}
		curr = curr->next;
		n++;
	}
	if (curr == NULL) {
		return;
	} else {
		ll_remove_nth_node(server->buckets[index], n);
		free(((struct info*)curr->data)->key);
		free(((struct info*)curr->data)->value);
		free(((struct info*)curr->data));
		free(curr);
	}
}

char* server_retrieve(server_memory* server, char* key) {
	// obtinere valoare din server
	int index = hash_function_keyy(key) % server->hmax;
	ll_node_t *curr;
	curr = server->buckets[index]->head;
	while (curr != NULL) {
		if (compare_function_ints(((struct info*)curr->data)->key, key) == 0) {
			break;
		}
		curr = curr->next;
	}
	if (curr == NULL) {
		return NULL;
	} else {
		return ((struct info*)curr->data)->value;
	}
}

void free_server_memory(server_memory* server) {
	// eliberare memorie
	int i;
	ll_node_t *prev, *curr;
	for (i = 0; i < server->hmax; i++) {
		curr = server->buckets[i]->head;
		while (curr != NULL) {
        	prev = curr;
			curr = curr->next;
			free(((struct info*)prev->data)->key);
			free(((struct info*)prev->data)->value);
        	free(prev->data);
			free(prev);
		}
		free(server->buckets[i]);
	}
	free(server->buckets);
	free(server);
	server = NULL;
}
