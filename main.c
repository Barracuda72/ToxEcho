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

#define _POSIX_C_SOURCE 200809L // For nanosleep()

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sodium/utils.h>
#include <tox/tox.h>

#define SAVEDATA_FILE "savedata.tox"

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

static void onConnectionStatus(
  Tox *tox, 
  TOX_CONNECTION connection_status, 
  void *user_data);

Tox* create_tox(const char* savedata_filename)
{
  // Allocate Tox options and initialize with defaults

  struct Tox_Options options;
  tox_options_default(&options);

  // Initialize Tox instance

  TOX_ERR_NEW err;
  Tox* tox = NULL;

  FILE* f = NULL;
  uint8_t* savedata = NULL;

  if (savedata_filename)
    f = fopen(savedata_filename, "rb");
  
  if (f) 
  {
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
 
    uint8_t *savedata = malloc(fsize);
    long read = fread(savedata, fsize, 1, f);
    fclose(f);

    if (read != fsize)
    {
      fprintf(stderr, "Failed to read savedata '%s': %s\n", 
        savedata_filename, strerror(errno));
    } else {
      options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
      options.savedata_data = savedata;
      options.savedata_length = fsize;
    }
  }

  tox = tox_new(&options, &err);
 
  if (savedata)
    free(savedata);

  if (err != TOX_ERR_NEW_OK)
    exit(EXIT_FAILURE);

  return tox;
}

char* tmpname(const char* prefix)
{
  int n = strlen(prefix);
  char* res = malloc(n+7+1);
  strncpy(res, prefix, n+1);
  strncat(res, "-XXXXXX", n+7+1);

  return res;
}

void update_savedata_file(const Tox *tox, const char* savedata_filename)
{
    size_t size = tox_get_savedata_size(tox);
    uint8_t *savedata = malloc(size);
    tox_get_savedata(tox, savedata);

    char* savedata_tmp_filename = tmpname(savedata_filename);

    FILE *f = fopen(savedata_tmp_filename, "wb");
    fwrite(savedata, size, 1, f);
    fclose(f);

    rename(savedata_tmp_filename, savedata_filename);

    free(savedata);
    free(savedata_tmp_filename);
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

void bootstrap_tox(Tox* tox)
{
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
    sodium_hex2bin(key_bin, sizeof(key_bin), node->key, sizeof(node->key)-1,
                       NULL, NULL, NULL);

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
}

int main()
{
  Tox* tox = create_tox(SAVEDATA_FILE);

  printf("ID: ");

  print_tox_id(tox);

  bootstrap_tox(tox);

  // Setup callbacks for Tox instance

  tox_callback_friend_request(tox, onFriendRequest);
  tox_callback_friend_message(tox, onFriendMessage);
  tox_callback_self_connection_status(tox, onConnectionStatus);

  update_savedata_file(tox, SAVEDATA_FILE);

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

void onFriendRequest(
  Tox *const tox,
  const uint8_t *const key,
  const uint8_t *const data,
  size_t length,
  void *const user_data)
{
  tox_friend_add_norequest(tox, key, NULL);
  update_savedata_file(tox, SAVEDATA_FILE);
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

void onConnectionStatus(
  Tox *const tox, 
  const TOX_CONNECTION connection_status, 
  void *const user_data)
{
  switch (connection_status) 
  {
    case TOX_CONNECTION_NONE:
      printf("Offline\n");
      break;

    case TOX_CONNECTION_TCP:
      printf("Online, using TCP\n");
      break;

    case TOX_CONNECTION_UDP:
      printf("Online, using UDP\n");
      break;
  }
}

