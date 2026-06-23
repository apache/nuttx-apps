/****************************************************************************
 * apps/netutils/dropbear/port/nuttx_config.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __APPS_NETUTILS_DROPBEAR_PORT_NUTTX_CONFIG_H
#define __APPS_NETUTILS_DROPBEAR_PORT_NUTTX_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DROPBEAR_SERVER 1
#define BUNDLED_LIBTOM 1
#define DISABLE_LASTLOG 1
#define DISABLE_PAM 1
#define DISABLE_PUTUTLINE 1
#define DISABLE_PUTUTXLINE 1
#define DISABLE_UTMP 1
#define DISABLE_UTMPX 1
#define DISABLE_WTMP 1
#define DISABLE_WTMPX 1

#ifndef CONFIG_NETUTILS_DROPBEAR_SYSLOG
#  define DISABLE_SYSLOG 1
#endif

#ifndef CONFIG_NETUTILS_DROPBEAR_COMPRESSION
#  define DISABLE_ZLIB 1
#endif

#define DROPBEAR_FUZZ 0
#define DROPBEAR_PLUGIN 0

#define HAVE_BASENAME 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CONST_GAI_STRERROR_PROTO 1
#define HAVE_CRYPT 1
#define HAVE_DECL_HTOLE64 1
#define HAVE_ENDIAN_H 1
#define HAVE_EXPLICIT_BZERO 1
#define HAVE_FREEADDRINFO 1
#define HAVE_GAI_STRERROR 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#define HAVE_GETRANDOM 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBGEN_H 1
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_PATHS_H 1
#define HAVE_PUTENV 1
#define HAVE_STATIC_ASSERT 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRLCAT 1
#define HAVE_STRLCPY 1
#define HAVE_STRUCT_ADDRINFO 1
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY 1
#define HAVE_SYS_RANDOM_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT8_T 1
#define HAVE_U_INT16_T 1
#define HAVE_U_INT32_T 1
#define HAVE_U_INT8_T 1
#define HAVE_UNDERSCORE_STATIC_ASSERT 1
#define HAVE_UNISTD_H 1
/* NuttX exposes writev(), but keep this port on Dropbear's simpler write()
 * path until the SSH-to-NSH channel bridge is validated with vectored
 * writes.
 */

#undef HAVE_WRITEV
#define STDC_HEADERS 1

#define PACKAGE_BUGREPORT ""
#define PACKAGE_NAME ""
#define PACKAGE_STRING ""
#define PACKAGE_TARNAME ""
#define PACKAGE_URL ""
#define PACKAGE_VERSION ""

#define SELECT_TYPE_ARG1 int
#define SELECT_TYPE_ARG234 (fd_set *)
#define SELECT_TYPE_ARG5 (struct timeval *)

#ifndef PF_UNIX
#  define PF_UNIX AF_UNIX
#endif

#ifndef GRND_NONBLOCK
#  define GRND_NONBLOCK O_NONBLOCK
#endif

#define IPPORT_RESERVED 1024

#define getuid dropbear_getuid
#define geteuid dropbear_geteuid
#define getpwuid dropbear_getpwuid
#define getpwnam dropbear_getpwnam
uid_t getuid(void);
uid_t geteuid(void);
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
int dropbear_auth_initialize(void);
int dropbear_verify_password(const char *username, const char *password);

#ifndef CONFIG_SCHED_USER_IDENTITY
/* NuttX has no supplementary-group support, so getgroups() does not exist. */

int dropbear_getgroups(int size, gid_t list[]);

#  define getgroups dropbear_getgroups
#endif

#endif /* __APPS_NETUTILS_DROPBEAR_PORT_NUTTX_CONFIG_H */
