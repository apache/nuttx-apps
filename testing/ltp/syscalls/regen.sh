#!/bin/sh

# this file is modified from "include/lapi/syscalls/regen.sh", which is belonged to
# ltp project, and using to generate the syscalls.h file.
# Here we using this bash script to generate the syscalls.h that working with
# NuttX syscall number and interfaces

output="syscalls.h"
rm -f "${output}".[1-9]*
output_pid="${output}.$$"

max_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null)
: ${max_jobs:=1}

srcdir=${0%/*}

err() {
	echo "$*" 1>&2
	exit 1
}

cat << EOF > "${output_pid}"
/************************************************
 * GENERATED FILE: DO NOT EDIT/PATCH THIS FILE  *
 *  change your arch specific .in file instead  *
 ************************************************/

/*
 * Here we stick all the ugly *fallback* logic for linux
 * system call numbers (those __NR_ thingies).
 *
 * Licensed under the GPLv2 or later, see the COPYING file.
 */

#ifndef __LAPI_SYSCALLS_H__
#define __LAPI_SYSCALLS_H__

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "cleanup.c"

/* the standard syscall() function interface in Linux platform */

int syscall(int syscall_nr, ...);

/* The syscall numbers in the NuttX system are not fixed in advance in
 * the code, but are dynamically generated through the code.
 * All syscall numbers are defined in enums in the dynamically generated code.
 * And if current build not enable the protect build, then syscall
 * is also not available, so for flat build, we should convert the syscall
 * to direct function call
 */

#define ltp_syscall(NR, ...) ({ \\
	int __ret; \\
	if (NR == __LTP__NR_INVALID_SYSCALL) { \\
		errno = ENOSYS; \\
		__ret = -1; \\
	} else { \\
		__ret = syscall(NR, ##__VA_ARGS__); \\
	} \\
	if (__ret == -1 && errno == ENOSYS) { \\
		tst_brkm(TCONF, CLEANUP, \\
			"syscall(%d) " #NR " not supported on your arch", \\
			NR); \\
	} \\
	__ret; \\
})

#define tst_syscall(NR, ...) ({ \\
	int tst_ret; \\
	if (NR == __LTP__NR_INVALID_SYSCALL) { \\
		errno = ENOSYS; \\
		tst_ret = -1; \\
	} else { \\
		tst_ret = syscall(NR, ##__VA_ARGS__); \\
	} \\
	if (tst_ret == -1 && errno == ENOSYS) { \\
		tst_brk(TCONF, "syscall(%d) " #NR " not supported", NR); \\
	} \\
	tst_ret; \\
})

/* Actually, the system call numbers we define are just for identification.
 * The bottom layer of vela will not actually use these system call numbers.
 * Because in the ltp test case, we also initiate real system calls directly
 * through function calls.
 * So the system call number here can be defined arbitrarily, as long as it
 * is unique
 */

EOF

jobs=0

(
echo "Generating data for NuttX ... "

(
echo
while read line ; do
	set -- ${line}
	nr="__NR_$1"
	shift
	if [ $# -eq 0 ] ; then
		err "invalid line found: $line"
	fi
	echo "#ifndef ${nr}"
	echo "#  define $*"
	echo "#  define ${nr} $1"
	echo "#endif"
done < "${srcdir}/nuttx.in"
echo
) >> "${output_pid}.nuttx"

) &

jobs=$(( jobs + 1 ))
if [ ${jobs} -ge ${max_jobs} ] ; then
	wait || exit 1
	jobs=0
fi

echo "Generating stub list ... "
(
echo
echo "/* Common stubs */"
echo "#define __LTP__NR_INVALID_SYSCALL -1" >> "${output_pid}"
for nr in $(awk '{print $1}' "${srcdir}/"*.in | sort -u) ; do
	nr="__NR_${nr}"
	echo "#ifndef ${nr}"
	echo "#  define ${nr} __LTP__NR_INVALID_SYSCALL"
	echo "#endif"
done
echo "#endif"
) >> "${output_pid}._footer"

wait || exit 1

printf "Combining them all ... "
for arch in nuttx _footer ; do
	cat "${output_pid}.${arch}"
done >> "${output_pid}"
mv "${output_pid}" "../${output}"
rm -f "${output_pid}"*
echo "OK!"
