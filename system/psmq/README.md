# System / `psmq` Publish Subscribe Message Queue

`psmq` is publish subscribe message queue. It's a set of programs and libraries
to implement publish/subscribe way of inter-process communication on top of
POSIX message queue.

Manuals, source code and more info at: https://psmq.kurwinet.pl

Little demo using `psmqd` broker, `psmq_pub` and `psmq_sub`:

Start broker and make it log to file

```
nsh> psmqd -b/brok -p/sd/psmqd/psmqd.log
```

Start subscribe thread that will read all messages send on `/can/*` topic

```
nsh> psmq_sub -n/sub -b/brok -t/can/* -o/sd/psmq-sub/can.log
n/connected to broker /brok
n/subscribed to /can/*
n/start receiving data
```

Publish some messages

```
nsh> psmq_pub -b/brok -t/can/engine/rpm -m50
nsh> psmq_pub -b/brok -t/adc/volt -m30
nsh> psmq_pub -b/brok -t/can/room/10/temp -m23
nsh> psmq_pub -b/brok -t/pwm/fan1/speed -m300
```

Check out subscribe thread logs

```
nsh> cat /sd/psmq-sub/can.log
```

```
[1970-01-01 00:00:53] topic: /can/engine/rpm, priority: 0, paylen: 3, payload:
[1970-01-01 00:00:53] 0x0000  35 30 00                                         50.
[1970-01-01 00:00:58] topic: /can/room/10/temp, priority: 0, paylen: 3, payload:
[1970-01-01 00:00:58] 0x0000  32 33 00                                         23.
```

As you can see `/adc/volt` and `/pwm/fan1/speed` haven't been received by
subscribe thread.

Content:

- `psmqd` – broker, relays messages between clients.
- `psmq_sub` – listens to specified topics, can be used as logger for
  communication (optional).
- `psmq_pub` – publishes messages directly from shell. Can send binary data, but
  requires pipes, so on nuttx it can only send ASCII.
- `libpsmq` – library used to communicate with the broker and send/receive
  messages.
