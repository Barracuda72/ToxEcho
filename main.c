#include <tox/tox.h>

#include <stdio.h>
#include <string.h>

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

  return 0;
}
