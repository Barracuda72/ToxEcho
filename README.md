Tox echo bot
============

Simple program using [toxcore](https://github.com/irungentoo/toxcore).
It connects to Tox, accepts any friendship request and returns received
messages to user.

It also can accept group invitations, reply to messages in group chats and react to calls (reject them).

List of DHT nodes to bootstrap (file `nodes`)
comes from Toxic source (not available anymore, refer to
[the official Tox status server](https://nodes.tox.chat)).

This program **does** save state, so you **don't** have to send friendship request
again after each run. But group membership is still lost on disconnect.

Further studying
----------------

* [Original ToxEcho bot, much simpler and cleaner](https://github.com/toxon/ToxEcho)

* [Main header of toxcore](https://github.com/irungentoo/toxcore/blob/master/toxcore/tox.h)
  is well-documented

* [ToxBot](https://github.com/JFreegman/ToxBot) is more functional, but very simple too
