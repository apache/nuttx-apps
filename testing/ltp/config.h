/****************************************************************************
 * apps/testing/ltp/config.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* include/config.h.  Generated from config.h.in by configure.  */

/* include/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if clone() supports 7 arguments. */

/* #undef CLONE_SUPPORTS_7_ARGS */

/* Define to 1 if you have the <asm/ldt.h> header file. */

/* #undef HAVE_ASM_LDT_H */

/* Define to 1 if you have the <asm/ptrace.h> header file. */

/* #undef HAVE_ASM_PTRACE_H */

/* Define to 1 if you have the __atomic_* compiler builtins */

#define HAVE_ATOMIC_MEMORY_MODEL 1

/* Define to 1 if you have __builtin___clear_cache */

#define HAVE_BUILTIN_CLEAR_CACHE 1

/* Define to 1 if you have the `clnttcp_create' function. */

/* #undef HAVE_CLNTTCP_CREATE */

/* Define to 1 if you have the `clone3' function. */

/* #undef HAVE_CLONE3 */

/* Define to 1 if you have the `copy_file_range' function. */

/* #undef HAVE_COPY_FILE_RANGE */

/* Define to 1 if you have the `daemon' function. */

#define HAVE_DAEMON 1

/* Define to 1 if you have the declaration of `IFLA_NET_NS_PID', and to 0 if
 * you don't.
 */

/* #undef HAVE_DECL_IFLA_NET_NS_PID */

/* Define to 1 if you have the declaration of `MADV_MERGEABLE', and to 0 if
 * you don't.
 */

/* #undef HAVE_DECL_MADV_MERGEABLE */

/* Define to 1 if you have the declaration of `PR_CAPBSET_DROP', and to 0 if
 * you don't.
 */

/* #undef HAVE_DECL_PR_CAPBSET_DROP */

/* Define to 1 if you have the declaration of `PR_CAPBSET_READ', and to 0 if
 * you don't.
 */

/* #undef HAVE_DECL_PR_CAPBSET_READ */

/* Define to 1 if you have the declaration of `PTRACE_GETSIGINFO',
 * and to 0 if you don't.
 */

/* #undef HAVE_DECL_PTRACE_GETSIGINFO */

/* Define to 1 if you have the declaration of `PTRACE_O_TRACEVFORKDONE', and
 * to 0 if you don't.
 */

/* #undef HAVE_DECL_PTRACE_O_TRACEVFORKDONE */

/* Define to 1 if you have the declaration of `PTRACE_SETOPTIONS', and to
 * 0 if you don't.
 */

/* #undef HAVE_DECL_PTRACE_SETOPTIONS */

/* Define to 1 if the system has the type `enum kcmp_type'. */

/* #undef HAVE_ENUM_KCMP_TYPE */

/* Define to 1 if you have the `epoll_pwait' function. */

#define HAVE_EPOLL_PWAIT 1

/* Define to 1 if you have the `execveat' function. */

/* #undef HAVE_EXECVEAT */

/* Define to 1 if you have the `fallocate' function. */

/* #undef HAVE_FALLOCATE */

/* Define to 1 if you have the `fchownat' function. */

/* #undef HAVE_FCHOWNAT */

/* Define to 1 if you have the `fork' function. */

/* #undef HAVE_FORK */

/* Define to 1 if you have the `fsconfig' function. */

/* #undef HAVE_FSCONFIG */

/* Define to 1 if you have the `fsmount' function. */

/* #undef HAVE_FSMOUNT */

/* Define to 1 if you have the `fsopen' function. */

/* #undef HAVE_FSOPEN */

/* Define to 1 if you have the `fspick' function. */

/* #undef HAVE_FSPICK */

/* Define to 1 if you have the `fstatat' function. */

/* #undef HAVE_FSTATAT */

/* Define to 1 if you have the <fts.h> header file. */

/* #undef HAVE_FTS_H */

/* Define to 1 if you have the `getauxval' function. */

/* #undef HAVE_GETAUXVAL */

/* Define to 1 if you have the `getdents' function. */

/* #undef HAVE_GETDENTS */

/* Define to 1 if you have the `getdents64' function. */

/* #undef HAVE_GETDENTS64 */

/* Define to 1 if you have the <ifaddrs.h> header file. */

/* #undef HAVE_IFADDRS_H */

/* Define to 1 if you have the <inttypes.h> header file. */

#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `io_pgetevents' function. */

/* #undef HAVE_IO_PGETEVENTS */

/* Define to 1 if you have `io_set_eventfd' function. */

/* #undef HAVE_IO_SET_EVENTFD */

/* Define to 1 if you have the `io_uring_enter' function. */

/* #undef HAVE_IO_URING_ENTER */

/* Define to 1 if you have the `io_uring_register' function. */

/* #undef HAVE_IO_URING_REGISTER */

/* Define to 1 if you have the `io_uring_setup' function. */

/* #undef HAVE_IO_URING_SETUP */

/* Define to 1 if you have the `kcmp' function. */

/* #undef HAVE_KCMP */

/* Define to 1 if you have the <keyutils.h> header file. */

/* #undef HAVE_KEYUTILS_H */

/* Define to 1 if you have libacl and it's headers installed */

/* #undef HAVE_LIBACL */

/* Define to 1 if you have libaio and it's headers installed. */

/* #undef HAVE_LIBAIO */

/* Define to 1 if you have the <libaio.h> header file. */

/* #undef HAVE_LIBAIO_H */

/* Define to 1 if you have libcap-2 installed. */

/* #undef HAVE_LIBCAP */

/* Define whether libcrypto and openssl headers are installed */

/* #undef HAVE_LIBCRYPTO */

/* Define to 1 if you have libkeyutils installed. */

/* #undef HAVE_LIBKEYUTILS */

/* Define to 1 if you have libmnl library and headers */

/* #undef HAVE_LIBMNL */

/* Define to 1 if you have both SELinux libraries and headers. */

/* #undef HAVE_LIBSELINUX_DEVEL */

/* Define to 1 if you have the <linux/can.h> header file. */

/* #undef HAVE_LINUX_CAN_H */

/* Define to 1 if you have the <linux/cgroupstats.h> header file. */

/* #undef HAVE_LINUX_CGROUPSTATS_H */

/* Define to 1 if you have the <linux/cryptouser.h> header file. */

/* #undef HAVE_LINUX_CRYPTOUSER_H */

/* Define to 1 if you have the <linux/dccp.h> header file. */

/* #undef HAVE_LINUX_DCCP_H */

/* Define to 1 if you have the <linux/fs.h> header file. */

/* #undef HAVE_LINUX_FS_H */

/* Define to 1 if you have the <linux/genetlink.h> header file. */

/* #undef HAVE_LINUX_GENETLINK_H */

/* Define to 1 if you have the <linux/if_alg.h> header file. */

/* #undef HAVE_LINUX_IF_ALG_H */

/* Define to 1 if you have the <linux/if_ether.h> header file. */

/* #undef HAVE_LINUX_IF_ETHER_H */

/* Define to 1 if you have the <linux/if_packet.h> header file. */

/* #undef HAVE_LINUX_IF_PACKET_H */

/* Define to 1 if you have the <linux/keyctl.h> header file. */

/* #undef HAVE_LINUX_KEYCTL_H */

/* Define to 1 if you have the <linux/mempolicy.h> header file. */

/* #undef HAVE_LINUX_MEMPOLICY_H */

/* Define to 1 if you have the <linux/module.h> header file. */

/* #undef HAVE_LINUX_MODULE_H */

/* Define to 1 if you have the <linux/netlink.h> header file. */

/* #undef HAVE_LINUX_NETLINK_H */

/* Define to 1 if you have the <linux/ptrace.h> header file. */

/* #undef HAVE_LINUX_PTRACE_H */

/* Define to 1 if having a valid linux/random.h */

/* #undef HAVE_LINUX_RANDOM_H */

/* Define to 1 if you have the <linux/seccomp.h> header file. */

/* #undef HAVE_LINUX_SECCOMP_H */

/* Define to 1 if you have the <linux/securebits.h> header file. */

/* #undef HAVE_LINUX_SECUREBITS_H */

/* Define to 1 if you have the <linux/signalfd.h> header file. */

/* #undef HAVE_LINUX_SIGNALFD_H */

/* Define to 1 if you have the <linux/taskstats.h> header file. */

/* #undef HAVE_LINUX_TASKSTATS_H */

/* Define to 1 if you have the <linux/tty.h> header file. */

/* #undef HAVE_LINUX_TTY_H */

/* Define to 1 if you have the <linux/types.h> header file. */

/* #undef HAVE_LINUX_TYPES_H */

/* Define to 1 if you have the <linux/userfaultfd.h> header file. */

/* #undef HAVE_LINUX_USERFAULTFD_H */

/* Define to 1 if you have the <memory.h> header file. */

#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdirat' function. */

/* #undef HAVE_MKDIRAT */

/* Define to 1 if you have the `mkdtemp' function. */

#define HAVE_MKDTEMP 1

/* Define to 1 if you have the `mknodat' function. */

/* #undef HAVE_MKNODAT */

/* Define to 1 if you have the <mm.h> header file. */

/* #undef HAVE_MM_H */

/* Define to 1 if you have the `modify_ldt' function. */

/* #undef HAVE_MODIFY_LDT */

/* Define to 1 if you have the `move_mount' function. */

/* #undef HAVE_MOVE_MOUNT */

/* Define to 1 if you have MREMAP_FIXED in <sys/mman.h>. */

/* #undef HAVE_MREMAP_FIXED */

/* Define to 1 if you have the `name_to_handle_at' function. */

/* #undef HAVE_NAME_TO_HANDLE_AT */

/* Define to 1 if you have the <netinet/sctp.h> header file. */

/* #undef HAVE_NETINET_SCTP_H */

/* Define to 1 if you have newer libcap-2 installed. */

/* #undef HAVE_NEWER_LIBCAP */

/* Define to 1 if you have the <numaif.h> header file. */

/* #undef HAVE_NUMAIF_H */

/* Define to 1 if you have the <numa.h> header file. */

/* #undef HAVE_NUMA_H */

/* Define to 1 if you have libnuma and it's headers version >= 2 installed. */

/* #undef HAVE_NUMA_V2 */

/* Define to 1 if you have the `openat' function. */

/* #undef HAVE_OPENAT */

/* Define to 1 if you have the `openat2' function. */

/* #undef HAVE_OPENAT2 */

/* Define to 1 if you have the <openssl/sha.h> header file. */

/* #undef HAVE_OPENSSL_SHA_H */

/* Define to 1 if you have the `open_tree' function. */

/* #undef HAVE_OPEN_TREE */

/* Define to 1 if you have struct perf_event_attr */

/* #undef HAVE_PERF_EVENT_ATTR */

/* Define to 1 if you have the `pidfd_open' function. */

/* #undef HAVE_PIDFD_OPEN */

/* Define to 1 if you have the `pidfd_send_signal' function. */

/* #undef HAVE_PIDFD_SEND_SIGNAL */

/* Define to 1 if you have the `pkey_mprotect' function. */

/* #undef HAVE_PKEY_MPROTECT */

/* Define to 1 if you have the `preadv' function. */

#define HAVE_PREADV 1

/* Define to 1 if you have the `preadv2' function. */

/* #undef HAVE_PREADV2 */

/* Define to 1 if you have the `profil' function. */

/* #undef HAVE_PROFIL */

/* Define to 1 if you have the <pthread.h> header file. */

#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `pwritev' function. */

#define HAVE_PWRITEV 1

/* Define to 1 if you have the `pwritev2' function. */

/* #undef HAVE_PWRITEV2 */

/* Define to 1 if you have the `readlinkat' function. */

/* #undef HAVE_READLINKAT */

/* Define to 1 if you have the `recvmmsg' function. */

/* #undef HAVE_RECVMMSG */

/* Define to 1 if you have the `renameat' function. */

/* #undef HAVE_RENAMEAT */

/* Define to 1 if you have the `renameat2' function. */

/* #undef HAVE_RENAMEAT2 */

/* Define to 1 if you have the `sched_getcpu' function. */

#define HAVE_SCHED_GETCPU 1

/* Define to 1 if you have the <selinux/selinux.h> header file. */

/* #undef HAVE_SELINUX_SELINUX_H */

/* Define to 1 if you have the `sendmmsg' function. */

/* #undef HAVE_SENDMMSG */

/* Define to 1 if you have the `setns' function. */

/* #undef HAVE_SETNS */

/* Define to 1 if you have the `signalfd' function. */

/* #undef HAVE_SIGNALFD */

/* Define to 1 if you have the `sigpending' function. */

#define HAVE_SIGPENDING 1

/* Define to 1 if you have the `splice' function. */

/* #undef HAVE_SPLICE */

/* Define to 1 if you have the `statx' function. */

/* #undef HAVE_STATX */

/* Define to 1 if you have the <stdint.h> header file. */

#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */

#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `stime' function. */

/* #undef HAVE_STIME */

/* Define to 1 if you have the <strings.h> header file. */

#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */

#define HAVE_STRING_H 1

/* Define to 1 if the system has the type `struct acct_v3'. */

/* #undef HAVE_STRUCT_ACCT_V3 */

/* Define to 1 if the system has the type `struct fanotify_event_info_fid'. */

/* #undef HAVE_STRUCT_FANOTIFY_EVENT_INFO_FID */

/* Define to 1 if `fsid.__val' is a member of `struct
 * fanotify_event_info_fid'.
 */

/* #undef HAVE_STRUCT_FANOTIFY_EVENT_INFO_FID_FSID___VAL */

/* Define to 1 if the system has the type
 * `struct fanotify_event_info_header'.
 */

/* #undef HAVE_STRUCT_FANOTIFY_EVENT_INFO_HEADER */

/* Define to 1 if the system has the type `struct file_dedupe_range'. */

/* #undef HAVE_STRUCT_FILE_DEDUPE_RANGE */

/* Define to 1 if the system has the type `struct fs_quota_statv'. */

/* #undef HAVE_STRUCT_FS_QUOTA_STATV */

/* Define to 1 if you have struct f_owner_ex */

/* #undef HAVE_STRUCT_F_OWNER_EX */

/* Define to 1 if the system has the type `struct if_nextdqblk'. */

/* #undef HAVE_STRUCT_IF_NEXTDQBLK */

/* Define to 1 if the system has the type `struct iovec'. */

#define HAVE_STRUCT_IOVEC 1

/* Define to 1 if the system has the type `struct ipc64_perm'. */

/* #undef HAVE_STRUCT_IPC64_PERM */

/* Define to 1 if the system has the type `struct loop_config'. */

/* #undef HAVE_STRUCT_LOOP_CONFIG */

/* Define to 1 if the system has the type `struct mmsghdr'. */

/* #undef HAVE_STRUCT_MMSGHDR */

/* Define to 1 if the system has the type `struct modify_ldt_ldt_s'. */

/* #undef HAVE_STRUCT_MODIFY_LDT_LDT_S */

/* Define to 1 if the system has the type `struct msqid64_ds'. */

/* #undef HAVE_STRUCT_MSQID64_DS */

/* Define to 1 if `aux_head' is a member of `struct perf_event_mmap_page'. */

/* #undef HAVE_STRUCT_PERF_EVENT_MMAP_PAGE_AUX_HEAD */

/* Define to 1 if the system has the type `struct ptrace_peeksiginfo_args'. */

/* #undef HAVE_STRUCT_PTRACE_PEEKSIGINFO_ARGS */

/* Define to 1 if the system has the type `struct pt_regs'. */

/* #undef HAVE_STRUCT_PT_REGS */

/* Define to 1 if the system has the type `struct rlimit64'. */

/* #undef HAVE_STRUCT_RLIMIT64 */

/* Define to 1 if the system has the type `struct semid64_ds'. */

/* #undef HAVE_STRUCT_SEMID64_DS */

/* Define to 1 if the system has the type `struct shmid64_ds'. */

/* #undef HAVE_STRUCT_SHMID64_DS */

/* Define to 1 if `sa_sigaction' is a member of `struct sigaction'. */

#define HAVE_STRUCT_SIGACTION_SA_SIGACTION 1

/* Define to 1 if `ssi_signo' is a member of `struct signalfd_siginfo'. */

/* #undef HAVE_STRUCT_SIGNALFD_SIGINFO_SSI_SIGNO */

/* Define to 1 if the system has the type `struct statx'. */

/* #undef HAVE_STRUCT_STATX */

/* Define to 1 if the system has the type `struct statx_timestamp'. */

/* #undef HAVE_STRUCT_STATX_TIMESTAMP */

/* Define to 1 if `freepages_count' is a member of `struct taskstats'. */

/* #undef HAVE_STRUCT_TASKSTATS_FREEPAGES_COUNT */

/* Define to 1 if `nvcsw' is a member of `struct taskstats'. */

/* #undef HAVE_STRUCT_TASKSTATS_NVCSW */

/* Define to 1 if `read_bytes' is a member of `struct taskstats'. */

/* #undef HAVE_STRUCT_TASKSTATS_READ_BYTES */

/* Define to 1 if the system has the type `struct termio'. */

/* #undef HAVE_STRUCT_TERMIO */

/* Define to 1 if the system has the type `struct tpacket_req3'. */

/* #undef HAVE_STRUCT_TPACKET_REQ3 */

/* Define to 1 if the system has the type `struct user_desc'. */

/* #undef HAVE_STRUCT_USER_DESC */

/* Define to 1 if the system has the type `struct user_regs_struct'. */

/* #undef HAVE_STRUCT_USER_REGS_STRUCT */

/* Define to 1 if `domainname' is a member of `struct utsname'. */

/* #undef HAVE_STRUCT_UTSNAME_DOMAINNAME */

/* Define to 1 if the system has the type `struct xt_entry_match'. */

/* #undef HAVE_STRUCT_XT_ENTRY_MATCH */

/* Define to 1 if the system has the type `struct xt_entry_target'. */

/* #undef HAVE_STRUCT_XT_ENTRY_TARGET */

/* Define to 1 if you have the `syncfs' function. */

/* #undef HAVE_SYNCFS */

/* Define to 1 if you have __sync_add_and_fetch */

#define HAVE_SYNC_ADD_AND_FETCH 1

/* Define to 1 if you have the `sync_file_range' function. */

/* #undef HAVE_SYNC_FILE_RANGE */

/* Define to 1 if you have the <sys/acl.h> header file. */

/* #undef HAVE_SYS_ACL_H */

/* Define to 1 if you have the <sys/capability.h> header file. */

/* #undef HAVE_SYS_CAPABILITY_H */

/* Define to 1 if you have the <sys/epoll.h> header file. */

#define HAVE_SYS_EPOLL_H 1

/* Define to 1 if you have the <sys/fanotify.h> header file. */

/* #undef HAVE_SYS_FANOTIFY_H */

/* Define to 1 if you have the <sys/inotify.h> header file. */

/* #undef HAVE_SYS_INOTIFY_H */

/* Define to 1 if you have the <sys/prctl.h> header file. */

#define HAVE_SYS_PRCTL_H 1

/* Define to 1 if you have the <sys/ptrace.h> header file. */

/* #undef HAVE_SYS_PTRACE_H */

/* Define to 1 if you have the <sys/reg.h> header file. */

/* #undef HAVE_SYS_REG_H */

/* Define to 1 if you have the <sys/shm.h> header file. */

#define HAVE_SYS_SHM_H 1

/* Define to 1 if you have the <sys/signalfd.h> header file. */

/* #undef HAVE_SYS_SIGNALFD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */

#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */

/* #undef HAVE_SYS_TIMERFD_H */

/* Define to 1 if you have the <sys/types.h> header file. */

#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/ustat.h> header file. */

/* #undef HAVE_SYS_USTAT_H */

/* Define to 1 if you have the <sys/utsname.h> header file. */

#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/xattr.h> header file. */

/* #undef HAVE_SYS_XATTR_H */

/* Define to 1 if you have the `tee' function. */

/* #undef HAVE_TEE */

/* Define to 1 if you have the `timerfd_create' function. */

/* #undef HAVE_TIMERFD_CREATE */

/* Define to 1 if you have the `timerfd_gettime' function. */

/* #undef HAVE_TIMERFD_GETTIME */

/* Define to 1 if you have the `timerfd_settime' function. */

/* #undef HAVE_TIMERFD_SETTIME */

/* Define to 1 if you have the <unistd.h> header file. */

#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unshare' function. */

/* #undef HAVE_UNSHARE */

/* Define to 1 if you have the `ustat' function. */

/* #undef HAVE_USTAT */

/* Define to 1 if you have utimensat(2) */

/* #undef HAVE_UTIMENSAT */

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the `vmsplice' function. */

/* #undef HAVE_VMSPLICE */

/* Define to 1 if you have the `xdr_char' function. */

/* #undef HAVE_XDR_CHAR */

/* Define to 1 if you have the <xfs/xqm.h> header file. */

/* #undef HAVE_XFS_XQM_H */

/* Error message when no NUMA support */

#define NUMA_ERROR_MSG "test requires libnuma >= 2 and it's development packages"

/* Name of package */

#define PACKAGE "ltp"

/* Define to the address where bug reports for this package should be sent. */

#define PACKAGE_BUGREPORT "ltp@lists.linux.it"

/* Define to the full name of this package. */

#define PACKAGE_NAME "ltp"

/* Define to the full name and version of this package. */

#define PACKAGE_STRING "ltp LTP_VERSION"

/* Define to the one symbol short name of this package. */

#define PACKAGE_TARNAME "ltp"

/* Define to the home page for this package. */

#define PACKAGE_URL ""

/* Define to the version of this package. */

#define PACKAGE_VERSION "LTP_VERSION"

/* Define to 1 if you have the ANSI C header files. */

#define STDC_HEADERS 1

/* Target is running Linux w/out an MMU */

#define UCLINUX 1

/* Version number of package */

#define VERSION "LTP_VERSION"

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
 * `char[]'.
 */

#define YYTEXT_POINTER 1
