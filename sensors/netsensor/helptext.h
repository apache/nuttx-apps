#define HELP_TEXT \
"USAGE:\n    netsensor [options] topic\n\nARGUMENTS:\n    topic          The " \
"uORB this netsensor will publish.\n\nOPTIONS:\n    -h             Display th" \
"is help message and quit.\n    -p <port>      Specify the UDP port to read f" \
"rom. Default: 5555\n    -d <devno>     Specify the device number that will b" \
"e used for the new topic\n                   instance. Default: increment to" \
" next available.\n    -q <len>       Specify the queue length (buffer length" \
") uORB will use to\n                   store published data. Default: 5\n   " \
" -t             If this flag is passed, timestamps will be overwritten by th" \
"e\n                   time the data was received over UDP.\n"
