Tox echo bot
============

Simpliest program using [toxcore](https://github.com/irungentoo/toxcore).
It connects to Tox, accepts any friendship request and returns received
messages to user.

List of DHT nodes to bootstrap (file `nodes`) comes from
[Toxic source](https://github.com/Tox/toxic/blob/master/misc/DHTnodes).

**WARNING**

This program doesn't save state, so you have to send friendship request
again after each run. For example of permanent state look at
[ToxBot source](https://github.com/JFreegman/ToxBot/blob/master/src/toxbot.c#L277).

Further studying
----------------

* [Main header of toxcore](https://github.com/irungentoo/toxcore/blob/master/toxcore/tox.h)
  is well-documented

* [ToxBot](https://github.com/JFreegman/ToxBot) is more functional, but very simple too
