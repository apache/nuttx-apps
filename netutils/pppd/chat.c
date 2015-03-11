#include "ppp_conf.h"
#include "ppp_arch.h"
#include "chat.h"

#include <poll.h>

#define CHAT_MAX_SKIP     8
#define CHAT_ECHO_TIMEOUT 500

static int chat_read_byte(int fd, char* c, int timeout)
{
  int ret;
  struct pollfd fds;

  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;

  ret = poll(&fds, 1, timeout);
  if (ret <= 0)
    {
      return -1;
    }

  ret = read(fd, c, 1);
  if (ret != 1)
    {
      return -1;
    }

  printf("chat: char = %c (0x%02X)\n", *c, *c);

  return 0;
}

static void chat_flush(int fd)
{
  char tmp;
  while (chat_read_byte(fd, &tmp, 0) == 0);
}

static int chat_check_response(int fd, const char* response, int timeout)
{
  char c;
  int ret;
  int skip = CHAT_MAX_SKIP;

  while (*response)
    {
      ret = chat_read_byte(fd, &c, timeout);
      if (ret < 0)
        {
          return ret;
        }

      if (skip > 0 && (c == '\r' || c == '\n')) 
        {
          --skip;
          continue;
        }

      if (c == *response)
        {
          ++response;
        }
      else
        {
          return -1;
        }
    }

  return 0;
}

int ppp_chat(int fd, struct chat_script_s *script, int echo)
{
  int ret;
  size_t len;
  struct chat_line_s *line = script->lines;
  const char* request = line->request;
  const char* response = line->response;

  while (request)
    {
      chat_flush(fd);

      printf("chat: send '%s`\n", request);
      len = strlen(request);
      ret = write(fd, request, len);
      if (ret < 0)
        {
          return ret;
        }
      else if ((size_t)ret != len)
        {
          return -1;
        }

      ret = write(fd, "\r\n", 2);
      if (ret != 2)
        {
          return -1;
        }

      /* Check echo if enabled */

      if (echo)
        {
          ret = chat_check_response(fd, request, CHAT_ECHO_TIMEOUT);
          if (ret < 0)
            {
              printf("chat: invalid echo\n");
              return ret;
            }
        }

      if (response)
        {
          printf("chat: wait for '%s`\n", response);
          ret = chat_check_response(fd, response, script->timeout * 1000);
          if (ret < 0)
            {
              printf("chat: bad response\n");
              return ret;
            }

          printf("chat: got it!\n");
        }

      ++line;
      request = line->request;
      response = line->response;
    }

  return 0;
}