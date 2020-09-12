/****************************************************************************
 * apps/include/system/nxrecorder.h
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

#ifndef __APPS_INCLUDE_SYSTEM_NXRECORDER_H
#define __APPS_INCLUDE_SYSTEM_NXRECORDER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/* This structure describes the internal state of the NxRecorder */

struct nxrecorder_s
{
  int         state;                   /* Current recorder state */
  int         dev_fd;                  /* File descriptor of active device */
  mqd_t       mq;                      /* Message queue for the recordthread */
  char        mqname[16];              /* Name of our message queue */
  pthread_t   record_id;               /* Thread ID of the recordthread */
  int         crefs;                   /* Number of references to the recorder */
  sem_t       sem;                     /* Thread sync semaphore */
  int         fd;                      /* File descriptor of open file */
  char        device[CONFIG_NAME_MAX]; /* Preferred audio device */
#ifdef CONFIG_AUDIO_MULTI_SESSION
  FAR void    *session;                /* Session assignment from device */
#endif
};

typedef int (*nxrecorder_func)(FAR struct nxrecorder_s *precorder,
                               char *pargs);

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
 * Name: nxrecorder_create
 *
 *   Allocates and Initializes a NxRecorder context that is passed to all
 *   nxrecorder routines.  The recorder MUST be destroyed using the
 *   nxrecorder_destroy() routine since the context is reference counted.
 *   The context can be used in a mode where the caller creates the
 *   context, starts a file recording, and then forgets about the context
 *   and it will self free.  This is because the nxrecorder_recordfile
 *   will also create a reference to the context, so the client calling
 *   nxrecorder_destroy() won't actually de-allocate anything.  The freeing
 *   will occur after the record thread has completed.
 *
 *   Alternately, the caller can create the object and hold on to it, then
 *   the context will persist until the original creator destroys it.
 *
 * Input Parameters:    None
 *
 * Returned Value:
 *   Pointer to created NxRecorder context or NULL if error.
 *
 ****************************************************************************/

FAR struct nxrecorder_s *nxrecorder_create(void);

/****************************************************************************
 * Name: nxrecorder_release
 *
 *   Reduces the reference count to the recorder and if it reaches zero,
 *   frees all memory used by the context.
 *
 * Input Parameters:
 *   precorder    Pointer to the NxRecorder context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxrecorder_release(FAR struct nxrecorder_s *precorder);

/****************************************************************************
 * Name: nxrecorder_reference
 *
 *   Increments the reference count to the recorder.
 *
 * Input Parameters:
 *   precorder    Pointer to the NxRecorder context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxrecorder_reference(FAR struct nxrecorder_s *precorder);

/****************************************************************************
 * Name: nxrecorder_setdevice
 *
 *   Sets the preferred Audio device to use with the instance of the
 *   nxrecorder.  Without a preferred device set, the nxrecorder will search
 *   the audio subsystem to find a suitable device depending on the
 *   type of audio operation requested (i.e. an MP3 decoder device when
 *   recording an MP3 file, a WAV decoder device for a WAV file, etc.).
 *
 * Input Parameters:
 *   precorder - Pointer to the context to initialize
 *   device    - Pointer to pathname of the preferred device
 *
 * Returned Value:
 *   OK if context initialized successfully, error code otherwise.
 *
 ****************************************************************************/

int nxrecorder_setdevice(FAR struct nxrecorder_s *precorder,
                         FAR const char *device);

/****************************************************************************
 * Name: nxrecorder_recordraw
 *
 *   Plays the specified media file (from the filesystem) using the
 *   Audio system.  If a preferred device has been set, that device
 *   will be used for the recordback, otherwise the first suitable device
 *   found in the /dev/audio directory will be used.
 *
 * Input Parameters:
 *   precorder - Pointer to the context to initialize
 *   filename  - Pointer to pathname of the file to record
 *   nchannels - channels num
 *   bpsampe   - bit width
 *   samprate  - sample rate
 *   chmap      channel map
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

int nxrecorder_recordraw(FAR struct nxrecorder_s *precorder,
                         FAR const char *filename, uint8_t nchannels,
                         uint8_t bpsamp, uint32_t samprate, uint8_t chmap);

/****************************************************************************
 * Name: nxrecorder_stop
 *
 *   Stops current recordback.
 *
 * Input Parameters:
 *   precorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxrecorder_stop(FAR struct nxrecorder_s *precorder);
#endif

/****************************************************************************
 * Name: nxrecorder_pause
 *
 *   Pauses current recordback.
 *
 * Input Parameters:
 *   precorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_pause(FAR struct nxrecorder_s *precorder);
#endif

/****************************************************************************
 * Name: nxrecorder_resume
 *
 *   Resumes current recordback.
 *
 * Input Parameters:
 *   precorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_resume(FAR struct nxrecorder_s *precorder);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_NXRECORDER_H */
