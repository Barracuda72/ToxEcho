/*
 * Tox echo bot.
 * Copyright (C) 2015-2017  Braiden Vasco
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tox/tox.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define IP_LENGTH_MAX 15

typedef char IP[IP_LENGTH_MAX + 1];
typedef char Key[(TOX_PUBLIC_KEY_SIZE * 2) + 1];

struct Node {
  IP ip;
  int port;
  Key key;
};

#define NODES_COUNT 26
#define NODES_FILE_NAME "nodes"

static void Key_to_KeyBin(const char *key, uint8_t *key_bin);

static void onFriendRequest(
  Tox *tox,
  const uint8_t *key,
  const uint8_t *data,
  size_t length,
  void *user_data);

static void onFriendMessage(
  Tox *tox,
  uint32_t friend_number,
  TOX_MESSAGE_TYPE type,
  const uint8_t *string,
  size_t length,
  void *user_data);

Tox* create_tox()
{
  // Allocate Tox options and initialize with defaults

  struct Tox_Options tox_options;
  tox_options_default(&tox_options);

  // Initialize Tox instance

  TOX_ERR_NEW err;

  Tox *tox = tox_new(&tox_options, &err);

  if (err != TOX_ERR_NEW_OK)
    exit(EXIT_FAILURE);

  return tox;
}

void print_tox_id(Tox* tox)
{
  // Print ID to STDOUT

  char address[TOX_ADDRESS_SIZE];
  tox_self_get_address(tox, (uint8_t*)address);

  for (int i = 0; i < TOX_ADDRESS_SIZE; ++i) {
    char d[3];
    snprintf(d, sizeof(d), "%02X", address[i] & 0xFF);
    printf("%s", d);
  }

  printf("\n");
}

int main()
{
  Tox* tox = create_tox();

  printf("ID: ");

  print_tox_id(tox);

  // Load list of nodes from file

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

  // Bootstrap Tox with loaded nodes

  for (int node_index = 0; node_index < NODES_COUNT; ++node_index) {
    struct Node *const node = &nodes[node_index];

    uint8_t key_bin[TOX_PUBLIC_KEY_SIZE];
    Key_to_KeyBin(node->key, key_bin);

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

  // Setup callbacks for Tox instance

  tox_callback_friend_request(tox, onFriendRequest);
  tox_callback_friend_message(tox, onFriendMessage);

  // Event loop

  struct timespec delay;

  while (true) {
    delay.tv_sec = 0;
    delay.tv_nsec = tox_iteration_interval(tox) * 1000000;
    nanosleep(&delay, NULL);

    tox_iterate(tox, NULL);
  }

  // Exit gracefully

  tox_kill(tox);

  exit(EXIT_SUCCESS);
}

void Key_to_KeyBin(const char *const key, uint8_t *const key_bin)
{
  for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; ++i)
    sscanf(&key[i * 2], "%2hhx", &key_bin[i]);
}

void onFriendRequest(
  Tox *const tox,
  const uint8_t *const key,
  const uint8_t *const data,
  size_t length,
  void *const user_data)
{
  tox_friend_add_norequest(tox, key, NULL);
}

void onFriendMessage(
  Tox *const tox,
  const uint32_t friend_number,
  const TOX_MESSAGE_TYPE type,
  const uint8_t *const string,
  const size_t length,
  void *const user_data)
{
  if (type != TOX_MESSAGE_TYPE_NORMAL)
    return;

  tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL, string, length, NULL);
}
