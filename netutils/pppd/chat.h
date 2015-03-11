#ifndef __APPS_NETUTILS_PPPD_CHAT_H
#define __APPS_NETUTILS_PPPD_CHAT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <time.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct chat_line_s
{
  const char *request;
  const char *response;
};

struct chat_script_s
{
  time_t timeout;
  struct chat_line_s lines[];
};

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

int ppp_chat(int fd, struct chat_script_s *script, int echo);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_CHAT_H */