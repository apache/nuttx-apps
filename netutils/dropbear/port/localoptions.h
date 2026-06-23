/****************************************************************************
 * apps/netutils/dropbear/port/localoptions.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __APPS_NETUTILS_DROPBEAR_PORT_LOCALOPTIONS_H
#define __APPS_NETUTILS_DROPBEAR_PORT_LOCALOPTIONS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DROPBEAR_TRACKING_MALLOC 1

/* Persist the native Dropbear ECDSA host key at the configured path. */

#define ECDSA_PRIV_FILENAME CONFIG_NETUTILS_DROPBEAR_HOSTKEY_PATH

#define DROPBEAR_SVR_DROP_PRIVS 0

#ifdef CONFIG_SCHED_USER_IDENTITY
#  define DROPBEAR_SVR_MULTIUSER 1
#else
#  define DROPBEAR_SVR_MULTIUSER 0
#endif

#define DROPBEAR_SVR_PASSWORD_AUTH 1
#define DROPBEAR_SVR_PUBKEY_AUTH 0
#define DROPBEAR_SVR_PUBKEY_OPTIONS 0

#define DROPBEAR_REEXEC 0
#define DROPBEAR_SMALL_CODE 1
#define DROPBEAR_USER_ALGO_LIST 0

#define DROPBEAR_X11FWD 0
#define DROPBEAR_SVR_AGENTFWD 0
#define DROPBEAR_SVR_LOCALTCPFWD 0
#define DROPBEAR_SVR_REMOTETCPFWD 0
#define DROPBEAR_SVR_LOCALSTREAMFWD 0
#define DROPBEAR_SVR_REMOTESTREAMFWD 0

#define DROPBEAR_DSS 0
#define DROPBEAR_RSA 0
#define DROPBEAR_ECDSA 1
#define DROPBEAR_ED25519 0
#define DROPBEAR_SK_KEYS 0
#define DROPBEAR_ECC_256 1
#define DROPBEAR_ECC_384 0
#define DROPBEAR_ECC_521 0

#define DROPBEAR_AES128 1
#define DROPBEAR_AES256 0
#define DROPBEAR_CHACHA20POLY1305 1
#define DROPBEAR_ENABLE_CBC_MODE 0
#define DROPBEAR_ENABLE_GCM_MODE 0

#define DROPBEAR_SHA1_HMAC 0
#define DROPBEAR_SHA2_256_HMAC 1
#define DROPBEAR_SHA2_512_HMAC 0
#define DROPBEAR_SHA1_96_HMAC 0

#define DROPBEAR_CURVE25519 1
#define DROPBEAR_DH_GROUP14_SHA1 0
#define DROPBEAR_DH_GROUP14_SHA256 0
#define DROPBEAR_DH_GROUP16 0
#define DROPBEAR_DH_GROUP1 0
#define DROPBEAR_ECDH 0
#define DROPBEAR_SNTRUP761 0
#define DROPBEAR_MLKEM768 0

#define DROPBEAR_DEFAULT_CLI_AUTHKEY "/etc/dropbear/authorized_keys"
#define DROPBEAR_SFTPSERVER 0

#endif /* __APPS_NETUTILS_DROPBEAR_PORT_LOCALOPTIONS_H */
