System / `trace` Task Tracer
============================

Task Tracer is the tool to collect the various events in the NuttX kernel and display the result graphically.

It can collect the following events.

- Task execution, termination, switching
- System call enter/leave
- Interrupt handler enter/leave

# Installation

## Install Trace Compass

Task Tracer uses the external tool "Trace Compass" to display the trace result.

Download it from https://www.eclipse.org/tracecompass/ and install into the host environment.

After the installation, execute it and choose `Tools` -> `add-ons` menu, then select `Install Extensions` to install the extension named "Trace Compass ftrace (Incubation)".

## NuttX kernel configuration

To enable the task trace function, the NuttX kernel configuration needs to be modified.

The following configurations must be enabled.

- `CONFIG_SCHED_INSTRUMENTATION`
- `CONFIG_SCHED_INSTRUMENTATION_FILTER`
- `CONFIG_SCHED_INSTRUMENTATION_SYSCALL`
- `CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER`
- `CONFIG_DRIVER_NOTE`
- `CONFIG_DRIVER_NOTERAM`
- `CONFIG_DRIVER_NOTECTL`
- `CONFIG_SYSTEM_TRACE`
- `CONFIG_SYSTEM_SYSTEM`

The following configurations are configurable parameters for trace. 

- `CONFIG_SCHED_INSTRUMENTATION_FILTER_DEFAULT_MODE`
  - Specify the default filter mode.
    If the following bits are set, the corresponding instrumentations are enabled on boot.
    - Bit 0 = Enable instrumentation
    - Bit 1 = Enable syscall instrumentation
    - Bit 2 = Enable IRQ instrumentation

- `CONFIG_SCHED_INSTRUMENTATION_NOTERAM_BUFSIZE`
  - Specify the note buffer size in bytes.
    Higher value can hold more note records, but consumes more kernel memory.

- `CONFIG_SCHED_INSTRUMENTATION_NOTERAM_DEFAULT_NOOVERWRITE`
  - If enabled, stop overwriting old notes in the circular buffer when the buffer is full by default.
    This is useful to keep instrumentation data of the beginning of a system boot.

After the configuration, rebuild the NuttX kernel and application.

If the trace function is enabled, "`trace`" built-in command is available.

# How to get trace data

The trace function can be controlled by "`trace`" command.

## Quick Guide

### Getting the trace

Trace is started by the following command.
```
nsh> trace start
```

Trace is stopped by the following command.
```
nsh> trace stop
```

If you want to get the trace while executing some command, the following command can be used.
```
nsh> trace cmd <command> [<args>...]
```

### Displaying the trace result

The trace result is accumulated in the memory.
After getting the trace, the following command displays the accumulated trace data to the console.
```
nsh> trace dump
```
This will be get the trace results like the followings:
```
<noname>-1   [0]   7.640000000: sys_close()
<noname>-1   [0]   7.640000000: sys_close -> 0
<noname>-1   [0]   7.640000000: sys_sched_lock()
<noname>-1   [0]   7.640000000: sys_sched_lock -> 0
<noname>-1   [0]   7.640000000: sys_nxsched_get_stackinfo()
<noname>-1   [0]   7.640000000: sys_nxsched_get_stackinfo -> 0
<noname>-1   [0]   7.640000000: sys_sched_unlock()
<noname>-1   [0]   7.640000000: sys_sched_unlock -> 0
<noname>-1   [0]   7.640000000: sys_clock_nanosleep()
<noname>-1   [0]   7.640000000: sched_switch: prev_comm=<noname> prev_pid=1 prev_state=S ==> next_comm=<noname> next_pid=0
<noname>-0   [0]   7.640000000: irq_handler_entry: irq=11
<noname>-0   [0]   7.640000000: irq_handler_exit: irq=11
<noname>-0   [0]   7.640000000: irq_handler_entry: irq=15
<noname>-0   [0]   7.650000000: irq_handler_exit: irq=15
<noname>-0   [0]   7.650000000: irq_handler_entry: irq=15
    :
```

By using the logging function of your terminal software, the trace result can be saved into the host environment and it can be used as the input for "Trace Compass".

If the target has a storage, the trace result can be stored into the file by using the following command.
It also can be used as the input for "Trace Compass" by transferring the file in the target device to the host.
```
nsh> trace dump <file name>
```

To display the trace result by "Trace Compass", choose `File` -> `Open Trace` menu to specify the trace data file name.


## Trace command description

### **trace start**
Start task tracing
```
trace start [-c][<duration>]
```
- `-c` : Continue the previous trace.
The trace data is not cleared before starting new trace.
- `<duration>` : Specify the duration of the trace by seconds.
Task tracing is stopped after the specified period.
If not specified, the tracing continues until stopped by the command.

### **trace stop**
Stop task tracing
```
trace stop
```

### **trace cmd**
Get the trace while running the specified command.
After the termination of the command, task tracing is stopped.
To use this command, `CONFIG_SYSTEM_SYSTEM` needs to be enabled.

```
trace cmd [-c] <command> [<args>...]
```
- `-c` : Continue the previous trace.
The trace data is not cleared before starting new trace.
- `<command>` : Specify the command to get the task trace.
- `<args>` : Arguments for the command.

Example:
```
nsh> trace cmd sleep 1
```

### **trace dump**
Output the trace result.
If the task trace is running, it is stopped before the output.
```
trace dump [-c][<filename>]
```
- `-c` : Not stop tracing before the output.
Because dumping trace itself is a task activity and new trace data is added while output, the dump will never stop.
- `<filename>` : Specify the filename to save the trace result.
If not specified, the trace result is displayed to console.


### **trace mode**
Set the task trace mode options.
The default value is given by the kernel configuration `CONFIG_SCHED_INSTRUMENTATION_FILTER_DEFAULT_MODE`.
```
trace mode [{+|-}{o|s|i}...]
```
- `+o` : Enable overwrite mode.
The trace buffer is a ring buffer and it can overwrite old data if no free space is available in the buffer.
Enables this behavior.

- `-o` : Disable overwrite mode.
The new trace data will be disposed when the buffer is full.
This is useful to keep the data of the beginning of the trace.

- `+s` : Enable system call trace.
It records the event of enter/leave system call which is issued by the application.
All system calls are recorded by default. `trace syscall` command can filter the system calls to be recorded.

- `-s` : Disable system call trace.

- `+i` : Enable interrupt trace.
It records the event of enter/leave interrupt handler which is occured while the tracing.
All IRQs are recorded by default. `trace irq` command can filter the IRQs to be recorded.

- `-i` : Disable interrupt trace.

If no command parameters are specified, display the current mode as the follows.

Example:
```
nsh> trace mode
Task trace mode:
 Trace                   : enabled
 Overwrite               : on  (+o)
 Syscall trace           : on  (+s)
  Filtered Syscalls      : 16
 IRQ trace               : on  (+i)
  Filtered IRQs          : 2
```

### **trace syscall**
Configure the filter of the system call trace.
```
trace syscall [{+|-}<syscallname>...]
```
- `+<syscallname>` : Add the specified system call name to the filter.
The execution of the filtered system call is not recorded into the trace data.<p>

- `-<syscallname>` : Remove the specified system call name from the filter.

Wildcard "`*`" can be used to specify the system call name.
For example, "`trace syscall +sem_*`" filters the system calls begin with "`sem_`", such as `sem_post()`, `sem_wait()`,...

If no command parameters are specified, display the current filter settings as the follows.

Example:
```
nsh> trace syscall
Filtered Syscalls: 16
  getpid
  sem_destroy
  sem_post
  sem_timedwait
  sem_trywait
  sem_wait
  mq_close
  mq_getattr
  mq_notify
  mq_open
  mq_receive
  mq_send
  mq_setattr
  mq_timedreceive
  mq_timedsend
  mq_unlink
```

### **trace irq**
Configure the filter of the interrupt trace.
```
trace irq [{+|-}<irqnum>...]
```
- `+<irqnum>` : Add the specified IRQ number to the filter.
The execution of the filtered IRQ handler is not recorded into the trace data.

- `-<irqnum>` : Remove the specified IRQ number from the filter.

Wildcard "`*`" can be used to specify all IRQs.

If no command parameters are specified, display the current filter settings as the follows.

Example:
```
nsh> trace irq
Filtered IRQs: 2
  11
  15
```
