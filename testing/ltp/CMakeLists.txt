# ##############################################################################
# apps/testing/ltp/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_TESTING_LTP)
  set(LTPS_VERSION 20230516)
  set(LTP_DOWNLOAD_URL
      https://github.com/linux-test-project/ltp/archive/refs/tags/)
  set(LTP_UNPACK ${CMAKE_CURRENT_SOURCE_DIR}/ltp)
  set(LTP_URL https://github.com/linux-test-project/ltp.git)

  set(TESTDIR ${LTP_UNPACK}/testcases/open_posix_testsuite)

  if(NOT EXISTS ${TESTDIR})
    FetchContent_Declare(
      ltp_fetch
      DOWNLOAD_NAME "ltp-${LTPS_VERSION}.zip"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "${LTP_DOWNLOAD_URL}${LTPS_VERSION}.zip" SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/ltp BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/testing/ltp/ltp
      PATCH_COMMAND
        patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-pthread_rwlock_unlock-follow-linux.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0002-Use-ifdef-instead-of-if-for-__linux__.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0003-LTP-Re-init-static-variable-to-avoid-errors-during-m.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0004-test-update-ltp-test-case.patch && patch
        -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0005-ltp-update-pthread_cond_timedwait-testcase.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0006-test-modified-ltp-rwlock-need-init.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0007-pthread_kill-add-usleep-aviod-semcount-overturn.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0008-test-ltp-fix-ltp_interfaces_sigaction_23_10-deadloop.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0009-ltp-modify-user-code-for-fdcheck-compatibility.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0010-ltp-fix-build-warning.patch && patch -p1
        -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0011-ltp-fix-the-proc.h-header-file-duplicate-inclusion.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0012-ltp-fix-build-error.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(ltp_fetch)
    if(NOT ltp_fetch_POPULATED)
      FetchContent_Populate(ltp_fetch)
    endif()
  endif()

  # the BLACKWORDS are shared for posix testcases and kernel testcases
  set(BLACKWORDS)
  if(NOT CONFIG_FS_AIO)
    list(APPEND BLACKWORDS "aio.h" "SIGPOLL")
  endif()
  list(APPEND BLACKWORDS "pthread_mutexattr_setprioceiling"
       "pthread_mutexattr_getprioceiling" "pthread_getattr_np"
       "pthread_mutex_getprioceiling")

  if(NOT CONFIG_PTHREAD_SPINLOCKS)
    list(APPEND BLACKWORDS "pthread_spin_init" "pthread_spin_destroy"
         "pthread_spin_trylock")
  endif()
  list(
    APPEND
    BLACKWORDS
    "CHILD_MAX"
    "setpgid("
    "PTHREAD_SCOPE_PROCESS"
    "setpgrp"
    "threads_scenarii.c"
    "pthread_mutex_lock"
    "ucontext.h"
    "msync"
    "lfind")

  if(NOT CONFIG_FS_TMPFS)
    list(APPEND BLACKWORDS "lio_listio")
  endif()

  if(NOT CONFIG_SIG_EVTHREAD)
    list(APPEND BLACKWORDS "fork")
  endif()

  if(NOT CONFIG_MQ_MAXMSGSIZE GREATER_EQUAL 64)
    list(
      APPEND
      BLACKWORDS
      "mq_close"
      "mq_getattr"
      "mq_notify"
      "mq_open"
      "mq_receive"
      "mq_send"
      "mq_setattr"
      "mq_timedreceive"
      "mq_timedsend"
      "mq_unlink")
  endif()

  if(NOT CONFIG_PTHREAD_MUTEX_TYPES)
    list(APPEND BLACKWORDS "pthread_mutexattr_settype")
  endif()

  if(NOT CONFIG_SIG_DEFAULT)
    list(APPEND BLACKWORDS "SIGKILL" "SIGSTOP")
  endif()

  if(NOT CONFIG_RR_INTERVAL GREATER 0)
    list(APPEND BLACKWORDS "SCHED_RR")
  endif()

  if(NOT CONFIG_FS_NAMED_SEMAPHORES)
    list(APPEND BLACKWORDS "sem_open" "sem_close" "sem_unlink")
  endif()

  if(NOT CONFIG_PIPES)
    list(APPEND BLACKWORDS "pipe")
  endif()

  if(NOT CONFIG_PTHREAD_SPINLOCKS)
    list(APPEND BLACKWORDS "pthread_spin_lock")
  endif()

  if(NOT CONFIG_SCHED_WAITPID)
    list(APPEND BLACKWORDS "waitpid")
  endif()

  if(NOT CONFIG_SCHED_WAITPID OR NOT CONFIG_SCHED_HAVE_PARENT)
    list(APPEND BLACKWORDS "wait")
  endif()

  if(NOT CONFIG_SCHED_USER_IDENTITY)
    list(
      APPEND
      BLACKWORDS
      "setuid"
      "setgid"
      "getuid"
      "getgid"
      "seteuid"
      "setegid"
      "geteuid"
      "getegid")
  endif()

  if(CONFIG_TLS_NELEM EQUAL 0)
    list(APPEND BLACKWORDS "pthread_key_create" "pthread_key_delete"
         "pthread_setspecific" "pthread_getspecific")
  endif()

  if(CONFIG_TLS_NCLEANUP EQUAL 0)
    list(APPEND BLACKWORDS "pthread_cleanup_push" "pthread_cleanup_pop")
  endif()

  if(NOT CONFIG_LIBC_EXECFUNCS)
    list(APPEND BLACKWORDS "execl" "execvp" "execlp" "execvpe")
  endif()

  if(NOT CONFIG_FS_SHMFS)
    list(APPEND BLACKWORDS "shm_open" "shm_unlink")
  endif()

  if(NOT LIBC_LOCALE_CATALOG)
    list(APPEND BLACKWORDS "catopen" "catgets" "catclose")
  endif()

  if(NOT CONFIG_MM_SHM)
    list(APPEND BLACKWORDS "shmget" "shmat")
  endif()

  add_compile_options(
    -Wall
    -Wno-strict-prototypes
    -Wno-return-type
    -Wno-format
    -Wno-uninitialized
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-unused-value
    -Wno-int-conversion
    -Wno-shadow
    -Wno-incompatible-pointer-types
    -Wno-overflow
    -Wno-int-to-pointer-cast)

  if(APPLE)
    add_compile_options(-Wno-integer-overflow -Wno-absolute-value)
  endif()

  # to build the posix testcases
  if(EXISTS ${TESTDIR})
    set(BLACKSRCS)

    list(
      APPEND
      BLACKSRCS
      ${TESTDIR}/conformance/definitions/time_h/19-1-buildonly.c
      ${TESTDIR}/conformance/definitions/signal_h/21-1-buildonly.c
      ${TESTDIR}/conformance/definitions/signal_h/27-1-buildonly.c
      ${TESTDIR}/conformance/definitions/time_h/19-1-buildonly.c
      ${TESTDIR}/conformance/interfaces/sem_post/8-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/9-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/2-2.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/10-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-5.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-3.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/2-3.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-4.c
      ${TESTDIR}/conformance/interfaces/pthread_sigmask/18-1.c
      ${TESTDIR}/conformance/interfaces/pthread_sigmask/4-1.c
      ${TESTDIR}/conformance/interfaces/sigprocmask/4-1.c)

    if(CONFIG_FDCHECK)
      list(APPEND BLACKSRCS ${TESTDIR}/conformance/interfaces/mq_close/3-1.c
           ${TESTDIR}/conformance/interfaces/mq_close/4-1.c)
    endif()

    if(NOT CONFIG_LIBC_LOCALTIME)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/definitions/time_h/34-1-buildonly.c
           ${TESTDIR}/conformance/definitions/time_h/35-3-buildonly.c)
    endif()

    list(
      APPEND
      BLACKSRCS
      ${TESTDIR}/conformance/definitions/time_h/35-1-buildonly.c
      ${TESTDIR}/conformance/definitions/time_h/35-2-buildonly.c
      ${TESTDIR}/stress/threads/pthread_kill/stress.c)

    if(NOT CONFIG_PTHREAD_SPINLOCKS)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/definitions/pthread_h/3-12-buildonly.c)
    endif()

    if(NOT CONFIG_SCHED_CHILD_STATUS)
      list(
        APPEND BLACKSRCS ${TESTDIR}/conformance/interfaces/pthread_exit/6-1.c
        ${TESTDIR}/conformance/behavior/WIFEXITED/1-1.c
        ${TESTDIR}/conformance/behavior/WIFEXITED/1-2.c)
    endif()

    if(NOT CONFIG_SCHED_SPORADIC)
      list(
        APPEND
        BLACKSRCS
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-2.c
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-3.c
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-4.c)
    endif()

    if(NOT CONFIG_LIBC_MAX_EXITFUNS GREATER_EQUAL 32)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_exit/4-1.c
           ${TESTDIR}/conformance/interfaces/pthread_exit/5-1.c)
    endif()

    if(NOT CONFIG_CANCELLATION_POINTS)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_exit/3-1.c)
    endif()

    if(NOT CONFIG_TLS_NELEM GREATER_EQUAL 16)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_setspecific/1-1.c)
    endif()

    file(GLOB_RECURSE LTP_ORIGS ${TESTDIR}/*.c)

    if(LTP_ORIGS)
      foreach(word IN LISTS BLACKWORDS)
        foreach(file ${LTP_ORIGS})
          file(STRINGS ${file} content)
          string(FIND "${content}" "${word}" pos)
          if(pos GREATER -1)
            list(APPEND BLACKLIST ${file})
          endif()
        endforeach()
      endforeach()
    endif()

    foreach(src ${BLACKSRCS})
      foreach(orig ${LTP_ORIGS})
        string(REGEX MATCH "${src}$" matched ${orig})
        if(matched)
          list(APPEND BLACKLIST ${orig})
        endif()
      endforeach()
    endforeach()

    foreach(ITEM ${BLACKLIST})
      list(REMOVE_ITEM LTP_ORIGS ${ITEM})
    endforeach()

    set(LTP_MAINCSRCS "")
    if(LTP_ORIGS)
      foreach(file ${LTP_ORIGS})
        file(STRINGS ${file} content)
        string(FIND "${content}" "main(" pos)
        if(pos GREATER -1)
          list(APPEND LTP_MAINCSRCS ${file})
        endif()
      endforeach()
    endif()

    set(LTP_CSRCS ${LTP_ORIGS})
    foreach(ITEM ${LTP_MAINCSRCS})
      list(REMOVE_ITEM LTP_CSRCS ${ITEM})
    endforeach()

    set(PROGNAME "")
    if(LTP_MAINCSRCS)
      foreach(file ${LTP_MAINCSRCS})
        string(REPLACE "/" ";" file_parts ${file})
        list(LENGTH file_parts num_parts)
        if(num_parts GREATER 2)
          list(GET file_parts -3 part1)
          list(GET file_parts -2 part2)
          list(GET file_parts -1 part3)
          string(REPLACE "-" "_" part1 ${part1})
          string(REPLACE "-" "_" part2 ${part2})
          string(REPLACE "-" "_" part3 ${part3})
          set(prog "ltp_${part1}_${part2}_${part3}")
          get_filename_component(progg ${prog} NAME_WE)
          list(APPEND PROGNAME ${progg})
        endif()
      endforeach()
    endif()

    if(CONFIG_NET_CAN)
      list(APPEND PROGNAME "can_rcv_own_msgs" "can_filter")
      list(APPEND LTP_MAINCSRCS
           "${TESTDIR}/../network/can/filter-tests/can_filter.c"
           "${TESTDIR}/../network/can/filter-tests/can_rcv_own_msgs.c")
    endif()

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${TESTDIR}/include)
    include_directories(${TESTDIR}/../../../include)
    include_directories(${TESTDIR}/../../include)

    list(LENGTH LTP_MAINCSRCS SOURCE_FILES_COUNT)
    math(EXPR LAST_INDEX "${SOURCE_FILES_COUNT} - 1")

    foreach(INDEX RANGE 0 ${LAST_INDEX})
      list(GET LTP_MAINCSRCS ${INDEX} SOURCE_FILES)
      list(GET PROGNAME ${INDEX} EXECUTABLE_NAMES)
      nuttx_add_application(
        NAME
        ${EXECUTABLE_NAMES}
        PRIORITY
        SCHED_PRIORITY_DEFAULT
        STACKSIZE
        ${CONFIG_TESTING_LTP_STACKSIZE}
        SRCS
        ${SOURCE_FILES})
    endforeach()
  endif()

  # ############################################################################
  # to build the linux kernel testcases
  set(TESTLINUX_DIR ${LTP_UNPACK}/testcases/kernel)

  if(EXISTS ${TESTLINUX_DIR})
    list(
      APPEND
      BLACKWORDS
      "connectors"
      "netlink"
      "cgroup"
      "cpuctl"
      "scsi" # nuttx do not support scsi device driver
      "RTC_"
      "V4L2"
      "v4l2"
      "TUSB_MAJOR"
      "tagp"
      "tmod"
      "LO_FLAGS_AUTOCLEAR"
      "IFF_ONE_QUEUE"
      "LO_FLAGS_READ_ONLY"
      "loopinfo"
      "sigjmp_buf"
      "FIBMAP"
      "block_dev_handle"
      "drm"
      "tst_path_has_mnt_flags"
      "personality" # nuttx do not support execution domains api
      "getcontext"
      "fmtmsg"
      "MS_BIND" # nuttx mount do not support MS_BIND flags
      "sound"
      "tracing"
      "CLONE_NEWNS"
      "CLONE_NEWPID"
      "CLONE_NEWUSER"
      "CLONE_NEWIPC"
      "CLONE_NEWUTS"
      "CLONE_NEWNET"
      "CLONE_NEWIPC"
      "NAMESPACE"
      "cap_last_cap"
      "crypto"
      "NUMA")

    if(NOT CONFIG_LIBC_EXECFUNCS)
      list(APPEND BLACKWORDS "execv" "execve")
    endif()

    list(
      APPEND
      BLACKWORDS
      "bitmask_alloc"
      "input"
      "msgctl" # nuttx do not support System V IPC api
      "semget"
      "semctl"
      "shmctl"
      "logging"
      "mce-test"
      "numa"
      "hugetlb" # nuttx do not support hugetlb
      "tomoyo" # nuttx do not support tomoyo security module
      "posix_fadvise" # nuttx do not support posix_fadvise syscall
      "sysctl" # nuttx do not support sysctl syscall
      "getgroups" # nuttx do not support getgroups syscall
      "setgroups" # nuttx do not support setgroups syscall
      "getdents" # nuttx do not support getdents syscall
      "rt_sigaction" # nuttx do not support rt_sigaction syscall
      "socketcall" # nuttx do not support socketcall syscall
      "readahead" # nuttx do not support readahead syscall
      "capget" # nuttx do not support capget syscall
      "BLKROSET" # nuttx do not support BLKROSET flag in ioctl syscall
      "quotactl" # nuttx do not support quotactl syscall
      "swapon" # nuttx do not support swapon syscall
      "swapoff" # nuttx do not support swapoff syscall
      "bpf" # nuttx do not support bpf syscall
      "mincore" # nuttx do not support mincore syscall
      "unshare" # nuttx do not support unshare syscall
      "kcmp" # nuttx do not support kcmp syscall
      "clone" # nuttx do not support clone syscall
      "capset" # nuttx do not support capset syscall
      "adjtimex" # nuttx do not support adjtimex syscall
      "confstr" # nuttx do not support confstr syscall
      "fallocate" # nuttx do not support fallocate syscall, and only support
                  # posix_fallocate syscall
      "rt_tgsigqueueinfo" # nuttx do not support rt_tgsigqueueinfo syscall
      "sync_file_range" # nuttx do not support sync_file_range syscall
      "vhangup" # nuttx do not support vhangup syscall
      "remap_file_pages" # nuttx do not support remap_file_pages syscall
      "acct" # nuttx do not support acct syscall
      "gethostid" # nuttx do not support gethostid syscall
      "mallopt" # nuttx do not support malloopt syscall
    )

    if(NOT CONFIG_TIMER_FD)
      list(APPEND BLACKWORDS "timerfd_create" "timerfd_gettime")
    endif()

    list(APPEND BLACKWORDS "renameat2" # nuttx do not support renameat2 syscall
    )

    if(NOT CONFIG_PSEUDOTERM)
      list(APPEND BLACKWORDS "unlockpt" "grantpt")
    endif()

    list(APPEND BLACKWORDS "eaccess" # nuttx do not support eaccess api
         "mremap" # nuttx do not support mremap api
    )

    if(NOT CONFIG_SMP)
      list(APPEND BLACKWORDS "sched_getaffinity")
    endif()

    if(NOT CONFIG_ARCH_ADDRENV
       OR NOT CONFIG_MM_PGALLOC
       OR NOT CONFIG_ARCH_USE_MMU)
      list(APPEND BLACKWORDS "sbrk")
    endif()

    list(
      APPEND
      BLACKWORDS
      "RLIMIT_RSS"
      "RLIMIT_NPROC"
      "RLIMIT_LOCKS"
      "RLIMIT_MSGQUEUE"
      "RLIMIT_SIGPENDING"
      "modify_ldt" # nuttd do not support modify_ldt syscall interface
      "SIGPWR"
      "_SC_PASS_MAX"
      "_SC_XOPEN_XCU_VERSION"
      "UL_GETFSIZE"
      "setresuid"
      "setresgid"
      "getsid"
      "getresuid"
      "getresgid")

    if(NOT CONFIG_PSEUDOFS_SOFTLINKS)
      list(APPEND BLACKWORDS "symlink" "link")
    endif()

    if(NOT CONFIG_FS_LARGEFILE)
      list(APPEND BLACKWORDS "off64_t" "fpos64_t" "F_GETLK64")
      list(APPEND BLACKWORDS "F_SETLK64" "F_SETLKW64" "flock64")
      list(APPEND BLACKWORDS "open64" "openat64" "creat64")
      list(APPEND BLACKWORDS "fallocate64" "posix_fadvise64"
           "posix_fallocate64")
      list(APPEND BLACKWORDS "lseek64" "pread64" "pwrite64")
      list(APPEND BLACKWORDS "truncate64" "ftruncate64" "lockf64")
      list(
        APPEND
        BLACKWORDS
        "getrlimit64"
        "setrlimit64"
        "prlimit64"
        "rlimit64"
        "rlim64_t")
    endif()
    if(NOT CONFIG_ARCH_SETJMP_H)
      list(APPEND BLACKWORDS "setjmp" "longjmp")
    endif()
    list(
      APPEND
      BLACKWORDS
      "in6addr_loopback"
      "BLKGETSIZE64"
      "BLKRAGET"
      "BLKRASET"
      "RNDGETENTCNT"
      "KEYCTL_JOIN_SESSION_KEYRING"
      "keyctl" # nuttx do not support keyctl syscall interface
      "ioctl_ns" # nuttx do not support ioctl_ns (ioctl for linux namespace)
      "si_uid" # nuttx "struct siginfo" do not have "si_uid" member
      "si_addr" # nuttx "struct siginfo" do not have "si_addr" member
      "TFD_TIMER_ABSTIME"
      "setdomainname"
      "chroot"
      "safe_chroot"
      "SAFE_BIND"
      "wait4"
      "ksmd"
      "PR_SET_PDEATHSIG"
      "PR_GET_PDEATHSIG"
      "ptrace" # nuttx do not support ptrace
      "uevents" # nuttx do not support uevents
      "futex" # nuttx do not support futex for now
      "tst_module_exists" # nuttx do not support linux kernel module framework
      "tst_module_load"
      "tst_module_unload"
      "module"
      "setpgid"
      "safe_setpgid"
      "safe_getpgid"
      "open_kconfig"
      "close_kconfig"
      "firmware" # nuttx do not support linux firmware load framework
      "nftw64")

    list(
      APPEND
      BLACKSRCS
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/io/disktest/main.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/io/direct_io/dma_thread_diotest.c
    )

    # syscalls/nftw/test_func.c has provide the implementation of test_func*(),
    # and the implementation are completely same as syscalls/nftw/test_func64.c
    # provided implementation, we just need to keep one of them. and the
    # following test64.c/lib64.c/tools64.c are deleted from source list with
    # same reason as test_func64.c
    list(
      APPEND
      BLACKSRCS
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/test_func64.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/test64.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/lib64.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/tools64.c)

    add_compile_options(
      -DHAVE_FCHOWNAT
      -DMAXPATHLEN=256
      -DUNIT_TEST
      -DHAVE_MKDIRAT
      -DHAVE_OPENAT
      -DTEST_CLEANUP
      -DBLKGETSIZE64=BLKGETSIZE
      -D__linux__
      -DLINUX)

    add_compile_options(
      -DHAVE_ASPRINTF
      -DHAVE_LINUX_FS_H
      -DTST_USE_NEWER64_SYSCALL=0
      -DINVALID_TEST_CASES=0
      -DHAVE_STRUCT_RLIMIT64
      -DHAVE_READLINKAT
      -DHAVE_FSTATAT)

    if(CONFIG_TIMER_FD)
      add_compile_options(-DHAVE_TIMERFD_CREATE -DHAVE_SYS_TIMERFD_H)
    endif()
    add_compile_options(
      -DHAVE_TIMERFD_GETTIME -DHAVE_TIMERFD_SETTIME -DHAVE_SYNCFS
      -DHAVE_MKNODAT -DHAVE_RENAMEAT2 -DHAVE_RENAMEAT)

    # add the macro definition to handle the naming conflication betweeb ltp
    # kernel testcases and the vela external projects
    add_compile_options(
      -Dsafe_touch=ltp_safe_touch
      -Dsafe_socket=ltp_safe_socket
      -Dsafe_bind=ltp_safe_bind
      -Dsafe_listen=ltp_safe_listen
      -Dsafe_connect=ltp_safe_connect
      -Dsafe_getsockname=ltp_safe_getsockname
      -Dsafe_close=ltp_safe_close
      -Dsafe_open=ltp_safe_open
      -Dsafe_write=ltp_safe_write
      -Derror=ltp_error)

    # The following warnings are all reported by the LTP kernel test cases
    # themselves. To avoid affecting the CI build working procedure, we need to
    # ignore these warnings.
    add_compile_options(
      -Wno-undef
      -Wno-pointer-sign
      -Wno-parentheses
      -Wno-use-after-free
      -Wno-discarded-qualifiers
      -Wno-implicit-int
      -Wno-pointer-to-int-cast
      -Wno-switch-bool
      -Wno-restrict
      -Wno-stringop-overflow
      -Wno-array-bounds
      -Wno-shift-count-overflow
      -Wno-stringop-overread)

    file(GLOB_RECURSE LTP_LINUX_ORIGS ${TESTLINUX_DIR}/*.c)

    if(LTP_LINUX_ORIGS)
      foreach(word IN LISTS BLACKWORDS)
        foreach(file ${LTP_LINUX_ORIGS})
          file(STRINGS ${file} content)
          string(FIND "${content}" "${word}" pos)
          if(pos GREATER -1)
            list(APPEND BLACKLIST ${file})
          endif()
        endforeach()
      endforeach()
    endif()

    foreach(src ${BLACKSRCS})
      foreach(orig ${LTP_LINUX_ORIGS})
        string(REGEX MATCH "${src}$" matched ${orig})
        if(matched)
          list(APPEND BLACKLIST ${orig})
        endif()
      endforeach()
    endforeach()

    foreach(ITEM ${BLACKLIST})
      list(REMOVE_ITEM LTP_LINUX_ORIGS ${ITEM})
    endforeach()

    set(LTP_LINUX_MAINCSRCS "")
    if(LTP_LINUX_ORIGS)
      foreach(file ${LTP_LINUX_ORIGS})
        file(STRINGS ${file} content)
        string(FIND "${content}" "main(" pos)
        if(pos GREATER -1)
          list(APPEND LTP_LINUX_MAINCSRCS ${file})
        else()
          string(FIND "${content}" "struct tst_test" pos)
          if(pos GREATER -1)
            list(APPEND LTP_LINUX_MAINCSRCS ${file})
          endif()
        endif()
      endforeach()
    endif()

    set(LTP_KERNEL_CSRCS ${LTP_LINUX_ORIGS})
    foreach(ITEM ${LTP_LINUX_MAINCSRCS})
      list(REMOVE_ITEM LTP_KERNEL_CSRCS ${ITEM})
    endforeach()

    set(PROGNAME "")
    if(LTP_LINUX_MAINCSRCS)
      foreach(file ${LTP_LINUX_MAINCSRCS})
        string(REPLACE "/" ";" file_parts ${file})
        list(LENGTH file_parts num_parts)
        if(num_parts GREATER 2)
          list(GET file_parts -3 part1)
          list(GET file_parts -2 part2)
          list(GET file_parts -1 part3)
          string(REPLACE "-" "_" part1 ${part1})
          string(REPLACE "-" "_" part2 ${part2})
          string(REPLACE "-" "_" part3 ${part3})
          set(prog "ltp_${part1}_${part2}_${part3}")
          get_filename_component(progg ${prog} NAME_WE)
          list(APPEND PROGNAME ${progg})
        endif()
      endforeach()
    endif()

    # since we have enabled the UNIT_TEST, the random_range.c and parse_opts.c
    # self unittest will also be build as an executable, we name it as
    # random_range_test and parse_opts_test
    list(APPEND LTP_LINUX_MAINCSRCS
         ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/random_range.c)
    list(APPEND PROGNAME "random_range_test")
    list(APPEND LTP_LINUX_MAINCSRCS
         ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/parse_opts.c)
    list(APPEND PROGNAME "parse_opts_test")
    list(APPEND LTP_LINUX_MAINCSRCS
         ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/fs/doio/open_flags.c)
    list(APPEND PROGNAME "doio_openflags_test")

    # the following are the basic source
    list(
      APPEND
      LTP_KERNEL_CSRCS
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/get_path.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/safe_file_ops.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/safe_macros.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/parse_opts.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/self_exec.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_ansi_color.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_assert.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_device.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_res.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_sys_conf.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_test.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_tmpdir.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_wallclock.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_checkpoint.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_clocks.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_sig.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_parse_opts.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_pid.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_cmd.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fill_file.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_mkfs.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_kvercmp.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_kernel.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_timer_test.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_hugepage.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_buffers.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_resource.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fs_setup.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_safe_macros.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/safe_net.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/safe_pthread.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/safe_stdio.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_supported_fs_types.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_timer.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_get_bad_addr.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_status.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_sig_proc.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_process_state.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_virt.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_kconfig.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_path_has_mnt_flags.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_capability.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fs_type.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_cpu.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fill_fs.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fs_link_count.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_dir_is_empty.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_coredump.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_fs_has_free.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/fs/fs-bench/boxmuler.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/fs/doio/bytes_by_prefix.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/fs/doio/string_to_tokens.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/device-drivers/base/user_base/tbase_ki.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/tools.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/nftw/lib.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/sched/sched_stress/sched.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/io/direct_io/diotest_routines.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/controllers/libcontrollers/libcontrollers.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/mem/lib/mem.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/memfd_create/memfd_create_common.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/lib/tst_safe_sysv_ipc.c
      ${CMAKE_CURRENT_LIST_DIR}/ltp/testcases/kernel/syscalls/ipc/lib/libipc.c
      ${CMAKE_CURRENT_LIST_DIR}/src/nx_syscall.c
      ${CMAKE_CURRENT_LIST_DIR}/src/dummy.c)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${TESTLINUX_DIR}/include)
    include_directories(${TESTLINUX_DIR}/../../include)
    include_directories(${TESTLINUX_DIR}/syscalls/ptrace)
    include_directories(${TESTLINUX_DIR}/syscalls/utils)
    include_directories(${TESTLINUX_DIR}/../../include/old)
    include_directories(${TESTLINUX_DIR}/fs/doio/include)
    include_directories(${TESTLINUX_DIR}/controllers/libcontrollers)
    include_directories(${TESTLINUX_DIR}/mem/include)
    include_directories(${TESTLINUX_DIR}/syscalls/ipc/lib)
    include_directories(${TESTLINUX_DIR}/syscalls/ipc/libnewipc)

    list(LENGTH LTP_LINUX_MAINCSRCS SOURCE_FILES_COUNT)
    math(EXPR LAST_INDEX "${SOURCE_FILES_COUNT} - 1")

    foreach(INDEX RANGE 0 ${LAST_INDEX})
      list(GET LTP_LINUX_MAINCSRCS ${INDEX} SOURCE_FILES)
      list(GET PROGNAME ${INDEX} EXECUTABLE_NAMES)
      nuttx_add_application(
        NAME
        ${EXECUTABLE_NAMES}
        PRIORITY
        SCHED_PRIORITY_DEFAULT
        STACKSIZE
        ${CONFIG_TESTING_LTP_STACKSIZE}
        SRCS
        ${SOURCE_FILES})
    endforeach()
  endif()

  list(APPEND LTP_CSRCS ${LTP_KERNEL_CSRCS})
  nuttx_add_library(ltp)
  target_sources(ltp PRIVATE ${LTP_CSRCS})

endif()
