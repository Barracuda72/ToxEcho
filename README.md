Tox echo bot
============

Simpliest program using [toxcore](https://github.com/irungentoo/toxcore).
It connects to Tox, accepts any friendship request and returns received
messages to user.

List of DHT nodes to bootstrap (file `nodes`)
comes from Toxic source (not available anymore, refer to
[the official Tox status server](https://nodes.tox.chat)).

**WARNING**

This program doesn't save state, so you have to send friendship request
again after each run. For example of permanent state look at
[ToxBot source](https://github.com/JFreegman/ToxBot/blob/d0ae67f364979cb9bf57f799d2cdd3aff234146b/src/toxbot.c#L280).

Further studying
----------------

* [Main header of toxcore](https://github.com/irungentoo/toxcore/blob/master/toxcore/tox.h)
  is well-documented

* [ToxBot](https://github.com/JFreegman/ToxBot) is more functional, but very simple too
