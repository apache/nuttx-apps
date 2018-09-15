/****************************************************************************
 * apps/include/system/nxrecorder.h
 *
 *   Copyright (C) 2017 Pinecone Inc. All rights reserved.
 *   Author: Zhong An <zhongan@pinecone.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_SYSTEM_NXRECORDER_H
#define __APPS_INCLUDE_SYSTEM_NXRECORDER_H 1

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
  int         state;          /* Current recorder state */
  int         devFd;          /* File descriptor of active device */
  mqd_t       mq;             /* Message queue for the recordthread */
  char        mqname[16];     /* Name of our message queue */
  pthread_t   recordId;       /* Thread ID of the recordthread */
  int         crefs;          /* Number of references to the recorder */
  sem_t       sem;            /* Thread sync semaphore */
  int         fd;             /* File descriptor of open file */
  char        device[CONFIG_NAME_MAX]; /* Preferred audio device */
#ifdef CONFIG_AUDIO_MULTI_SESSION
  FAR void    *session;       /* Session assigment from device */
#endif
};

typedef int (*nxrecorder_func)(FAR struct nxrecorder_s *pRecorder, char *pargs);

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
 *   pRecorder    Pointer to the NxRecorder context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxrecorder_release(FAR struct nxrecorder_s *pRecorder);

/****************************************************************************
 * Name: nxrecorder_reference
 *
 *   Increments the reference count to the recorder.
 *
 * Input Parameters:
 *   pRecorder    Pointer to the NxRecorder context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxrecorder_reference(FAR struct nxrecorder_s *pRecorder);

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
 *   pRecorder - Pointer to the context to initialize
 *   device    - Pointer to pathname of the preferred device
 *
 * Returned Value:
 *   OK if context initialized successfully, error code otherwise.
 *
 ****************************************************************************/

int nxrecorder_setdevice(FAR struct nxrecorder_s *pRecorder,
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
 *   pRecorder - Pointer to the context to initialize
 *   filename  - Pointer to pathname of the file to record
 *   nchannels - channels num
 *   bpsampe   - bit width
 *   samprate  - sample rate
 *
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

int nxrecorder_recordraw(FAR struct nxrecorder_s *pRecorder,
                         FAR const char *filename, uint8_t nchannels,
                         uint8_t bpsamp, uint32_t samprate);

/****************************************************************************
 * Name: nxrecorder_stop
 *
 *   Stops current recordback.
 *
 * Input Parameters:
 *   pRecorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxrecorder_stop(FAR struct nxrecorder_s *pRecorder);
#endif

/****************************************************************************
 * Name: nxrecorder_pause
 *
 *   Pauses current recordback.
 *
 * Input Parameters:
 *   pRecorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_pause(FAR struct nxrecorder_s *pRecorder);
#endif

/****************************************************************************
 * Name: nxrecorder_resume
 *
 *   Resumes current recordback.
 *
 * Input Parameters:
 *   pRecorder   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and recordback started.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_resume(FAR struct nxrecorder_s *pRecorder);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_NXRECORDER_H */
