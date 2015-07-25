#include <tox/tox.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IP_LENGTH_MAX 15

struct Node {
  char ip[IP_LENGTH_MAX + 1];
  int port;
  char key[(TOX_PUBLIC_KEY_SIZE * 2) + 1];
};

#define NODES_COUNT 26
#define NODES_FILE_NAME "nodes"

int main()
{
  struct Tox_Options tox_options;
  memset(&tox_options, 0, sizeof(struct Tox_Options));
  tox_options_default(&tox_options);

  TOX_ERR_NEW err;

  Tox *tox = tox_new(&tox_options, &err);

  if (err != TOX_ERR_NEW_OK)
    exit(EXIT_FAILURE);

  char address[TOX_ADDRESS_SIZE];
  tox_self_get_address(tox, (uint8_t*)address);

  printf("ID: ");

  for (int i = 0; i < TOX_ADDRESS_SIZE; ++i) {
    char d[3];
    snprintf(d, sizeof(d), "%02X", address[i] & 0xFF);
    printf("%s", d);
  }

  printf("\n");

  struct Node nodes[NODES_COUNT];

  FILE *nodes_file = fopen(NODES_FILE_NAME, "r");

  if (!nodes_file)
    exit(EXIT_FAILURE);

  for (int node_index = 0; node_index < NODES_COUNT; ++node_index) {
    struct Node *const node = &nodes[node_index];

    fscanf(nodes_file, "%s %d %s",
      node->ip,
      &node->port,
      node->key
    );
  }

  fclose(nodes_file);

  for (int node_index = 0; node_index < NODES_COUNT; ++node_index) {
    struct Node *const node = &nodes[node_index];

    uint8_t key_bin[TOX_PUBLIC_KEY_SIZE];

    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; ++i)
      sscanf(&node->key[i * 2], "%2hhx", &key_bin[i]);

    TOX_ERR_BOOTSTRAP err;

    tox_bootstrap(tox, node->ip, node->port, key_bin, &err);

    if (err != TOX_ERR_BOOTSTRAP_OK)
      fprintf(stderr, "Failed to bootstrap (\"%s\", %d, \"%s\") with error code %d\n",
        node->ip,
        node->port,
        node->key,
        err
      );
  }

  while (true) {
  }

  return 0;
}
