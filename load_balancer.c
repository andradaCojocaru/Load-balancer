/* Copyright 2021 Cojocaru Andrada-Ioana */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "utils.h"

#define HMAX 10000
#define HMAX_BALANCER 10000000
#define NUM 100000

struct server_info {
    unsigned int eticheta;
    unsigned int replica;
};

struct load_balancer {
	server_memory **memory;
    server_info *hashring;
    int size;
    int size_max;
};

unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}


load_balancer* init_load_balancer() {
    // initializare balancer cu elementele sale
	load_balancer *main;
    main = calloc(1, sizeof(load_balancer));
    DIE(main == NULL, "Eroare alocare memorie");
    main->size = 0;
    main->size_max = HMAX_BALANCER;
    main->memory = calloc(NUM, sizeof(server_memory*));
    DIE(main->memory == NULL, "Eroare alocare memorie");
    main->hashring = calloc(HMAX_BALANCER, sizeof(server_info));
    DIE(main->hashring == NULL, "Eroare alocare memorie");

	return main;
}

void
loader_store(load_balancer* main, char* key, char* value, int* server_id) {
    // adaugare obiecte pe serverele corespunzatoare
	int i = 0;
    unsigned int index;
    index = hash_function_key(key);
    while (i < main->size && main->hashring[i].replica < index) {
        i++;
    }
    if (i >= main->size) {
            i = 0;
    }
    *server_id = main->hashring[i].eticheta % NUM;
    server_store(main->memory[*server_id], key, value);
}

char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
    // obtinerea valorii corespunzatoare cheii
	int i = 0;
    unsigned int index;
    index = hash_function_key(key);
    while (i < main->size && main->hashring[i].replica < index) {
        i++;
    }
    if (i >= main->size) {
            i = 0;
    }
    *server_id = main->hashring[i].eticheta % NUM;
    return server_retrieve(main->memory[*server_id], key);
}

void loader_add_server(load_balancer* main, int server_id) {
    // adaugare server
    server_memory *server;
	unsigned int index, eticheta;
    unsigned int key_size, value_size;
    int j, k, i;
    int id = 0, poz;
    ll_node_t *curr;
    char *key;
    char *value;
    main->memory[server_id] = init_server_memory();
    // adaugare replici
    for (i = 0; i < 3; i++) {
        eticheta = i * NUM + server_id;
        index = hash_function_servers(&eticheta);
        j = 0;
        while (j < main->size && main->hashring[j].replica < index) {
            j++;
        }
        if (main->size  >= main->size_max) {
            main->hashring = realloc(main->hashring, 3 * sizeof(server_info));
            main->size_max += 3;
        }
        if (main->size != 0) {
            for (k = main->size - 1; k >= j; k--) {
                main->hashring[k + 1].eticheta = main->hashring[k].eticheta;
                main->hashring[k + 1].replica = main->hashring[k].replica;
            }
        }
        main->hashring[j].eticheta = eticheta;
        main->hashring[j].replica = index;
        main->size = main->size + 1;
    }
    // balansarea obiectelor pe noul server
    for (i = 0; i < main->size; i++) {
        if ((int)main->hashring[i].eticheta % NUM == server_id) {
            if (i < main->size - 1) {
                poz = main->hashring[i + 1].eticheta % NUM;
            } else {
                poz = main->hashring[0].eticheta % NUM;
            }
            server = main->memory[poz];
            k = 0;
            for (j = 0; j < server->hmax; j++) {
		        curr = server->buckets[j]->head;
		        while (curr != NULL) {
                    key = ((struct info*)curr->data)->key;
                    value = ((struct info*)curr->data)->value;
                    key_size = strlen(key) + 1;
                    value_size = strlen(value) + 1;
                    char key_duplicate[key_size], value_duplicate[value_size];
                    strcpy(key_duplicate, key);
                    strcpy(value_duplicate, value);
                    curr = curr->next;
                    loader_store(main, key_duplicate, value_duplicate, &id);
                    if (id != poz) {
                        server_remove(server, key);
                    }
		        }
	        }
        }
    }
}

void loader_remove_server(load_balancer* main, int server_id) {
    // stergerea unui server
	server_memory* server;
    int i, j;
    int id;
	ll_node_t *curr;
    char *key;
    char *value;
    server = main->memory[server_id];
    for (i = 0; i < main->size; i++) {
        if ((int)main->hashring[i].eticheta % NUM == server_id) {
            for (j = i; j < main->size - 1; j++) {
                main->hashring[j].eticheta = main->hashring[j + 1].eticheta;
                main->hashring[j].replica = main->hashring[j + 1].replica;
            }
            main->size--;
            i--;
        }
    }
    // balansarea obiectelor de pe serverul scos
	for (i = 0; i < server->hmax; i++) {
		curr = server->buckets[i]->head;
		while (curr != NULL) {
			key = ((struct info*)curr->data)->key;
			value = ((struct info*)curr->data)->value;
            curr = curr->next;
            loader_store(main, key, value, &id);
            server_remove(server, key);
		}
	}
}

void free_load_balancer(load_balancer* main) {
    // eliberarea memoriei
    unsigned int i;
    for (i = 0 ; i < NUM; i++) {
        if (main->memory[i] != NULL) {
            free_server_memory(main->memory[i]);
        }
    }
    free(main->memory);
    free(main->hashring);
    free(main);
}

