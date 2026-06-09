==============
iptables
==============

Name
----

iptables — administration tool for IPv4 packet filtering and NAT

Synopsis
--------

.. code-block:: text

   iptables -t table -[AD] chain rule-specification
   iptables -t table -I chain [rulenum] rule-specification
   iptables -t table -D chain rulenum
   iptables -t table -P chain target
   iptables -t table -[FL] [chain]

Description
-----------

iptables is used to set up, maintain, and inspect the tables of IPv4 packet
filter rules in the NuttX kernel. Several different tables may be defined.
Each table contains a number of built-in chains and may also contain
user-defined chains.

Each chain is a list of rules which can match a set of packets. Each rule
specifies what to do with a packet that matches. This is called a `target`,
which may be a jump to a user-defined chain in the same table.

Targets
-------

A firewall rule specifies criteria for a packet and a target. If the packet
does not match, the next rule in the chain is examined; if it does match,
then the next rule is specified by the value of the target, which can be the
name of a user-defined chain or one of the special values ``ACCEPT``,
``DROP``, or ``RETURN``.

``ACCEPT``
   Means to let the packet through.

``DROP``
   Means to drop the packet on the floor.

``RETURN``
   Means stop traversing this chain and resume at the next rule in the
   previous (calling) chain. If the end of a built-in chain is reached or a
   rule with target ``RETURN`` is matched, the target specified by the chain
   policy determines the fate of the packet.

Tables
------

There is only one table in NuttX:

``filter``
   This is the default table (if no ``-t`` option is passed). It contains
   the built-in chains:

   - ``INPUT`` (for packets destined for local sockets)
   - ``FORWARD`` (for packets being routed through the box)
   - ``OUTPUT`` (for locally-generated packets)

Commands
--------

These options specify the desired action to perform. Only one of them can
be specified on the command line, unless otherwise specified below.

``-A, --append chain rule-specification``
   Append one or more rules to the end of the selected chain.

``-D, --delete chain rule-specification``
   Delete one or more rules from the selected chain. There are two versions
   of this command: the rule can be specified as a number (starting from 1)
   or as a rule specification.

``-I, --insert chain [rulenum] rule-specification``
   Insert one or more rules in the selected chain as giving rule number.
   The rule number is specified as the first argument. If no rule number is
   specified, the rule is inserted at the top of the chain.

``-L, --list [chain]``
   List all rules in the selected chain. If no chain is selected, all chains
   are listed.

``-F, --flush [chain]``
   Flush the selected chain (all the chains in the table if none is given).
   This is the same as deleting all the rules one by one.

``-P, --policy chain target``
   Set the policy for the built-in chain to the given target. The policy
   target must be either ``ACCEPT`` or ``DROP``.

Options
-------

The following parameters make up a rule specification (as used in the add,
delete, insert, append, and replace commands).

``-t, --table table``
   Specify the table to manipulate. The default is ``filter``.

``-p, --protocol [!] protocol``
   The protocol of the rule or of the packet to check. The specified
   protocol can be one of ``tcp``, ``udp``, ``icmp``, or ``all``. The
   number 0 is equivalent to ``all``.

``-s, --source [!] address[/mask]``
   Source specification. Address can be either a network name, a hostname
   (please note that specifying any name to be resolved with a remote query
   such as DNS is a really bad idea), a network IP address (with /mask), or
   a plain IP address.

``-d, --destination [!] address[/mask]``
   Destination specification. See the description of the ``-s`` (source)
   flag for a detailed description of the syntax.

``-j, --jump target``
   This specifies the target of the rule; i.e., what to do if the packet
   matches it. The target can be a user-defined chain (other than the one
   this rule is in), one of the special targets ``ACCEPT``, ``DROP``, or
   ``RETURN``.

``-i, --in-interface [!] name``
   Name of an interface via which a packet was received. The ``+`` wildcard
   can be used to match all interfaces.

``-o, --out-interface [!] name``
   Name of an interface via which a packet is going to be sent. The ``+``
   wildcard can be used to match all interfaces.

``--source-port, --sport [!] port[:port]``
   Source port or port range specification. This can either be a service
   name or a port number. The ``port:port`` form specifies a range.

``--destination-port, --dport [!] port[:port]``
   Destination port or port range specification. See the description of the
   ``--source-port`` flag for a detailed description of the syntax.

``--icmp-type [!] typename``
   This allows specification of the ICMP type, which can be a numeric
   ICMP type or a command name.

Examples
--------

Append a rule to the INPUT chain to accept all incoming TCP traffic:

.. code-block:: text

   NuttShell (NSH) NuttX-12.x
   nsh> iptables -A INPUT -p tcp -j ACCEPT

Insert a rule at position 1 in the INPUT chain to drop traffic from a
specific source:

.. code-block:: text

   nsh> iptables -I INPUT 1 -s 192.168.1.100 -j DROP

List all rules in the filter table:

.. code-block:: text

   nsh> iptables -L
   Chain INPUT (policy ACCEPT)
   target     prot opt source               destination
   DROP       all  --  192.168.1.100        anywhere
   ACCEPT     tcp  --  anywhere             anywhere

   Chain FORWARD (policy ACCEPT)
   target     prot opt source               destination

   Chain OUTPUT (policy ACCEPT)
   target     prot opt source               destination

Delete a rule by number:

.. code-block:: text

   nsh> iptables -D INPUT 1

Flush all chains:

.. code-block:: text

   nsh> iptables -F

Set the default policy for the INPUT chain to DROP:

.. code-block:: text

   nsh> iptables -P INPUT DROP

Accept incoming HTTP traffic on port 80:

.. code-block:: text

   nsh> iptables -A INPUT -p tcp --dport 80 -j ACCEPT

Accept incoming SSH traffic on port 22:

.. code-block:: text

   nsh> iptables -A INPUT -p tcp --dport 22 -j ACCEPT

Drop all incoming ICMP traffic:

.. code-block:: text

   nsh> iptables -A INPUT -p icmp -j DROP

Configuration
-------------

This command requires the following configuration options:

- :kconfig:option:`CONFIG_SYSTEM_IPTABLES` - Enable the iptables command
- :kconfig:option:`CONFIG_NET_IPTABLES` - Enable netfilter/iptables support
- :kconfig:option:`CONFIG_NET_IPv4` - Enable IPv4 support

Optional configuration:

- :kconfig:option:`CONFIG_SYSTEM_IPTABLES_PRIORITY` - Task priority
  (default: 100)
- :kconfig:option:`CONFIG_SYSTEM_IPTABLES_STACKSIZE` - Stack size
  (default: DEFAULT_TASK_STACKSIZE)
- :kconfig:option:`CONFIG_SYSTEM_IPTABLES_LOCK_FILE_PATH` - Lock file
  path to prevent concurrent overwrite (default: ``/tmp/iptables.lock``)

See Also
---------

:doc:`ip6tables`

.. note::
   This man page is based on the iptables implementation in NuttX
   ``apps/system/iptables/iptables.c`` and ``iptables_utils.c``.
