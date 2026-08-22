==========================
``zbus`` ZBus message bus
==========================

Port of the zbus message bus to NuttX, built entirely on native NuttX
primitives: channels (typed shared messages) observed by
listeners, subscribers, message subscribers and async listeners, with
publishers and consumers fully decoupled.

Channels and observers are defined statically in any source file with
declarative macros::

    ZBUS_LISTENER_DEFINE(acc_listener, listener_cb);
    ZBUS_SUBSCRIBER_DEFINE(acc_subscriber, 4);

    ZBUS_CHAN_DEFINE(acc_chan, struct acc_msg, NULL, NULL,
                     ZBUS_OBSERVERS(acc_listener, acc_subscriber),
                     ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0));

The definitions are collected at link time through the NuttX iterable
sections infrastructure (``include/nuttx/iterable_sections.h``): the
board linker script must include ``<nuttx/linker/common-rom.ld>`` inside
its ``.text`` output section, or ``CONFIG_ZBUS_LINKER_INSERT`` may be
used (see its help text for constraints).

Notable differences from the Zephyr original:

* Timeouts in milliseconds: ``ZBUS_NO_WAIT`` (0) / ``ZBUS_FOREVER`` (-1).
* Subscriber queues are kernel message queues (``file_mq_*``) opened
  lazily on first use; ``mq_send`` payload copying replaces the Zephyr
  ``net_buf`` machinery.

Not ported:

* Multi-domain proxy agent (experimental upstream; a NuttX equivalent
  would be built on rpmsg).
* Direct publishing from interrupt handlers -- use the deferred helper
  ``ZBUS_ISR_PUBLISHER_DEFINE``/``zbus_isr_pub()``
  (``CONFIG_ZBUS_ISR_PUBLISHER``) instead.
* Priority boost (HLP) -- enable the native
  ``CONFIG_PRIORITY_INHERITANCE`` instead.
* net_buf pools/pool isolation (obsolete: message queues copy payloads).
* Static/user-provided runtime observer nodes (heap allocation only).

FLAT build required.

Requirements: ``CONFIG_MQ_MAXMSGSIZE`` >=
``CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE`` + ``sizeof(void *)`` when
message subscribers or async listeners are enabled;
``CONFIG_SCHED_LPWORK`` for async listeners.

See also:

* ``apps/examples/zbus`` -- runnable example (``CONFIG_EXAMPLES_ZBUS``).
* ``apps/testing/zbus`` -- cmocka test suite (``CONFIG_TESTING_ZBUS``).
* Full documentation: ``Documentation/applications/system/zbus`` and
  ``Documentation/components/iterable_sections.rst`` in the nuttx
  repository.
