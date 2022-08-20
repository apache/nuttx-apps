/****************************************************************************
 * apps/system/uorb/uORB/uORB.h
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

#ifndef __APP_SYSTEM_UORB_UORB_UORB_H
#define __APP_SYSTEM_UORB_UORB_UORB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/sensor.h>

#include <sys/time.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct orb_metadata;
typedef void (*orb_print_message)(FAR const struct orb_metadata *meta,
                                  FAR const void *buffer);

struct orb_metadata
{
  FAR const char   *o_name;     /* Unique object name */
  uint16_t          o_size;     /* Object size */
#ifdef CONFIG_DEBUG_UORB
  orb_print_message o_cb;       /* Function pointer of output topic message */
#endif
};

typedef FAR const struct orb_metadata *orb_id_t;

struct orb_state
{
  uint32_t max_frequency;       /* Object maximum frequency, Hz */
  uint32_t min_batch_interval;  /* Object minimum batch interval, us */
  uint32_t queue_size;          /* The maximum number of buffered elements,
                                 * if 1, no queuing is is used
                                 */
  uint32_t nsubscribers;        /* Number of subscribers */
  uint64_t generation;          /* Mainline generation */
};

struct orb_object
{
  orb_id_t meta;                /* The metadata of topic object */
  int      instance;            /* The instance of topic object */
};

typedef uint64_t orb_abstime;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ORB_SENSOR_PATH        "/dev/uorb/"
#define ORB_USENSOR_PATH       "/dev/usensor"
#define ORB_PATH_MAX           (NAME_MAX + 16)

#ifdef CONFIG_UORB_ALERT
#  define uorbpanic(fmt, ...)  _alert(fmt "\n", ##__VA_ARGS__)
#else
#  define uorbpanic            _none
#endif

#ifdef CONFIG_UORB_ERROR
#  define uorberr(fmt, ...)    _err(fmt "\n", ##__VA_ARGS__)
#else
#  define uorberr              _none
#endif

#ifdef CONFIG_UORB_WARN
#  define uorbwarn(fmt, ...)   _warn(fmt "\n", ##__VA_ARGS__)
#else
#  define uorbwarn             _none
#endif

#ifdef CONFIG_UORB_INFO
#  define uorbinfo(fmt, ...)   _info(fmt "\n", ##__VA_ARGS__)
#else
#  define uorbinfo             _none
#endif

#ifdef CONFIG_DEBUG_UORB
#  define uorbdebug(fmt, ...)  syslog(LOG_INFO, fmt "\n", ##__VA_ARGS__)
#else
#  define uorbdebug            _none
#endif

#define uorbinfo_raw(fmt, ...) syslog(LOG_INFO, fmt "\n", ##__VA_ARGS__)

/**
 * Generates a pointer to the uORB metadata structure for
 * a given topic.
 *
 * The topic must have been declared previously in scope
 * with ORB_DECLARE().
 *
 * @param name    The name of the topic.
 */
#define ORB_ID(name)  &g_orb_##name

/**
 * Declare the uORB metadata for a topic (used by code generators).
 *
 * @param name      The name of the topic.
 */
#if defined(__cplusplus)
# define ORB_DECLARE(name) extern "C" const struct orb_metadata g_orb_##name
#else
# define ORB_DECLARE(name) extern const struct orb_metadata g_orb_##name
#endif

/**
 * Define (instantiate) the uORB metadata for a topic.
 *
 * The uORB metadata is used to help ensure that updates and
 * copies are accessing the right data.
 *
 * Note that there must be no more than one instance of this macro
 * for each topic.
 *
 * @param name    The name of the topic.
 * @param struct  The structure the topic provides.
 * @param cb      The function pointer of output topic message.
 */
#ifdef CONFIG_DEBUG_UORB
#define ORB_DEFINE(name, structure, cb) \
  const struct orb_metadata g_orb_##name = \
  { \
    #name, \
    sizeof(structure), \
    cb, \
  };
#else
#define ORB_DEFINE(name, structure, cb) \
  const struct orb_metadata g_orb_##name = \
  { \
    #name, \
    sizeof(structure), \
  };
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: orb_open
 *
 * Description:
 *   Open device exist node with name, instance and flags.
 *
 * Input Parameters:
 *   name         The topic name.
 *   instance     Instance number to open.
 *   flags        The open flags.
 *
 * Returned Value:
 *   fd on success, otherwise returns negative value and set errno.
 ****************************************************************************/

int orb_open(FAR const char *name, int instance, int flags);

/****************************************************************************
 * Name: orb_close
 *
 * Description:
 *   Close fd.
 *
 * Input Parameters:
 *   fd       A fd returned by orb_open.
 *
 * Returned Value:
 *   0 on success.
 ****************************************************************************/

int orb_close(int fd);

/****************************************************************************
 * Name: orb_advertise_multi_queue
 *
 * Description:
 *   This performs the initial advertisement of a topic; it creates the topic
 *   node in /dev/uorb and publishes the initial data.
 *
 * Input Parameters:
 *   meta         The uORB metadata (usually from the ORB_ID() macro)
 *   data         A pointer to the initial data to be published.
 *   instance     Pointer to an integer which yield the instance ID,
 *                (has default 0 if pointer is NULL).
 *   queue_size   Maximum number of buffered elements.
 *
 * Returned Value:
 *   -1 on error, otherwise returns an file descriptor
 *   that can be used to publish to the topic.
 *   If the topic in question is not known (due to an
 *   ORB_DEFINE with no corresponding ORB_DECLARE)
 *   this function will return -1 and set errno to ENOENT.
 ****************************************************************************/

int orb_advertise_multi_queue(FAR const struct orb_metadata *meta,
                              FAR const void *data,
                              FAR int *instance,
                              unsigned int queue_size);

static inline int orb_advertise(FAR const struct orb_metadata *meta,
                                FAR const void *data)
{
  int instance = 0;

  return orb_advertise_multi_queue(meta, data, &instance, 1);
}

static inline int orb_advertise_queue(FAR const struct orb_metadata *meta,
                                      FAR const void *data,
                                      unsigned int queue_size)
{
  int instance = 0;

  return orb_advertise_multi_queue(meta, data, &instance, queue_size);
}

static inline int orb_advertise_multi(FAR const struct orb_metadata *meta,
                                      FAR const void *data,
                                      FAR int *instance)
{
  return orb_advertise_multi_queue(meta, data, instance, 1);
}

/****************************************************************************
 * Name: orb_advertise_multi_queue_persist
 *
 * Description:
 *   orb_advertise_multi_queue_persist is similar to orb_advertise_mult and
 *   it can ensures that every subscriber has access to current and
 *   future data.
 *
 * Input Parameters:
 *   meta         The uORB metadata (usually from the ORB_ID() macro)
 *   data         A pointer to the initial data to be published.
 *   instance     Pointer to an integer which yield the instance ID,
 *                (has default 0 if pointer is NULL).
 *
 * Returned Value:
 *   -1 on error, otherwise returns an file descriptor
 *   that can be used to publish to the topic.
 *   If the topic in question is not known (due to an
 *   ORB_DEFINE with no corresponding ORB_DECLARE)
 *   this function will return -1 and set errno to ENOENT.
 ****************************************************************************/

int orb_advertise_multi_queue_persist(FAR const struct orb_metadata *meta,
                                      FAR const void *data,
                                      FAR int *instance,
                                      unsigned int queue_size);

/****************************************************************************
 * Name: orb_unadvertise
 *
 * Description:
 *   Unadvertise a topic.
 *
 * Input Parameters:
 *   fd       A fd returned by orb_advertise or orb_advertise_multi.
 *
 * Returned Value:
 *   0 on success.
 ****************************************************************************/

static inline int orb_unadvertise(int fd)
{
  return orb_close(fd);
}

/****************************************************************************
 * Name: orb_publish_multi
 *
 * Description:
 *   Publish the specified length of new data to a topic.
 *
 *   The data is published to the topic and any waiting subscribers will be
 *   notified. Subscribers that are not waiting can check the topic for
 *   updates using orb_check.
 *
 * Input Parameters:
 *   fd       The fd returned from orb_advertise.
 *   data     A pointer to the data to be published.
 *   len      The length of the data to be published.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with errno set accordingly.
 ****************************************************************************/

ssize_t orb_publish_multi(int fd, FAR const void *data, size_t len);

static inline int orb_publish(FAR const struct orb_metadata *meta,
                              int fd, FAR const void *data)
{
  int ret;

  ret = orb_publish_multi(fd, data, meta->o_size);
  return ret == meta->o_size ? 0 : -1;
}

static inline int orb_publish_auto(FAR const struct orb_metadata *meta,
                                   FAR int *fd, FAR const void *data,
                                   FAR int *instance)
{
  if (fd && *fd)
    {
      return orb_publish(meta, *fd, data);
    }
  else
    {
      int tmp;

      tmp = orb_advertise_multi_queue_persist(meta, data, instance, 1);
      if (tmp < 0)
        {
          return tmp;
        }

      if (fd)
        {
          *fd = tmp;
          return tmp;
        }
      else
        {
          return orb_unadvertise(tmp);
        }
    }
}

/****************************************************************************
 * Name: orb_subscribe_multi
 *
 * Description:
 *   Subscribe to a topic.
 *
 *   The data is published to the topic and any waiting subscribers will be
 *   notified. Subscribers that are not waiting can check the topic for
 *   updates using orb_check.
 *
 *   If there were any publications of the topic prior to the subscription,
 *   an orb_check right after orb_subscribe_multi will return true.
 *
 *   Subscription will succeed even if the topic has not been advertised;
 *   in this case, the topic will have a timestamp of zero, it will never
 *   signal a poll() event, checking will always return false and it cannot
 *   be copied, until the topic is subsequently advertised.
 *
 * Input Parameters:
 *   meta       The uORB metadata (usually from the ORB_ID() macro)
 *   instance   The instance of the topic. Instance 0 matches the topic of
 *              the orb_subscribe() call.
 *
 * Returned Value:
 *   -1 on error, otherwise returns a fd
 *   that can be used to read and update the topic.
 *   If the topic in question is not known (due to an
 *   ORB_DEFINE_OPTIONAL with no corresponding ORB_DECLARE)
 *   this function will return -1 and set errno to ENOENT.
 ****************************************************************************/

int orb_subscribe_multi(FAR const struct orb_metadata *meta,
                        unsigned instance);

static inline int orb_subscribe(FAR const struct orb_metadata *meta)
{
  return orb_subscribe_multi(meta, 0);
}

/****************************************************************************
 * Name: orb_unsubscribe
 *
 * Description:
 *   Unsubscribe from a topic.
 *
 * Input Parameters:
 *   fd       A fd returned from orb_subscribe.
 *
 * Returned Value:
 *   0 on success.
 ****************************************************************************/

static inline int orb_unsubscribe(int fd)
{
  return orb_close(fd);
}

/****************************************************************************
 * Name: orb_copy_multi
 *
 * Description:
 *   Fetch the specified length of data from a topic.
 *
 *   This is the only operation that will reset the internal marker that
 *   indicates that a topic has been updated for a subscriber. Once poll
 *   or check return indicating that an updaet is available, this call
 *   must be used to update the subscription.
 *
 * Input Parameters:
 *   fd       A fd returned from orb_subscribe.
 *   buffer   Pointer to the buffer receiving the data, or NULL if the
 *            caller wants to clear the updated flag without.
 *   len      The length to the buffer receiving the data.
 *
 * Returned Value:
 *   The positive non-zero number of bytes read on success.
 *   0 on if an end-of-file condition,
 *   -1 otherwise with errno set accordingly.
 ****************************************************************************/

ssize_t orb_copy_multi(int fd, FAR void *buffer, size_t len);

static inline int orb_copy(FAR const struct orb_metadata *meta,
                           int fd, FAR void *buffer)
{
  int ret;

  ret = orb_copy_multi(fd, buffer, meta->o_size);
  return ret == meta->o_size ? 0 : -1;
}

/****************************************************************************
 * Name: orb_get_state
 *
 * Description:
 *   Get some state about all subscriber of topic.
 *
 *   This state contains the maximum frequency and minimum batch interval
 *   in all subscriber, and it also contanis enable to indicate whether
 *   the current node is subscribed or activated.
 *
 *   If no one subscribes this topic, the state is set:
 *     max_frequency to 0. min_batch_interval to 0, enable to false.
 *
 * Input Parameters:
 *   fd       The fd returned from orb_advertise / orb_subscribe.
 *   state    Pointer to an state of struct orb_state type. This is an
 *            output parameter and will be set to the current state of topic.
 *
 * Returned Value:
 *   -1 on error.
 ****************************************************************************/

int orb_get_state(int fd, FAR struct orb_state *state);

/****************************************************************************
 * Name: orb_check
 *
 * Description:
 *   Check whether a topic has been published to since the last orb_copy.
 *
 *   This check can be used to determine whether to copy the topic when
 *   not using poll(), or to avoid the overhead of calling poll() when the
 *   topic is likely to have updated.
 *
 *   Updates are tracked on a per-fd basis; this call will continue to
 *   return true until orb_copy is called using the same fd.
 *
 * Input Parameters:
 *   fd       A fd returned from orb_subscribe.
 *   update   Set to true if the topic has been updated since the
 *            last time it was copied using this fd.
 *
 * Returned Value:
 *   0 if the check was successful,
 *   -1 otherwise with errno set accordingly.
 ****************************************************************************/

int orb_check(int fd, FAR bool *updated);

/****************************************************************************
 * Name: orb_ioctl
 *
 * Description:
 *   Ioctl control for the subscriber, the same as ioctl().
 *
 * Input Parameters:
 *   fd       A fd returned from orb_advertise / orb_subscribe.
 *   cmd      Ioctl command.
 *   arg      Ioctl argument.
 *
 * Returned Value:
 *   0 on success.
 ****************************************************************************/

int orb_ioctl(int fd, int cmd, unsigned long arg);

/****************************************************************************
 * Name: orb_set_batch_interval
 *
 * Description:
 *   The batch interval set through api is just the value user wants,
 *   and the final value depends on the hardware FIFO capability.
 *   This API will send POLLPRI event to notify publisher and
 *   publisher determine the final batch interval.
 *
 *   This API is only for topics with hardware fifo, such as sensor with
 *   hardware fifo, otherwise it's meaningless.
 *
 * Input Parameters:
 *   fd             A fd returned from orb_subscribe.
 *   batch_interval An batch interval in us.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

int orb_set_batch_interval(int fd, unsigned batch_interval);

/****************************************************************************
 * Name: orb_get_batch_interval
 *
 * Description:
 *   Get the batch interval in batch mode.
 *
 *   This API is only for topics with hardware fifo, such as sensor with
 *   hardware fifo, otherwise it's meaningless.
 *
 *   @see orb_set_batch_interval()
 *
 * Input Parameters:
 *   fd              A fd returned from orb_subscribe.
 *   batch_interval  The returned batch interval in us.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

int orb_get_batch_interval(int fd, FAR unsigned *batch_interval);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Set the minimum interval between which updates seen for a subscription.
 *
 * Input Parameters:
 *   fd         A fd returned from orb_subscribe.
 *   interval   An interval period in us.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

int orb_set_interval(int fd, unsigned interval);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Get the minimum interval between which updates seen for a subscription.
 *
 * Input Parameters:
 *   fd         A fd returned from orb_subscribe.
 *   interval   The returned interval period in us.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

int orb_get_interval(int fd, FAR unsigned *interval);

/****************************************************************************
 * Name:
 *   orb_set_frequency
 *
 * Description:
 *   Set the maximum frequency for a subscription.
 *
 * Input Parameters:
 *   fd         A fd returned from orb_subscribe.
 *   frequency  A frequency in hz.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

static inline int orb_set_frequency(int fd, unsigned frequency)
{
  return orb_set_interval(fd, frequency ? 1000000 / frequency : 0);
}

/****************************************************************************
 * Name:
 *   orb_get_frequency
 *
 * Description:
 *   Get the maximum frequency for a subscription.
 *
 * Input Parameters:
 *   fd         A fd returned from orb_subscribe.
 *   frequency  The returned frequency in hz.
 *
 * Returned Value:
 *   0 on success, -1 otherwise with ERRNO set accordingly.
 ****************************************************************************/

static inline int orb_get_frequency(int fd, FAR unsigned *frequency)
{
  unsigned interval;
  int ret;

  ret = orb_get_interval(fd, &interval);
  if (ret < 0)
    {
      return ret;
    }

  *frequency = interval ? 1000000 / interval : 0;

  return 0;
}

/****************************************************************************
 * Name: orb_elapsed_time
 *
 * Description:
 *   Get current value of system time in us.
 *
 * Returned Value:
 *   Absolute time.
 ****************************************************************************/

orb_abstime orb_absolute_time(void);

/****************************************************************************
 * Name: orb_elapsed_time
 *
 * Description:
 *   Compute the delta between a timestamp taken in the past and now.
 *
 *   This function is not interrupt save.
 *
 * Input Parameters:
 *   then   Past system time.
 *
 * Returned Value:
 *   Bewteen time.
 ****************************************************************************/

static inline orb_abstime orb_elapsed_time(FAR const orb_abstime *then)
{
  return orb_absolute_time() - *then;
}

/****************************************************************************
 * Name: orb_exists
 *
 * Description:
 *   Check if a topic instance has already been advertised.
 *
 * Input Parameters:
 *   meta       ORB topic metadata.
 *   instance   ORB instance
 *
 * Returned Value:
 *   0 if the topic exists, -1 otherwise.
 ****************************************************************************/

int orb_exists(FAR const struct orb_metadata *meta, int instance);

/****************************************************************************
 * Name: orb_group_count
 *
 * Description:
 *   Get instance amount of advertised topic instances.
 *
 * Input Parameters:
 *   meta       ORB topic metadata.
 *
 * Returned Value:
 *   0 if none
 ****************************************************************************/

int orb_group_count(FAR const struct orb_metadata *meta);

/****************************************************************************
 * Name: orb_get_meta
 *
 * Description:
 *   Get the metadata of topic object by name string.
 *
 * Input Parameters:
 *   name       The name of topic, ex: sensor_accel, sensor_accel0.
 *
 * Returned Value:
 *   The metadata on success. NULL on failure.
 ****************************************************************************/

FAR const struct orb_metadata *orb_get_meta(FAR const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __APP_SYSTEM_UORB_UORB_UORB_H */
