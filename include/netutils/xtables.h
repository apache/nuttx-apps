/****************************************************************************
 * apps/include/netutils/xtables.h
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

#ifndef __APPS_INCLUDE_NETUTILS_XTABLES_H
#define __APPS_INCLUDE_NETUTILS_XTABLES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <getopt.h>
#include <netinet/in.h>
#include <stdbool.h>

#include <nuttx/net/netfilter/netfilter.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define XTABLES_VERSION_CODE 12

/* Select the format the input has to conform to, as well as the target type
 * (area pointed to with XTOPT_POINTER). Note that the storing is not always
 * uniform. cb->val will be populated with as much as there is space, i.e.
 * exactly 2 items for ranges, but the target area can receive more values
 * (e.g. in case of ranges), or less values (e.g. XTTYPE_HOSTMASK).
 *
 * XTTYPE_NONE:        option takes no argument
 * XTTYPE_UINT*:       standard integer
 * XTTYPE_UINT*RC:     colon-separated range of standard integers
 * XTTYPE_DOUBLE:      double-precision floating point number
 * XTTYPE_STRING:      arbitrary string
 * XTTYPE_TOSMASK:     8-bit TOS value with optional mask
 * XTTYPE_MARKMASK32:  32-bit mark with optional mask
 * XTTYPE_SYSLOGLEVEL: syslog level by name or number
 * XTTYPE_HOST:        one host or address (ptr: union nf_inet_addr)
 * XTTYPE_HOSTMASK:    one host or address, with an optional prefix length
 *                      ptr: union nf_inet_addr; only host portion is stored
 * XTTYPE_PROTOCOL:    protocol number/name from /etc/protocols ptr: uint8_t
 * XTTYPE_PORT:        16-bit port name or number (supports XTOPT_NBO)
 * XTTYPE_PORTRC:      colon-separated port range (names acceptable),
 *                      (supports XTOPT_NBO)
 * XTTYPE_PLEN:        prefix length
 * XTTYPE_PLENMASK:    prefix length (ptr: union nf_inet_addr)
 * XTTYPE_ETHERMAC:    Ethernet MAC address in hex form
 */

enum xt_option_type
{
  XTTYPE_NONE,
  XTTYPE_UINT8,
  XTTYPE_UINT16,
  XTTYPE_UINT32,
  XTTYPE_UINT64,
  XTTYPE_UINT8RC,
  XTTYPE_UINT16RC,
  XTTYPE_UINT32RC,
  XTTYPE_UINT64RC,
  XTTYPE_DOUBLE,
  XTTYPE_STRING,
  XTTYPE_TOSMASK,
  XTTYPE_MARKMASK32,
  XTTYPE_SYSLOGLEVEL,
  XTTYPE_HOST,
  XTTYPE_HOSTMASK,
  XTTYPE_PROTOCOL,
  XTTYPE_PORT,
  XTTYPE_PORTRC,
  XTTYPE_PLEN,
  XTTYPE_PLENMASK,
  XTTYPE_ETHERMAC,
};

enum xtables_tryload
{
  XTF_DONT_LOAD,
  XTF_DURING_LOAD,
  XTF_TRY_LOAD,
  XTF_LOAD_MUST_SUCCEED,
};

enum xtables_exittype
{
  OTHER_PROBLEM = 1,
  PARAMETER_PROBLEM,
  VERSION_PROBLEM,
  RESOURCE_PROBLEM,
  XTF_ONLY_ONCE,
  XTF_NO_INVERT,
  XTF_BAD_VALUE,
  XTF_ONE_ACTION,
};

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct xtables_globals
{
  unsigned int option_offset;
  FAR const char *program_name;
  FAR const char *program_version;
  FAR struct option *orig_opts;
  FAR struct option *opts;
  noreturn_function void (*exit_err)(enum xtables_exittype status,
                                     FAR const char *msg, ...)
                          printf_like(2, 3);
  int (*compat_rev)(FAR const char *name, uint8_t rev, int opt);
};

/* name:       name of option
 * type:       type of input and validation method, see XTTYPE_*
 * id:         unique number (within extension) for option, 0-31
 * excl:       bitmask of flags that cannot be used with this option
 * also:       bitmask of flags that must be used with this option
 * flags:      bitmask of option flags, see XTOPT_*
 * ptroff:     offset into private structure for member
 * size:       size of the item pointed to by ptroff; this is a safeguard
 * min:        lowest allowed value (for singular integral types)
 * max:        highest allowed value (for singular integral types)
 */

struct xt_option_entry
{
  FAR const char *name;
  enum xt_option_type type;
  unsigned int id;
  unsigned int excl;
  unsigned int also;
  unsigned int flags;
  unsigned int ptroff;
  size_t size;
  unsigned int min;
  unsigned int max;
};

struct xt_xlate;
struct xt_xlate_tg_params
{
  FAR const void *ip;
  FAR const struct xt_entry_target *target;
  int numeric;
  bool escape_quotes;
};

/* ext_name:   name of extension currently being processed
 * data:       per-extension (kernel) data block
 * udata:      per-extension private scratch area
 *             (cf. xtables_{match,target}->udata_size)
 * xflags:     options of the extension that have been used
 */

struct xt_fcheck_call
{
  FAR const char *ext_name;
  FAR void *data;
  FAR void *udata;
  unsigned int xflags;
};

struct xt_xlate_mt_params
{
  FAR const void *ip;
  FAR const struct xt_entry_match *match;
  int numeric;
  bool escape_quotes;
};

/* arg:        input from command line
 * ext_name:   name of extension currently being processed
 * entry:      current option being processed
 * data:       per-extension kernel data block
 * xflags:     options of the extension that have been used
 * invert:     whether option was used with !
 * nvals:      number of results in uXX_multi
 * val:        parsed result
 * udata:      per-extension private scratch area
 *             (cf. xtables_{match,target}->udata_size)
 */

struct xt_option_call
{
  FAR const char *arg;
  FAR const char *ext_name;
  FAR const struct xt_option_entry *entry;
  FAR void *data;
  unsigned int xflags;
  bool invert;
  uint8_t nvals;
  union
  {
    uint8_t u8;
    uint8_t u8_range[2];
    uint8_t syslog_level;
    uint8_t protocol;
    uint16_t u16;
    uint16_t u16_range[2];
    uint16_t port;
    uint16_t port_range[2];
    uint32_t u32;
    uint32_t u32_range[2];
    uint64_t u64;
    uint64_t u64_range[2];
    double dbl;
    struct
    {
      union nf_inet_addr haddr;
      union nf_inet_addr hmask;
      uint8_t hlen;
    };
    struct
    {
      uint8_t tos_value;
      uint8_t tos_mask;
    };
    struct
    {
      uint32_t mark;
      uint32_t mask;
    };
    uint8_t ethermac[6];
  } val;

  /* Wished for a world where the ones below were gone: */

  union
  {
    FAR struct xt_entry_match **match;
    FAR struct xt_entry_target **target;
  };

  FAR void *xt_entry;
  FAR void *udata;
};

/* Include file for additions: new matches and targets. */

struct xtables_match
{
  /* ABI/API version this module requires. Must be first member,
   * as the rest of this struct may be subject to ABI changes.
   */

  FAR const char *version;
  FAR struct xtables_match *next;
  FAR const char *name;
  FAR const char *real_name;

  /* Revision of match (0 by default). */

  uint8_t revision;

  /* Extension flags */

  uint8_t ext_flags;
  uint16_t family;

  /* Size of match data. */

  size_t size;

  /* Size of match data relevant for userspace comparison purposes */

  size_t userspacesize;

  /* Function which prints out usage message. */

  void (*help)(void);

  /* Initialize the match. */

  void (*init)(FAR struct xt_entry_match *m);

  /* Function which parses command options; returns true if it
   * ate an option
   */

  /* entry is struct ipt_entry for example */

  int (*parse)(int c, FAR char **argv, int invert, FAR unsigned int *flags,
               FAR const void *entry, FAR struct xt_entry_match **match);

  /* Final check; exit if not ok. */

  void (*final_check)(unsigned int flags);

  /* Prints out the match iff non-NULL: put space at end */

  /* ip is struct ipt_ip * for example */

  void (*print)(FAR const void *ip,
                FAR const struct xt_entry_match *match, int numeric);

  /* Saves the match info in parsable form to stdout. */

  /* ip is struct ipt_ip * for example */

  void (*save)(FAR const void *ip, FAR const struct xt_entry_match *match);

  /* Print match name or alias */

  FAR const char *(*alias)(FAR const struct xt_entry_match *match);

  /* Pointer to list of extra command-line options */

  FAR const struct option *extra_opts;

  /* New parser */

  void (*x6_parse)(FAR struct xt_option_call *);
  void (*x6_fcheck)(FAR struct xt_fcheck_call *);
  FAR const struct xt_option_entry *x6_options;

  /* Translate iptables to nft */

  int (*xlate)(FAR struct xt_xlate *xl,
               FAR const struct xt_xlate_mt_params *params);

  /* Size of per-extension instance extra "global" scratch space */

  size_t udata_size;

  /* Ignore these men behind the curtain: */

  FAR void *udata;
  unsigned int option_offset;
  FAR struct xt_entry_match *m;
  unsigned int mflags;
  unsigned int loaded; /* simulate loading so options are merged properly */
};

struct xtables_target
{
  /* ABI/API version this module requires. Must be first member,
   * as the rest of this struct may be subject to ABI changes.
   */

  FAR const char *version;
  FAR struct xtables_target *next;
  FAR const char *name;

  /* Real target behind this, if any. */

  FAR const char *real_name;

  /* Revision of target (0 by default). */

  uint8_t revision;

  /* Extension flags */

  uint8_t ext_flags;
  uint16_t family;

  /* Size of target data. */

  size_t size;

  /* Size of target data relevant for userspace comparison purposes */

  size_t userspacesize;

  /* Function which prints out usage message. */

  void (*help)(void);

  /* Initialize the target. */

  FAR void (*init)(struct xt_entry_target *t);

  /* Function which parses command options; returns true if it
   * ate an option
   */

  /* entry is struct ipt_entry for example */

  int (*parse)(int c, FAR char **argv, int invert, FAR unsigned int *flags,
               FAR const void *entry,
               FAR struct xt_entry_target **targetinfo);

  /* Final check; exit if not ok. */

  void (*final_check)(unsigned int flags);

  /* Prints out the target iff non-NULL: put space at end */

  void (*print)(FAR const void *ip,
                FAR const struct xt_entry_target *target, int numeric);

  /* Saves the targinfo in parsable form to stdout. */

  void (*save)(FAR const void *ip,
               FAR const struct xt_entry_target *target);

  /* Print target name or alias */

  FAR const char *(*alias)(FAR const struct xt_entry_target *target);

  /* Pointer to list of extra command-line options */

  FAR const struct option *extra_opts;

  /* New parser */

  void (*x6_parse)(FAR struct xt_option_call *);
  void (*x6_fcheck)(FAR struct xt_fcheck_call *);
  FAR const struct xt_option_entry *x6_options;

  /* Translate iptables to nft */

  int (*xlate)(FAR struct xt_xlate *xl,
               FAR const struct xt_xlate_tg_params *params);
  size_t udata_size;

  /* Ignore these men behind the curtain: */

  FAR void *udata;
  unsigned int option_offset;
  FAR struct xt_entry_target *t;
  unsigned int tflags;
  unsigned int used;
  unsigned int loaded; /* simulate loading so options are merged properly */
};

struct xtables_rule_match
{
  FAR struct xtables_rule_match *next;
  FAR struct xtables_match *match;

  /* Multiple matches of the same type: the ones before
   * the current one are completed from parsing point of view
   */

  bool completed;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

EXTERN FAR struct xtables_match *xtables_matches;
EXTERN FAR struct xtables_target *xtables_targets;
EXTERN FAR struct xtables_globals *xt_params;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int xtables_insmod(FAR const char *, FAR const char *, bool);
int xtables_init_all(FAR struct xtables_globals *xtp,
                     uint8_t nfproto);
uint16_t xtables_parse_protocol(FAR const char *s);
void xtables_option_tpcall(unsigned int, FAR char **, bool,
                           FAR struct xtables_target *, FAR void *);
void xtables_option_mpcall(unsigned int, FAR char **, bool,
                           FAR struct xtables_match *, FAR void *);
void xtables_option_tfcall(FAR struct xtables_target *);
void xtables_option_mfcall(FAR struct xtables_match *);
FAR struct option *xtables_options_xfrm(FAR struct option *,
                                        FAR struct option *,
                                        FAR const struct xt_option_entry *,
                                        FAR unsigned int *);
FAR struct option *xtables_merge_options(FAR struct option *origopts,
FAR struct option *oldopts, FAR const struct option *newopts,
FAR unsigned int *option_offset);
FAR struct xtables_match *xtables_find_match(FAR const char *name,
           enum xtables_tryload, FAR struct xtables_rule_match **match);
FAR struct xtables_target *xtables_find_target(FAR const char *name,
                                               enum xtables_tryload);
int xtables_compatible_revision(FAR const char *name,
                                uint8_t revision, int opt);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_XTABLES_H */
