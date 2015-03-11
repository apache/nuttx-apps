#ifndef __APPS_NETUTILS_PPPD_PPP_ARCH_H
#define __APPS_NETUTILS_PPPD_PPP_ARCH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <net/if.h>

#include <apps/netutils/netlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TTYNAMSIZ 16

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ppp_context_s;

typedef uint8_t  u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

time_t ppp_arch_clock_seconds(void);

int ppp_arch_getchar(struct ppp_context_s *ctx, u8_t *p);
int ppp_arch_putchar(struct ppp_context_s *ctx, u8_t c);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_PPP_ARCH_H */