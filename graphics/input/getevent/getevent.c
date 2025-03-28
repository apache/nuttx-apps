/****************************************************************************
 * apps/graphics/input/getevent/getevent.c
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>

#include <nuttx/input/keyboard.h>
#include <nuttx/input/mouse.h>
#include <nuttx/input/touchscreen.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETEVENT_INFO(format, ...) \
  printf("[getevent]: " format, ##__VA_ARGS__)

#define GETEVENT_ERR(format, ...) \
  fprintf(stderr, "[getevent]: " format, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct input_device_s;

typedef CODE int  (*input_init_cb)(FAR struct input_device_s *);
typedef CODE void (*input_read_cb)(FAR struct input_device_s *);

struct touch_priv_s
{
  uint8_t maxpoint;
  struct touch_sample_s sample[];
};

struct input_device_s
{
  int fd;
  input_init_cb init_cb;
  input_read_cb read_cb;
  FAR void *priv;
  FAR struct input_device_s *next;
  char path[];
};

struct input_devices_ctx_s
{
  FAR struct input_device_s *head;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_device
 ****************************************************************************/

static int open_device(FAR struct input_device_s *dev)
{
  GETEVENT_INFO("opening %s\n", dev->path);
  dev->fd = open(dev->path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

  if (dev->fd < 0)
    {
      GETEVENT_ERR("open %s failed, errno: %d\n", dev->path, errno);
      return -errno;
    }

  if (dev->init_cb)
    {
      return dev->init_cb(dev);
    }

  return 0;
}

/****************************************************************************
 * Name: touch_init
 ****************************************************************************/

static int touch_init(FAR struct input_device_s *dev)
{
  FAR struct touch_priv_s *priv;
  uint8_t maxpoint;
  int ret;

  ret = ioctl(dev->fd, TSIOC_GETMAXPOINTS, &maxpoint);
  if (ret < 0)
    {
      GETEVENT_ERR("ioctl GETMAXPOINTS failed: %d, "
                   "errno: %d\n", ret, errno);
      return -errno;
    }

  priv = calloc(1, sizeof(struct touch_priv_s) +
                SIZEOF_TOUCH_SAMPLE_S(maxpoint));
  if (!priv)
    {
      GETEVENT_ERR("Memory allocation failed for touch_priv_s\n");
      return -ENOMEM;
    }

  priv->maxpoint = maxpoint;
  dev->priv = priv;
  GETEVENT_INFO("ioctl TSIOC_GETMAXPOINTS: %d\n", maxpoint);
  return ret;
}

/****************************************************************************
 * Name: read_mouse_event
 ****************************************************************************/

static void read_mouse_event(FAR struct input_device_s *dev)
{
  struct mouse_report_s sample;
  ssize_t nbytes;

  /* Read one sample */

  nbytes = read(dev->fd, &sample, sizeof(struct mouse_report_s));

  /* Print the sample data on successful return */

  if (nbytes == sizeof(struct mouse_report_s))
    {
      GETEVENT_INFO("mouse event: %s\n", dev->path);
      GETEVENT_INFO("   buttons : %02" PRIx8 "\n", sample.buttons);
      GETEVENT_INFO("         x : %d\n", sample.x);
      GETEVENT_INFO("         y : %d\n", sample.y);
#ifdef CONFIG_INPUT_MOUSE_WHEEL
      GETEVENT_INFO("     wheel : %d\n", sample.wheel);
#endif
    }
}

/****************************************************************************
 * Name: read_touch_event
 ****************************************************************************/

static void read_touch_event(FAR struct input_device_s *dev)
{
  FAR struct touch_priv_s *priv = (FAR struct touch_priv_s *)dev->priv;
  ssize_t nbytes;
  size_t i;

  /* Read one sample */

  nbytes = read(dev->fd, priv->sample,
                SIZEOF_TOUCH_SAMPLE_S(priv->maxpoint));

  /* Print the sample data on successful return */

  if (nbytes == SIZEOF_TOUCH_SAMPLE_S(priv->maxpoint))
    {
      GETEVENT_INFO("touch event: %s\n", dev->path);
      GETEVENT_INFO("   npoints : %" PRId32 "\n", priv->sample->npoints);
      for (i = 0; i < priv->maxpoint; i++)
        {
          if (priv->sample->point[i].flags == 0)
            {
              break;
            }

          GETEVENT_INFO("Point      : %d\n", priv->sample->point[i].id);
          GETEVENT_INFO("     flags : %02x\n", priv->sample->point[i].flags);
          GETEVENT_INFO("         x : %d\n", priv->sample->point[i].x);
          GETEVENT_INFO("         y : %d\n", priv->sample->point[i].y);
#ifdef CONFIG_GRAPHICS_INPUT_GETEVENT_DETAIL_INFO
          GETEVENT_INFO("         h : %d\n", priv->sample->point[i].h);
          GETEVENT_INFO("         w : %d\n", priv->sample->point[i].w);
          GETEVENT_INFO("  pressure : %d\n",
                        priv->sample->point[i].pressure);
#endif
          GETEVENT_INFO(" timestamp : %" PRIu64"\n",
                        priv->sample->point[i].timestamp);
        }
    }
}

/****************************************************************************
 * Name: read_keyboard_event
 ****************************************************************************/

static void read_keyboard_event(FAR struct input_device_s *dev)
{
  struct keyboard_event_s sample;
  ssize_t nbytes;

  /* Read one sample */

  nbytes = read(dev->fd, &sample, sizeof(struct keyboard_event_s));

  /* Print the sample data on successful return */

  if (nbytes == sizeof(struct keyboard_event_s))
    {
      GETEVENT_INFO("keyboard event: %s\n", dev->path);
      GETEVENT_INFO("         type : %" PRIu32 "\n", sample.type);
      GETEVENT_INFO("         code : %" PRIu32 "\n", sample.code);
    }
}

/****************************************************************************
 * Name: cleanup_input_devices
 ****************************************************************************/

static void cleanup_input_devices(FAR struct input_devices_ctx_s *ctx)
{
  FAR struct input_device_s *dev = ctx->head;
  FAR struct input_device_s *next;

  while (dev)
    {
      next = dev->next;
      if (dev->fd >= 0)
        {
          close(dev->fd);
        }

      free(dev->priv);
      free(dev);
      dev = next;
    }

  ctx->head = NULL;
}

/****************************************************************************
 * Name: signal_handler
 ****************************************************************************/

static void
signal_handler(int signum, FAR siginfo_t *info, FAR void *context)
{
  FAR volatile sig_atomic_t *running = info->si_user;
  *running = 0;
}

/****************************************************************************
 * Name: run_event_loop
 ****************************************************************************/

static void run_event_loop(FAR struct input_devices_ctx_s *ctx)
{
  FAR struct input_device_s *dev;
  FAR struct pollfd *fds = NULL;
  volatile sig_atomic_t running = 1;
  struct sigaction sa;
  int fd_count = 0;
  int timeout = 500;
  int i = 0;
  int ret;

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_user = (FAR void *)&running;
  sigaction(SIGINT, &sa, NULL);

  for (dev = ctx->head; dev; dev = dev->next)
    {
      if (dev->fd >= 0)
        {
          fd_count++;
        }
    }

  fds = calloc(fd_count, sizeof(struct pollfd));

  if (!fds)
    {
      GETEVENT_ERR("Memory allocation failed for fds\n");
      return;
    }

  for (dev = ctx->head; dev; dev = dev->next)
    {
      if (dev->fd >= 0)
        {
          fds[i].fd = dev->fd;
          fds[i].events = POLLIN;
          fds[i].revents = 0;
          i++;
        }
    }

  while (running)
    {
      ret = poll(fds, fd_count, timeout);
      if (ret < 0 && errno != EINTR)
        {
          GETEVENT_ERR("poll error: %d\n", errno);
          break;
        }

      for (i = 0; i < fd_count; i++)
        {
          if (!(fds[i].revents & POLLIN))
            {
              continue;
            }

          for (dev = ctx->head; dev; dev = dev->next)
            {
              if (dev->fd == fds[i].fd)
                {
                  dev->read_cb(dev);
                  break;
                }
            }
        }
    }

  free(fds);
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(void)
{
  printf("Usage: getevent [OPTIONS]\n");
  printf("Options:\n");
  printf(" -h                Show this help message\n");
  printf(" -m /dev/mouse    -- mouse device path\n");
  printf(" -t /dev/input    -- touch device path\n");
  printf(" -k /dev/keyboard -- keyboard device path\n");
  printf("No options: Auto detect devices in /dev\n");
}

/****************************************************************************
 * Name: alloc_input_device
 ****************************************************************************/

static FAR struct input_device_s *
alloc_input_device(FAR struct input_devices_ctx_s *ctx,
                   FAR const char *path)
{
  FAR struct input_device_s *dev;
  FAR struct input_device_s *cur;
  size_t path_len = strlen(path) + 1;

  dev = calloc(1, sizeof(struct input_device_s) + path_len);
  if (!dev)
    {
      GETEVENT_ERR("Memory allocation failed for device\n");
      return NULL;
    }

  memcpy(dev->path, path, path_len);

  if (!ctx->head)
    {
      ctx->head = dev;
    }
  else
    {
      cur = ctx->head;
      while (cur->next)
        {
          cur = cur->next;
        }

      cur->next = dev;
    }

  return dev;
}

/****************************************************************************
 * Name: add_input_device
 ****************************************************************************/

static int add_input_device(FAR struct input_devices_ctx_s *ctx,
                            input_init_cb init_cb,
                            input_read_cb read_cb,
                            FAR const char *path)
{
  FAR struct input_device_s *dev = alloc_input_device(ctx, path);

  if (!dev)
    {
      return -ENOMEM;
    }

  dev->fd = -1;
  dev->init_cb = init_cb;
  dev->read_cb = read_cb;

  return open_device(dev);
}

/****************************************************************************
 * Name: detect_devices
 ****************************************************************************/

static int detect_devices(FAR struct input_devices_ctx_s *ctx)
{
  FAR struct dirent *entry;
  char path[PATH_MAX];
  FAR DIR *dir;
  int ret = 0;

  dir = opendir("/dev");
  if (!dir)
    {
      GETEVENT_ERR("Failed to open /dev directory, errno: %d\n", errno);
      return -errno;
    }

  while ((entry = readdir(dir)))
    {
      if (entry->d_type == DT_CHR)
        {
          if (snprintf(path, sizeof(path), "/dev/%s", entry->d_name) < 0)
            {
              GETEVENT_ERR("Memory allocation failed for path\n");
              continue;
            }

          if (strncmp(entry->d_name, "mouse", 5) == 0)
            {
              ret = add_input_device(ctx, NULL, read_mouse_event, path);
            }
          else if (strncmp(entry->d_name, "input", 5) == 0)
            {
              ret = add_input_device(ctx, touch_init, read_touch_event,
                                     path);
            }
          else if (strncmp(entry->d_name, "kbd", 3) == 0)
            {
              ret = add_input_device(ctx, NULL, read_keyboard_event, path);
            }

          if (ret < 0)
            {
              break;
            }
        }
    }

  closedir(dir);
  return ret;
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static int parse_commandline(FAR struct input_devices_ctx_s *ctx,
                             int argc, FAR char **argv)
{
  int ret = -EINVAL;
  int ch;

  if (argc == 1)
    {
      return detect_devices(ctx);
    }

  while ((ch = getopt(argc, argv, "hn:m:t:k:")) != -1)
    {
      switch (ch)
        {
        case 'm':
          ret = add_input_device(ctx, NULL, read_mouse_event, optarg);
          break;
        case 't':
          ret = add_input_device(ctx, touch_init, read_touch_event, optarg);
          break;
        case 'k':
          ret = add_input_device(ctx, NULL, read_keyboard_event, optarg);
          break;
        case '?':
        case 'h':
        default:
          show_usage();
          break;
        }

      if (ret < 0)
        {
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct input_devices_ctx_s ctx;
  int ret;

  memset(&ctx, 0, sizeof(ctx));

  /* Parse the command line arguments */

  ret = parse_commandline(&ctx, argc, argv);
  if (ret < 0)
    {
      goto cleanup;
    }

  /* Start event loop for input processing */

  run_event_loop(&ctx);

  /* Clean up resources */

cleanup:
  cleanup_input_devices(&ctx);

  GETEVENT_INFO("Terminating!\n");

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
