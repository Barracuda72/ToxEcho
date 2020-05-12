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
#include <unistd.h>

#include <sodium/utils.h>
#include <tox/tox.h>
#include <tox/toxav.h>

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

static void onConferenceInvite(
  Tox *tox, 
  uint32_t friend_number, 
  TOX_CONFERENCE_TYPE type, 
  const uint8_t *cookie,
  size_t length, 
  void *user_data);

static void onConferenceMessage(
  Tox *tox, 
  uint32_t conference_number, 
  uint32_t peer_number,
  TOX_MESSAGE_TYPE type, 
  const uint8_t *message, 
  size_t length, 
  void *user_data);

static void onAudioReceived(
  /*Tox*/void *tox, 
  uint32_t group_number, 
  uint32_t peer_number, 
  const int16_t *pcm, 
  unsigned int samples, 
  uint8_t channels, 
  uint32_t sample_rate, 
  void *user_data);

static void onCallReceived(
  ToxAV *av, 
  uint32_t friend_number, 
  bool audio_enabled, 
  bool video_enabled, 
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

    if (read != 1)
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
  const char* suffix = "-XXXXXX";
  char* res = malloc(strlen(prefix)+strlen(suffix)+1);
  strcpy(res, prefix);
  strcat(res, suffix);

  return res;
}

void update_savedata_file(const Tox *tox, const char* savedata_filename)
{
    int size = tox_get_savedata_size(tox);
    uint8_t *savedata = malloc(size);
    tox_get_savedata(tox, savedata);

    char* savedata_tmp_filename = tmpname(savedata_filename);

    int fd = mkstemp(savedata_tmp_filename);
    int written = write(fd, savedata, size);
    close(fd);

    if (written != size)
      fprintf(stderr, "Error saving savedata!\n");

    rename(savedata_tmp_filename, savedata_filename);

    free(savedata);
    free(savedata_tmp_filename);
}

void print_tox_id(Tox* tox)
{
  // Print ID to STDOUT

  char address[TOX_ADDRESS_SIZE];
  tox_self_get_address(tox, (uint8_t*)address);

  for (unsigned int i = 0; i < TOX_ADDRESS_SIZE; ++i) {
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

    int count = fscanf(nodes_file, "%s %d %s",
      node->ip,
      &node->port,
      node->key
    );

    if (count != 3)
      fprintf(stderr, "Failed to parse node %d description!\n", node_index);
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

  const char *name = "Echo Bot";
  tox_self_set_name(tox, (uint8_t*)name, strlen(name), NULL);

  const char *status_message = "Echoing your messages";
  tox_self_set_status_message(tox, (uint8_t*)status_message, strlen(status_message), NULL);

  bootstrap_tox(tox);

  printf("ID: ");

  print_tox_id(tox);

  // Create corresponding ToxAV
  ToxAV* toxav = toxav_new(tox, NULL);

  // Setup callbacks for Tox instance

  tox_callback_friend_request(tox, onFriendRequest);
  tox_callback_friend_message(tox, onFriendMessage);
  tox_callback_self_connection_status(tox, onConnectionStatus);
  tox_callback_conference_invite(tox, onConferenceInvite);
  tox_callback_conference_message(tox, onConferenceMessage);

  toxav_callback_call(toxav, onCallReceived, NULL);

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

  toxav_kill(toxav);
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
  // Unused variables
  (void)data;
  (void)length;
  (void)user_data;

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
  // Unused variables
  (void)user_data;

  if (type != TOX_MESSAGE_TYPE_NORMAL)
    return;

  tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL, string, length, NULL);
}

void onConnectionStatus(
  Tox *const tox, 
  const TOX_CONNECTION connection_status, 
  void *const user_data)
{
  // Unused variables
  (void)tox;
  (void)user_data;

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

void onConferenceInvite(
  Tox *const tox, 
  const uint32_t friend_number, 
  const TOX_CONFERENCE_TYPE type, 
  const uint8_t *const cookie,
  size_t length, 
  void *const user_data)
{
  // Unused variables
  (void)user_data;

  switch (type)
  {
    case TOX_CONFERENCE_TYPE_TEXT:
      tox_conference_join(tox, friend_number, cookie, length, NULL);
      break;

    case TOX_CONFERENCE_TYPE_AV:
      toxav_join_av_groupchat(tox, friend_number, cookie, length, onAudioReceived, NULL);
      break;
  }

  update_savedata_file(tox, SAVEDATA_FILE);
}

void onConferenceMessage(
  Tox *const tox, 
  const uint32_t conference_number, 
  const uint32_t peer_number,
  const TOX_MESSAGE_TYPE type, 
  const uint8_t *const message, 
  size_t length, 
  void *const user_data)
{
  // Unused variables
  (void)user_data;

  if (type != TOX_MESSAGE_TYPE_NORMAL)
    return;

  if (!tox_conference_peer_number_is_ours(tox, conference_number, peer_number, NULL))
    tox_conference_send_message(tox, conference_number, TOX_MESSAGE_TYPE_NORMAL, message, length, NULL);
}

void onAudioReceived(
  /*Tox*/void *const tox, 
  const uint32_t group_number, 
  const uint32_t peer_number, 
  const int16_t *const pcm, 
  const unsigned int samples, 
  const uint8_t channels, 
  const uint32_t sample_rate, 
  void *const user_data)
{
  // Unused variables
  (void)tox;
  (void)group_number;
  (void)peer_number;
  (void)pcm;
  (void)samples;
  (void)channels;
  (void)sample_rate;
  (void)user_data;

  // Do nothing
}

void onCallReceived(
  ToxAV *const av, 
  const uint32_t friend_number, 
  const bool audio_enabled, 
  const bool video_enabled, 
  void *const user_data)
{
  // Unused variables
  (void)audio_enabled;
  (void)user_data;

  // Sleep a bit
  struct timespec delay;

  delay.tv_sec = 3;
  delay.tv_nsec = 0;
  nanosleep(&delay, NULL);

  // Reject call
  toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, NULL);

  // Send message
  const char* message = NULL;

  if (video_enabled)
    message = "Sorry, I don't accept video calls";
  else
    message = "Sorry, I'm not in the mood for voice chat";

  tox_friend_send_message(toxav_get_tox(av), friend_number, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*)message, strlen(message), NULL);
}
