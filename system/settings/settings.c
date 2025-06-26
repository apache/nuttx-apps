/****************************************************************************
 * apps/system/settings/settings.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nuttx/crc32.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <nuttx/config.h>

#include "storage.h"
#include "system/settings.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_SETTINGS_CACHED_SAVES
#  define TIMER_SIGNAL SIGRTMIN
#endif

#ifndef CONFIG_SYSTEM_SETTINGS_CACHE_TIME_MS
#  define CONFIG_SYSTEM_SETTINGS_CACHE_TIME_MS 100
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int      sanity_check(FAR char *str);
static uint32_t hash_calc(void);
static int      get_setting(FAR char *key, FAR setting_t **setting);
static size_t   get_string(FAR setting_t *setting, FAR char *buffer,
                         size_t size);
static int      set_string(FAR setting_t *setting, FAR char *str);
static int      get_int(FAR setting_t *setting, FAR int *i);
static int      set_int(FAR setting_t *setting, int i);
static int      get_bool(FAR setting_t *setting, FAR int *i);
static int      set_bool(FAR setting_t *setting, int i);
static int      get_float(FAR setting_t *setting, FAR double *f);
static int      set_float(FAR setting_t *setting, FAR double f);
static int      get_ip(FAR setting_t *setting, FAR struct in_addr *ip);
static int      set_ip(FAR setting_t *setting, FAR struct in_addr *ip);
static int      load(void);
static void     save(void);
static void     signotify(void);
static void     dump_cache(union sigval ptr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct
{
  pthread_mutex_t   mtx;
  uint32_t          hash;
  bool              wrpend;
  bool              initialized;
  storage_t         store[CONFIG_SYSTEM_SETTINGS_MAX_STORAGES];
  struct notify_s   notify[CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS];
#if defined(CONFIG_SYSTEM_SETTINGS_CACHED_SAVES)
  struct sigevent   sev;
  struct itimerspec trigger;
  timer_t           timerid;
#endif
} g_settings;

setting_t map[CONFIG_SYSTEM_SETTINGS_MAP_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hash_calc
 *
 * Description:
 *    Gets a setting for a string value
 *
 * Input Parameters:
 *    none
 * Returned Value:
 *   crc32 hash of all the settings
 *
 ****************************************************************************/

static uint32_t hash_calc(void)
{
  return crc32((FAR uint8_t *)map, sizeof(map));
}

/****************************************************************************
 * Name: get_setting
 *
 * Description:
 *    Gets a setting for a given key
 *
 * Input Parameters:
 *    key        - key of the required setting
 *    setting    - pointer to pointer for the setting
 *
 * Returned Value:
 *   The value of the setting for the given key
 *
 ****************************************************************************/

static int get_setting(FAR char *key, FAR setting_t **setting)
{
  int i;
  int ret = OK;

  assert(*setting == NULL);

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAP_SIZE; i++)
    {
      if (map[i].type == SETTING_EMPTY)
        {
          ret = -ENOENT;
          goto exit;
        }

      if (strcmp(map[i].key, key) == 0)
        {
          *setting = &map[i];
          goto exit;
        }
    }

  *setting = NULL;
  ret = -ENOENT;

exit:
  return ret;
}

/****************************************************************************
 * Name: get_string
 *
 * Description:
 *    Gets a setting for a string value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    str            - pointer to return the string value
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static size_t get_string(FAR setting_t *setting, FAR char *buffer,
                         size_t size)
{
  assert(setting);
  if ((setting->type != SETTING_STRING) &&
      (setting->type != SETTING_IP_ADDR))
    {
      return -EACCES;
    }

  if (setting->type == SETTING_STRING)
    {
      FAR const char *s = setting->val.s;
      size_t len = strlen(s);

      assert(len < size);
      if (len >= size)
        {
          return 0;
        }

      strncpy(buffer, s, size);
      buffer[size - 1] = '\0';

      return len;
    }
  else if (setting->type == SETTING_IP_ADDR)
    {
      assert(INET_ADDRSTRLEN < size);

      inet_ntop(AF_INET, &setting->val.ip, buffer, size);
      buffer[size - 1] = '\0';

      return strlen(buffer);
    }

  return 0;
}

/****************************************************************************
 * Name: set_string
 *
 * Description:
 *    Creates a setting for a string value setting->type != set_
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    str            - the string value
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int set_string(FAR setting_t *setting, FAR char *str)
{
  assert(setting);

  if ((setting->type != SETTING_STRING) &&
      (setting->type != SETTING_IP_ADDR))
    {
      return -EACCES;
    }

  if (strlen(str) >= CONFIG_SYSTEM_SETTINGS_VALUE_SIZE)
    {
      return -EINVAL;
    }

  if (strlen(str) && (sanity_check(str) < 0))
    {
      return -EINVAL;
    }

  setting->type = SETTING_STRING;
  strncpy(setting->val.s, str, CONFIG_SYSTEM_SETTINGS_VALUE_SIZE);
  setting->val.s[CONFIG_SYSTEM_SETTINGS_VALUE_SIZE - 1] = '\0';

  return OK;
}

/****************************************************************************
 * Name: get_int
 *
 * Description:
 *    Gets a setting for an integer value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    i              - pointer to return the integer value
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int get_int(FAR setting_t *setting, FAR int *i)
{
  assert(setting);

  switch (setting->type)
    {
      case SETTING_INT:
        {
          *i = setting->val.i;
        }
        break;

      case SETTING_BOOL:
        {
          *i = !!setting->val.i;
        }
        break;

      case SETTING_FLOAT:
        {
          *i = (int)setting->val.f;
        }
        break;

      default:
        {
          return -EACCES;
        }
        break;
    }

  return OK;
}

/****************************************************************************
 * Name: set_int
 *
 * Description:
 *    Creates a setting for an integer value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    i              - the integer value
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int set_int(FAR setting_t *setting, int i)
{
  assert(setting);
  if ((setting->type != SETTING_INT)  &&
      (setting->type != SETTING_BOOL) &&
      (setting->type != SETTING_FLOAT))
    {
      return -EACCES;
    }

  setting->type = SETTING_INT;
  setting->val.i = i;

  return OK;
}

/****************************************************************************
 * Name: get_bool
 *
 * Description:
 *    Gets a setting for a boolean value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    f              - pointer to return the boolean value
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int get_bool(FAR setting_t *setting, FAR int *i)
{
  assert(setting);

  switch (setting->type)
    {
      case SETTING_INT:
      case SETTING_BOOL:
        {
          *i = !!setting->val.i;
        }
        break;

      default:
        {
          return -EACCES;
        }
        break;
    }

  return OK;
}

/****************************************************************************
 * Name: set_bool
 *
 * Description:
 *    Creates a setting for a boolean value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    i              - the boolean value
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int set_bool(FAR setting_t *setting, int i)
{
  assert(setting);
  if ((setting->type != SETTING_BOOL) && (setting->type != SETTING_INT))
    {
      return -EACCES;
    }

  setting->type = SETTING_BOOL;
  setting->val.i = !!i;

  return OK;
}

/****************************************************************************
 * Name: get_float
 *
 * Description:
 *    Gets a setting for a float value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    f              - pointer to return the floating point value
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int get_float(FAR setting_t *setting, FAR double *f)
{
  assert(setting);

  switch (setting->type)
    {
      case SETTING_FLOAT:
        {
          *f = setting->val.f;
        }
        break;

      case SETTING_INT:
        {
          *f = (double)setting->val.i;
        }
        break;

      default:
        {
          return -EACCES;
        }
        break;
    }

  return OK;
}

/****************************************************************************
 * Name: set_float
 *
 * Description:
 *    Creates a setting for a float value
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    f              - the floating point value
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int set_float(FAR setting_t *setting, double f)
{
  assert(setting);
  if ((setting->type != SETTING_FLOAT) &&
      (setting->type != SETTING_INT))
    {
      return -EACCES;
    }

  setting->type = SETTING_FLOAT;
  setting->val.f = f;

  return OK;
}

/****************************************************************************
 * Name: get_ip
 *
 * Description:
 *    Creates a setting for an IP address
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    ip             - pointer to return the IP address
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int get_ip(FAR setting_t *setting, FAR struct in_addr *ip)
{
  int ret;
  assert(setting);

  switch (setting->type)
    {
      case SETTING_IP_ADDR:
        {
          memcpy(ip, &setting->val.ip, sizeof(struct in_addr));
          ret = OK;
        }
        break;

      case SETTING_STRING:
        {
          ret = inet_pton(AF_INET, setting->val.s, ip);
        }
        break;

      default:
        {
          return -EACCES;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: set_ip
 *
 * Description:
 *    Creates a setting for an IP address
 *
 * Input Parameters:
 *    setting        - pointer to the setting type
 *    ip             - IP address (in network byte order)
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int set_ip(FAR setting_t *setting, FAR struct in_addr *ip)
{
  assert(setting);
  if ((setting->type != SETTING_IP_ADDR) &&
      (setting->type != SETTING_STRING))
    {
      return -EACCES;
    }

  setting->type = SETTING_IP_ADDR;
  memcpy(&setting->val.ip, ip, sizeof(struct in_addr));

  return OK;
}

/****************************************************************************
 * Name: load
 *
 * Description:
 *    Loads all values
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int load(void)
{
  int i;
  int ret;
  int loadfailed = 0;

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAX_STORAGES; i++)
    {
      if ((g_settings.store[i].file[0] != '\0') &&
           g_settings.store[i].load_fn)
        {
          ret = g_settings.store[i].load_fn(g_settings.store[i].file);
          if (ret < 0)
            {
              loadfailed++;
            }
        }
    }

  if (loadfailed >= CONFIG_SYSTEM_SETTINGS_MAX_STORAGES)
    {
      /* ALL storages failed to load. We have a problem. */

      return -ENODATA;
    }
  else
    {
      return OK;
    }
}

/****************************************************************************
 * Name: save
 *
 * Description:
 *    Saves cached values, either immediately or on a timer
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void save(void)
{
  g_settings.wrpend = true;

#ifdef CONFIG_SYSTEM_SETTINGS_CACHED_SAVES
  timer_settime(g_settings.timerid, 0, &g_settings.trigger, NULL);
#else
  union sigval value =
  {
    .sival_ptr = &g_settings.wrpend,
  };

  dump_cache(value);
#endif
}

/****************************************************************************
 * Name: signotify
 *
 * Description:
 *    Notify anything waiting for a signal
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *    none
 *
 ****************************************************************************/

static void signotify(void)
{
  int i;

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS; i++)
    {
      if (g_settings.notify[i].pid == 0)
        {
          break;
        }

      kill(g_settings.notify[i].pid, g_settings.notify[i].signo);
    }
}

/****************************************************************************
 * Name: dump_cache
 *
 * Description:
 *    Writes out the cached data to the appropriate storage
 *
 * Input Parameters:
 *    value            - parameter passed if called from a timer signal
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void dump_cache(union sigval ptr)
{
  int ret = OK;
  FAR bool *wrpend = (bool *)ptr.sival_ptr;

  int i;

  ret = pthread_mutex_lock(&g_settings.mtx);
  assert(ret >= 0);

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAX_STORAGES; i++)
    {
      if ((g_settings.store[i].file[0] != '\0') &&
           g_settings.store[i].save_fn)
        {
          ret = g_settings.store[i].save_fn(g_settings.store[i].file);
#if 0
          if (ret < 0)
            {
              /* What to do? We can't return anything from a void function.
               *
               * MIGHT BE A FUTURE REVISIT NEEDED
               */
            }
#endif
        }
    }

  *wrpend = false;

  pthread_mutex_unlock(&g_settings.mtx);
}

/****************************************************************************
 * Name: sanity_check
 *
 * Description:
 *    Checks that the string does not contain "illegal" characters
 *
 * Input Parameters:
 *    str              - the string to check
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

static int sanity_check(FAR char *str)
{
#ifdef CONFIG_DEBUG_ASSERTIONS
  assert(strchr(str, '=')  == NULL);
  assert(strchr(str, ';')  == NULL);
  assert(strchr(str, '\n') == NULL);
  assert(strchr(str, '\r') == NULL);
#else
  if ((strchr(str, '=')  != NULL) || (strchr(str, ';')  != NULL) ||
      (strchr(str, '\n') != NULL) || (strchr(str, '\r') != NULL))
    {
      return -EINVAL;
    }

#endif

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: settings_init
 *
 * Description:
 *   Initializes the settings storage.
 *
 * Input Parameters:  none
 *
 * Returned Value:    none
 *
 ****************************************************************************/

void settings_init(void)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
  pthread_mutex_init(&g_settings.mtx, &attr);

  memset(map, 0, sizeof(map));
  memset(g_settings.store, 0, sizeof(g_settings.store));
  memset(g_settings.notify, 0, sizeof(g_settings.notify));

#if defined(CONFIG_SYSTEM_SETTINGS_CACHED_SAVES)
  memset(&g_settings.sev, 0, sizeof(struct sigevent));
  g_settings.sev.sigev_notify          = SIGEV_THREAD;
  g_settings.sev.sigev_signo           = TIMER_SIGNAL;
  g_settings.sev.sigev_value.sival_ptr = &g_settings.wrpend;
  g_settings.sev.sigev_notify_function = dump_cache;

  memset(&g_settings.trigger, 0, sizeof(struct itimerspec));
  g_settings.trigger.it_value.tv_sec   =
                               CONFIG_SYSTEM_SETTINGS_CACHE_TIME_MS / 1000;
  g_settings.trigger.it_value.tv_nsec  =
                              (CONFIG_SYSTEM_SETTINGS_CACHE_TIME_MS % 1000) *
                               1000 * 1000;

  timer_create(CLOCK_REALTIME, &g_settings.sev, &g_settings.timerid);
#endif
  g_settings.initialized = true;
  g_settings.hash = 0;
  g_settings.wrpend = false;
}

/****************************************************************************
 * Name: settings_setstorage
 *
 * Description:
 *    Sets a file to be used as a settings storage.
 *    Except from the first file, if loading the file causes any changes
 *    to the settings, then the new map will be dumped to all files
 *    (effectively it syncs all storages).
 *
 * Input Parameters:
 *    file             - the filename of the storage to use
 *    type             - the type of the storage (BINARY or TEXT)
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

int settings_setstorage(FAR char *file, enum storage_type_e type)
{
  FAR storage_t *storage = NULL;
  int ret = OK;
  int idx = 0;
  uint32_t h;

  assert(g_settings.initialized);

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  while ((idx < CONFIG_SYSTEM_SETTINGS_MAX_STORAGES) &&
         (g_settings.store[idx].file[0] != '\0'))
    {
      idx++;
    }

  assert(idx < CONFIG_SYSTEM_SETTINGS_MAX_STORAGES);
  if (idx >= CONFIG_SYSTEM_SETTINGS_MAX_STORAGES)
    {
      ret = -ENOSPC;
      goto errout;
    }

  assert(strlen(file) < CONFIG_SYSTEM_SETTINGS_MAX_FILENAME);
  if (strlen(file) >= CONFIG_SYSTEM_SETTINGS_MAX_FILENAME)
    {
      ret = -EINVAL;
      goto errout;
    }

  storage = &g_settings.store[idx];
  strncpy(storage->file, file, sizeof(storage->file));
  storage->file[sizeof(storage->file) - 1] = '\0';

  switch (type)
  {
    case STORAGE_BINARY:
      {
        storage->load_fn = load_bin;
        storage->save_fn = save_bin;
      }
      break;

    case STORAGE_TEXT:
      {
        storage->load_fn = load_text;
        storage->save_fn = save_text;
      }
      break;

    default:
      {
        assert(0);
      }
      break;
  }

  ret = storage->load_fn(storage->file);

  h = hash_calc();

  /* Only save if there are more than 1 storages. */

  if ((storage != &g_settings.store[0]) && ((h != g_settings.hash) ||
      (access(file, F_OK) != 0)))
    {
      signotify();
      save();
    }

  g_settings.hash = h;

errout:
  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}

/****************************************************************************
 * Name: settings_sync
 *
 * Description:
 *    Synchronizes the storage.
 *
 *    wait_dump        - if cached saves are enabled, this determines whether
 *                       the function will wait until the save is actually
 *                       completed or just schedule a new save
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_sync(bool wait_dump)
{
  int ret = OK;
  uint32_t h;

  assert(g_settings.initialized);

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  ret = load();
  if (ret < 0) /* All storages failed to load */
    {
      goto done;
    }

  h = hash_calc();
  if (h != g_settings.hash)
    {
      g_settings.hash = h;
      signotify();
      save();
    }

done:
  pthread_mutex_unlock(&g_settings.mtx);

#ifdef CONFIG_SYSTEM_SETTINGS_CACHED_SAVES
  if ((wait_dump && (ret >= 0)))
    {
      while (g_settings.wrpend)
        {
          usleep(10 * 1000); /* Sleep for 10ms */
        }
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: settings_notify
 *
 * Description:
 *    Registers a task to be notified on any change of the settings.
 *    Whenever any value is changed, a signal will be sent to all
 *    registered threads. Signals are NOT sent when new settings are
 *    created or when the whole storage is cleared.
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_notify(void)
{
  int ret;
  int idx = 0;

  assert(g_settings.initialized);

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  while ((idx < CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS) &&
          g_settings.notify[idx].pid)
    {
      idx++;
    }

  assert(idx < CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS);
  if (idx >= CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS)
    {
      ret = -EINVAL;
      goto errout;
    }

  g_settings.notify[idx].pid = getpid();
  g_settings.notify[idx].signo = CONFIG_SYSTEM_SETTINGS_SIGNO;

errout:
  pthread_mutex_unlock(&g_settings.mtx);

  return OK;
}

/****************************************************************************
 * Name: settings_hash
 *
 * Description:
 *    Gets the hash of the settings storage.
 *
 *    This hash represents the internal state of the settings map. A
 *    unique number is calculated based on the contents of the whole map.
 *
 *    This hash can be used to check the settings for any alterations, i.e.
 *    any setting that may had its value changed since last check.
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_hash(FAR uint32_t *h)
{
  assert(g_settings.initialized);

  *h = g_settings.hash;

  return OK;
}

/****************************************************************************
 * Name: settings_clear
 *
 * Description:
 *    Clears all settings.
 *    Data in all storages are purged.
 *
 *    Note that if the settings are cleared during the application run-time
 *    (i.e. not during initialization), every access to the settings storage
 *    will fail.
 *
 *    All settings must be created again. This can be done either by calling
 *    Settings_create() again, or by restarting the application.
 *
 * Input Parameters:
 *    none
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_clear(void)
{
  int ret;

  assert(g_settings.initialized);

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  memset(map, 0, sizeof(map));
  g_settings.hash = 0;

  save();

  pthread_mutex_unlock(&g_settings.mtx);

  while (g_settings.wrpend)
    {
      usleep(10 * 1000); /* Sleep for 10ms */
    }

  return OK;
}

/****************************************************************************
 * Name: settings_create
 *
 * Description:
 *    Creates a new setting.
 *
 *    If the setting is found to exist in any of the storages, it will
 *    be loaded. Else, it will be created and the default value will be
 *    assigned.
 *
 * Input Parameters:
 *        key         - the key of the setting.
 *        type        - the type of the setting.
 *        ...         - the default value of the setting.
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_create(FAR char *key, enum settings_type_e type, ...)
{
  int ret = OK;
  FAR setting_t *setting = NULL;
  int j;

  assert(g_settings.initialized);

  if (type == SETTING_EMPTY)
    {
      return -EINVAL;
    }

  if (strlen(key) == 0 || strlen(key) >= CONFIG_SYSTEM_SETTINGS_KEY_SIZE)
    {
      return -EINVAL;
    }

  if (!isalpha(key[0]) || (sanity_check(key) < 0))
    {
      return -EINVAL;
    }

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  for (j = 0; j < CONFIG_SYSTEM_SETTINGS_MAP_SIZE; j++)
    {
      if (strcmp(key, map[j].key) == 0)
        {
          setting = &map[j];

          /* We found a setting with this key name */

          goto errout;
        }

      if (map[j].type == SETTING_EMPTY)
        {
          setting = &map[j];
          strncpy(setting->key, key, CONFIG_SYSTEM_SETTINGS_KEY_SIZE);
          setting->key[CONFIG_SYSTEM_SETTINGS_KEY_SIZE - 1] = '\0';

          /* This setting is empty/unused - we can use it */

         break;
        }
    }

  if (setting == NULL)
    {
      goto errout;
    }

  if ((setting->type == SETTING_EMPTY) ||
      (setting->type != type))
    {
      bool set_val = false;

      va_list ap;
      va_start(ap, type);

      switch (type)
      {
        case SETTING_STRING:
          {
            FAR char *str = va_arg(ap, FAR char *);

            if ((setting->type == SETTING_STRING) ||
                (setting->type == SETTING_IP_ADDR))
              {
                break;
              }

            set_val = true;
            setting->type = SETTING_STRING;
            ret = set_string(setting, str);
          }
          break;

        case SETTING_INT:
          {
            int i = va_arg(ap, int);

            if ((setting->type == SETTING_INT)  ||
                (setting->type == SETTING_BOOL) ||
                (setting->type == SETTING_FLOAT))
              {
                break;
              }

            set_val = true;
            setting->type = SETTING_INT;
            ret = set_int(setting, i);
          }
          break;

        case SETTING_BOOL:
          {
            int i = va_arg(ap, int);

            if ((setting->type == SETTING_BOOL) ||
                (setting->type == SETTING_INT))
              {
                break;
              }

            set_val = true;
            setting->type = SETTING_BOOL;
            ret = set_bool(setting, i);
          }
          break;

        case SETTING_FLOAT:
          {
            double f = va_arg(ap, double);

            if ((setting->type == SETTING_FLOAT) ||
                (setting->type == SETTING_INT))
              {
                break;
              }

            set_val = true;
            setting->type = SETTING_FLOAT;
            ret = set_float(setting, f);
          }
          break;

        case SETTING_IP_ADDR:
          {
            FAR struct in_addr *ip = va_arg(ap, FAR struct in_addr *);

            if ((setting->type == SETTING_IP_ADDR) ||
                (setting->type == SETTING_STRING))
              {
                break;
              }

            set_val = true;
            setting->type = SETTING_IP_ADDR;
            ret = set_ip(setting, ip);
          }
          break;

        default:
        case SETTING_EMPTY:
          {
            assert(0);
            ret = -EINVAL;
          }
          break;
      }

      va_end(ap);

      if ((ret < 0) || !set_val)
        {
          memset(setting, 0, sizeof(setting_t));
          setting = NULL;
        }
      else
        {
          g_settings.hash = hash_calc();
          save();
        }
    }

errout:
  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}

/****************************************************************************
 * Name: settings_type
 *
 * Description:
 *    Gets the type of a setting.
 *
 * Input Parameters:
 *    key         - the key of the setting.
 *    type        - pointer to int for the returned setting type
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_type(FAR char *key, FAR enum settings_type_e *stype)
{
  int ret;
  FAR setting_t *setting = NULL;

  assert(g_settings.initialized);
  assert(stype != NULL);
  assert(key != NULL);

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  ret = get_setting(key, &setting);

  if (setting)
    {
      *stype = setting->type;
    }

  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}

/****************************************************************************
 * Name: settings_get
 *
 * Description:
 *    Gets the value of a setting.
 *
 * Input Parameters:
 *    key         - the key of the setting.
 *    type        - the type of the setting
 *    ...         - pointer to store the setting value plus, if a string
 *                  setting, the length of the string to get
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_get(FAR char *key, enum settings_type_e type, ...)
{
  int ret;
  FAR setting_t *setting = NULL;

  assert(g_settings.initialized);
  assert(type != SETTING_EMPTY);
  assert(key[0] != '\0');

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  ret = get_setting(key, &setting);
  if (ret < 0)
    {
      goto errout;
    }

  va_list ap;
  va_start(ap, type);

  switch (type)
  {
    case SETTING_STRING:
      {
        FAR char *buf = va_arg(ap, FAR char *);
        size_t len = va_arg(ap, size_t);
        ret = (int)get_string(setting, buf, len);
      }
      break;

    case SETTING_INT:
      {
        FAR int *i = va_arg(ap, FAR int *);
        ret = get_int(setting, i);
      }
      break;

    case SETTING_BOOL:
      {
        FAR int *i = va_arg(ap, FAR int *);
        ret = get_bool(setting, i);
      }
      break;

    case SETTING_FLOAT:
      {
        FAR double *f = va_arg(ap, FAR double *);
        ret = get_float(setting, f);
      }
      break;

    case SETTING_IP_ADDR:
      {
        FAR struct in_addr *ip = va_arg(ap, FAR struct in_addr *);
        ret = get_ip(setting, ip);
      }
      break;

    default:
      {
        assert(0);
      }
      break;
  }

  va_end(ap);

errout:
  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}

/****************************************************************************
 * Name: settings_set
 *
 * Description:
 *    Sets the value of a setting. The type can be changed, within limits,
 *    here too.
 *
 * Input Parameters:
 *    key         - the key of the setting.
 *    type        - the type of the setting
 *    ...         - the new value of the setting.
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_set(FAR char *key, enum settings_type_e type, ...)
{
  int ret;
  FAR setting_t *setting = NULL;
  uint32_t h;

  assert(g_settings.initialized);
  assert(type != SETTING_EMPTY);
  assert(key[0] != '\0');

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  ret = get_setting(key, &setting);
  if (ret < 0)
    {
      goto errout;
    }

  va_list ap;
  va_start(ap, type);

  switch (type)
  {
    case SETTING_STRING:
      {
        FAR char *str = va_arg(ap, FAR char *);
        ret = set_string(setting, str);
      }
      break;

    case SETTING_INT:
      {
        int i = va_arg(ap, int);
        ret = set_int(setting, i);
      }
      break;

    case SETTING_BOOL:
      {
        int i = va_arg(ap, int);
        ret = set_bool(setting, i);
      }
      break;

    case SETTING_FLOAT:
      {
        double f = va_arg(ap, double);
        ret = set_float(setting, f);
      }
      break;

    case SETTING_IP_ADDR:
      {
        FAR struct in_addr *ip = va_arg(ap, FAR struct in_addr *);
        ret = set_ip(setting, ip);
      }
      break;

    default:
      {
        assert(0);
      }
      break;
  }

  va_end(ap);

  if (ret >= 0)
    {
      h = hash_calc();
      if (h != g_settings.hash)
        {
          g_settings.hash = h;

          signotify();
          save();
        }
    }

errout:
  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}

/****************************************************************************
 * Name: settings_iterate
 *
 * Description:
 *    Gets a copy of a setting at the specified position. It can be used to
 *    iterate over the settings map, by using successive values of idx.
 *
 * Input Parameters:
 *    idx         - the iteration index for the setting.
 *    setting     - pointer to return the setting value
 *
 * Returned Value:
 *    Success or negated failure code
 *
 ****************************************************************************/

int settings_iterate(int idx, FAR setting_t *setting)
{
  int ret;

  assert(g_settings.initialized);
  assert(setting);

  if ((idx < 0) || (idx >= CONFIG_SYSTEM_SETTINGS_MAP_SIZE))
    {
      memset(setting, 0, sizeof(setting_t));
      return -EINVAL;
    }

  ret = pthread_mutex_lock(&g_settings.mtx);
  if (ret < 0)
    {
      return ret;
    }

  memcpy(setting, &map[idx], sizeof(setting_t));

  if (map[idx].type == SETTING_EMPTY)
    {
      ret = -ENOENT;
    }

  pthread_mutex_unlock(&g_settings.mtx);

  return ret;
}
