/****************************************************************************
 * apps/include/system/nxlooper.h
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

#ifndef __APPS_INCLUDE_SYSTEM_NXLOOPER_H
#define __APPS_INCLUDE_SYSTEM_NXLOOPER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/* This structure describes the internal state of the NxLooper */

struct nxlooper_s
{
  int         loopstate;                   /* Current looper test state */
  int         recorddev_fd;                /* File descriptor of active
                                            * record device */
  char        recorddev[CONFIG_NAME_MAX];  /* Preferred record device */
  int         playdev_fd;                  /* File descriptor of active
                                            * play device */
  char        playdev[CONFIG_NAME_MAX];    /* Preferred loopback device */
  int         crefs;                       /* Number of references */
  sem_t       sem;                         /* Thread sync semaphore */
  char        mqname[16];                  /* Name of play message queue */
  mqd_t       mq;                          /* Message queue for the
                                            * loopthread */
  pthread_t   loop_id;                     /* Thread ID of the loopthread */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  FAR void    *pplayses;       /* Session assignment from device */
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
  FAR void    *precordses;     /* Session assignment from device */
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  uint16_t    volume;         /* Volume as a whole percentage (0-100) */
#endif
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

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nxlooper_create
 *
 *   Allocates and Initializes a NxLooper context that is passed to all
 *   nxlooper routines.  The looper MUST be destroyed using the
 *   nxlooper_destroy() routine since the context is reference counted.
 *   The context can be used in a mode where the caller creates the
 *   context, starts a looping, and then forgets about the context
 *   and it will self free.  This is because the nxlooper_loopraw
 *   will also create a reference to the context, so the client calling
 *   nxlooper_destroy() won't actually de-allocate anything. The freeing
 *   will occur after the loopthread has completed.
 *
 *   Alternately, the caller can create the object and hold on to it, then
 *   the context will persist until the original creator destroys it.
 *
 * Input Parameters:    None
 *
 * Returned Value:
 *   Pointer to created NxLooper context or NULL if error.
 ****************************************************************************/

FAR struct nxlooper_s *nxlooper_create(void);

/****************************************************************************
 * Name: nxlooper_release
 *
 *   Reduces the reference count to the looper and if it reaches zero,
 *   frees all memory used by the context.
 *
 * Input Parameters:
 *   plooper    Pointer to the NxLooper context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxlooper_release(FAR struct nxlooper_s *plooper);

/****************************************************************************
 * Name: nxlooper_reference
 *
 *   Increments the reference count to the looper.
 *
 * Input Parameters:
 *   plooper    Pointer to the NxLooper context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxlooper_reference(FAR struct nxlooper_s *plooper);

/****************************************************************************
 * Name: nxlooper_setdevice
 *
 *   Sets the preferred Audio device to use with the instance of the
 *   nxlooper.  Without a preferred device set, the nxlooper will search
 *   the audio subsystem to find a suitable device depending on the
 *   type of audio operation requested.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *   device    - Pointer to pathname of the preferred device
 *
 * Returned Value:
 *   OK if context initialized successfully, error code otherwise.
 *
 ****************************************************************************/

int nxlooper_setdevice(FAR struct nxlooper_s *plooper,
                       FAR const char *device);

/****************************************************************************
 * Name: nxlooper_loopraw
 *
 *   nxlooper_loopraw() tries to record and then play the raw data using the
 *   Audio system.  If a device is specified, it will try to use that
 *   device.
 *
 * Input Parameters:
 *   plooper    Pointer to the initialized Looper context
 *   nchannels  channel num
 *   bpsampe    bit width
 *   samprate   sample rate
 *   chmap      channel map
 *
 * Returned Value:
 *   OK         File is being looped
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxlooper_loopraw(FAR struct nxlooper_s *plooper,
                     uint8_t nchannels, uint8_t bpsamp,
                     uint32_t samprate, uint8_t chmap);

/****************************************************************************
 * Name: nxlooper_stop
 *
 *   Stops current loopback.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxlooper_stop(FAR struct nxlooper_s *plooper);
#endif

/****************************************************************************
 * Name: nxlooper_pause
 *
 *   Pauses current loopback.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxlooper_pause(FAR struct nxlooper_s *plooper);
#endif

/****************************************************************************
 * Name: nxlooper_resume
 *
 *   Resumes current loopback.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxlooper_resume(FAR struct nxlooper_s *plooper);
#endif

/****************************************************************************
 * Name: nxlooper_setvolume
 *
 *   Sets the loopback volume.  The volume is represented in 1/10th of a
 *   percent increments, so the range is 0-1000.  A value of 10 would mean
 *   1%.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *   volume    - Volume level to set in 1/10th percent increments
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
int nxlooper_setvolume(FAR struct nxlooper_s *plooper, uint16_t volume);
#endif

/****************************************************************************
 * Name: nxlooper_systemreset
 *
 *   Performs an audio system reset, including a hardware reset on all
 *   registered audio devices.
 *
 * Input Parameters:
 *   plooper   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET
int nxlooper_systemreset(FAR struct nxlooper_s *plooper);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_NXLOOPER_H */
