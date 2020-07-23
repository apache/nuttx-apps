/****************************************************************************
 * camera/camera_main.c
 *
 *   Copyright 2018, 2020 Sony Semiconductor Solutions Corporation
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
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>

#include <nuttx/video/video.h>

#include "camera_fileutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMAGE_JPG_SIZE     (512*1024)  /* 512kB for FullHD Jpeg file. */
#define IMAGE_YUV_SIZE     (320*240*2) /* QVGA YUV422 */

#define VIDEO_BUFNUM       (3)
#define STILL_BUFNUM       (1)

#define MAX_CAPTURE_NUM     (100)
#define DEFAULT_CAPTURE_NUM (10)

#define START_CAPTURE_TIME  (5000)   /* mili-seconds */
#define KEEP_VIDEO_TIME     (10000)  /* mili-seconds */

#define RESET_INITIAL_TIME(t) gettimeofday(&(t), NULL)
#define UPDATE_DIFF_TIME(d, then, now) \
    { \
      gettimeofday(&(now), NULL); \
      (d) = ((((now).tv_sec - (then).tv_sec) * 1000) \
                   + (((now).tv_usec - (then).tv_usec) / 1000)); \
    }

#define APP_STATE_BEFORE_CAPTURE  (0)
#define APP_STATE_UNDER_CAPTURE   (1)
#define APP_STATE_AFTER_CAPTURE   (2)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct v_buffer
{
  uint32_t *start;
  uint32_t length;
};

typedef struct v_buffer v_buffer_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int camera_prepare(int fd, enum v4l2_buf_type type,
                          uint32_t buf_mode, uint32_t pixformat,
                          uint16_t hsize, uint16_t vsize,
                          struct v_buffer **vbuf,
                          uint8_t buffernum, int buffersize);
static void free_buffer(struct v_buffer  *buffers, uint8_t bufnum);
static int parse_arguments(int argc, char *argv[],
                           int *capture_num, enum v4l2_buf_type *type);
static int get_camimage(int fd, struct v4l2_buffer *v4l2_buf,
    enum v4l2_buf_type buf_type);
static int release_camimage(int fd, struct v4l2_buffer *v4l2_buf);
static int start_stillcapture(int v_fd, enum v4l2_buf_type capture_type);
static int stop_stillcapture(int v_fd, enum v4l2_buf_type capture_type);

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
 * Name: camera_prepare()
 *
 * Description:
 *   Allocate frame buffer for camera and Queue the allocated buffer
 *   into video driver.
 ****************************************************************************/

static int camera_prepare(int fd, enum v4l2_buf_type type,
                          uint32_t buf_mode, uint32_t pixformat,
                          uint16_t hsize, uint16_t vsize,
                          struct v_buffer **vbuf,
                          uint8_t buffernum, int buffersize)
{
  int ret;
  int cnt;
  struct v4l2_format fmt =
  {
    0
  };

  struct v4l2_requestbuffers req =
  {
    0
  };

  struct v4l2_buffer buf =
  {
    0
  };

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  req.type   = type;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = buffernum;
  req.mode   = buf_mode;

  ret = ioctl(fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
      return ret;
    }

  /* VIDIOC_S_FMT set format */

  fmt.type                = type;
  fmt.fmt.pix.width       = hsize;
  fmt.fmt.pix.height      = vsize;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = pixformat;

  ret = ioctl(fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_S_FMT: errno = %d\n", errno);
      return ret;
    }

  /* Prepare video memory to store images */

  *vbuf = malloc(sizeof(v_buffer_t) * buffernum);

  if (!(*vbuf))
    {
      printf("Out of memory for array of v_buffer_t[%d]\n", buffernum);
      return ERROR;
    }

  for (cnt = 0; cnt < buffernum; cnt++)
    {
      (*vbuf)[cnt].length = buffersize;

      /* Note:
       * VIDIOC_QBUF set buffer pointer.
       * Buffer pointer must be 32bytes aligned.
       */

      (*vbuf)[cnt].start  = memalign(32, buffersize);
      if (!(*vbuf)[cnt].start)
        {
          printf("Out of memory for image buffer of %d/%d\n",
                 cnt, buffernum);

          /* Release allocated memory. */

          while (cnt)
            {
              cnt--;
              free((*vbuf)[cnt].start);
            }

          free(*vbuf);
          *vbuf = NULL;
          return ERROR;
        }
    }

  /* VIDIOC_QBUF enqueue buffer */

  for (cnt = 0; cnt < buffernum; cnt++)
    {
      memset(&buf, 0, sizeof(v4l2_buffer_t));
      buf.type = type;
      buf.memory = V4L2_MEMORY_USERPTR;
      buf.index = cnt;
      buf.m.userptr = (unsigned long)(*vbuf)[cnt].start;
      buf.length = (*vbuf)[cnt].length;

      ret = ioctl(fd, VIDIOC_QBUF, (unsigned long)&buf);
      if (ret)
        {
          printf("Fail QBUF %d\n", errno);
          free_buffer(*vbuf, buffernum);
          *vbuf = NULL;
          return ERROR;
        }
    }

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(fd, VIDIOC_STREAMON, (unsigned long)&type);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
      free_buffer(*vbuf, buffernum);
      *vbuf = NULL;
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: free_buffer()
 *
 * Description:
 *   All free allocated memory of v_buffer.
 ****************************************************************************/

static void free_buffer(struct v_buffer  *buffers, uint8_t bufnum)
{
  uint8_t cnt;
  if (buffers)
    {
      for (cnt = 0; cnt < bufnum; cnt++)
        {
          if (buffers[cnt].start)
            {
              free(buffers[cnt].start);
            }
        }

      free(buffers);
    }
}

/****************************************************************************
 * Name: parse_argument()
 *
 * Description:
 *   Parse and decode commandline arguments.
 ****************************************************************************/

static int parse_arguments(int argc, char *argv[],
                           int *capture_num, enum v4l2_buf_type *type)
{
  if (argc == 1)
    {
      *capture_num = DEFAULT_CAPTURE_NUM;
      *type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
  else if (argc == 2)
    {
      if (strncmp(argv[1], "-jpg", 5) == 0)
        {
          *capture_num = DEFAULT_CAPTURE_NUM;
          *type = V4L2_BUF_TYPE_STILL_CAPTURE;
        }
      else
        {
          *capture_num = atoi(argv[1]);
          if ((*capture_num < 0) || (*capture_num > MAX_CAPTURE_NUM))
            {
              printf("Invalid capture num(%d). must be >=0 and <=%d\n",
                    *capture_num, MAX_CAPTURE_NUM);
              return ERROR;
            }

          *type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        }
    }
  else if (argc == 3)
    {
      if (strncmp(argv[1], "-jpg", 5) == 0)
        {
          *capture_num = atoi(argv[2]);
          if ((*capture_num < 0) || (*capture_num > MAX_CAPTURE_NUM))
            {
              printf("Invalid capture num(%d). must be >=0 and <=%d\n",
                    *capture_num, MAX_CAPTURE_NUM);
              return ERROR;
            }

          *type = V4L2_BUF_TYPE_STILL_CAPTURE;
        }
      else
        {
          printf("Invalid argument 1 : %s\n", argv[1]);
          return ERROR;
        }
    }
  else
    {
      printf("Too many arguments\n");
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: get_camimage()
 *
 * Description:
 *   DQBUF camera frame buffer from video driver with taken picture data.
 ****************************************************************************/

static int get_camimage(int fd, struct v4l2_buffer *v4l2_buf,
    enum v4l2_buf_type buf_type)
{
  int ret;

  /* VIDIOC_DQBUF acquires captured data. */

  memset(v4l2_buf, 0, sizeof(v4l2_buffer_t));
  v4l2_buf->type = buf_type;
  v4l2_buf->memory = V4L2_MEMORY_USERPTR;

  ret = ioctl(fd, VIDIOC_DQBUF, (unsigned long)v4l2_buf);
  if (ret)
    {
      printf("Fail DQBUF %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: release_camimage()
 *
 * Description:
 *   Re-QBUF to set used frame buffer into video driver.
 ****************************************************************************/

static int release_camimage(int fd, struct v4l2_buffer *v4l2_buf)
{
  int ret;

  /* VIDIOC_QBUF sets buffer pointer into video driver again. */

  ret = ioctl(fd, VIDIOC_QBUF, (unsigned long)v4l2_buf);
  if (ret)
    {
      printf("Fail QBUF %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: start_stillcapture()
 *
 * Description:
 *   Start STILL capture stream by TAKEPICT_START if buf_type is
 *   STILL_CAPTURE.
 ****************************************************************************/

static int start_stillcapture(int v_fd, enum v4l2_buf_type capture_type)
{
  int ret;

  if (capture_type == V4L2_BUF_TYPE_STILL_CAPTURE)
    {
      ret = ioctl(v_fd, VIDIOC_TAKEPICT_START, 0);
      if (ret < 0)
        {
          printf("Failed to start taking picture\n");
          return ERROR;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: stop_stillcapture()
 *
 * Description:
 *   Stop STILL capture stream by TAKEPICT_STOP if buf_type is STILL_CAPTURE.
 ****************************************************************************/

static int stop_stillcapture(int v_fd, enum v4l2_buf_type capture_type)
{
  int ret;

  if (capture_type == V4L2_BUF_TYPE_STILL_CAPTURE)
    {
      ret = ioctl(v_fd, VIDIOC_TAKEPICT_STOP, false);
      if (ret < 0)
        {
          printf("Failed to stop taking picture\n");
          return ERROR;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main()
 *
 * Description:
 *   main routine of this example.
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int v_fd;
  int capture_num = DEFAULT_CAPTURE_NUM;
  enum v4l2_buf_type capture_type = V4L2_BUF_TYPE_STILL_CAPTURE;
  struct v4l2_buffer v4l2_buf;
  const char *save_dir;

  struct timeval then;

  struct v_buffer *buffers_video = NULL;
  struct v_buffer *buffers_still = NULL;

  /* =====  Parse and Check arguments  ===== */

  ret = parse_arguments(argc, argv, &capture_num, &capture_type);
  if (ret != OK)
    {
      printf("usage: %s ([-jpg]) ([capture num])\n", argv[0]);
      return ERROR;
    }

  /* =====  Initialization Code  ===== */

  /* Select storage to save image files */

  save_dir = futil_initialize();

  /* Open the device file. */

  v_fd = open("/dev/video", 0);
  if (v_fd < 0)
    {
      printf("ERROR: Failed to open video.errno = %d\n", errno);
      ret = ERROR;
      goto exit_without_cleaning_buffer;
    }

  /* Prepare for STILL_CAPTURE stream.
   *
   * The video buffer mode is V4L2_BUF_MODE_FIFO mode.
   * In this FIFO mode, if all VIDIOC_QBUFed frame buffers are captured image
   * and no additional frame buffers are VIDIOC_QBUFed, the capture stops and
   * waits for new VIDIOC_QBUFed frame buffer.
   * And when new VIDIOC_QBUF is executed, the capturing is resumed.
   *
   * Allocate freame buffers for FullHD JPEG size (512KB).
   * Number of frame buffers is defined as STILL_BUFNUM(1).
   * And all allocated memorys are VIDIOC_QBUFed.
   */

  ret = camera_prepare(v_fd, V4L2_BUF_TYPE_STILL_CAPTURE,
                       V4L2_BUF_MODE_FIFO, V4L2_PIX_FMT_JPEG,
                       VIDEO_HSIZE_FULLHD, VIDEO_VSIZE_FULLHD,
                       &buffers_still, STILL_BUFNUM, IMAGE_JPG_SIZE);
  if (ret != OK)
    {
      goto exit_this_app;
    }

  /* Prepare for VIDEO_CAPTURE stream.
   *
   * The video buffer mode is V4L2_BUF_MODE_RING mode.
   * In this RING mode, if all VIDIOC_QBUFed frame buffers are captured image
   * and no additional frame buffers are VIDIOC_QBUFed, the capture continues
   * as the oldest image in the V4L2_BUF_QBUFed frame buffer is reused in
   * order from the captured frame buffer and a new camera image is
   * recaptured.
   *
   * Allocate freame buffers for QVGA YUV422 size (320x240x2=150KB).
   * Number of frame buffers is defined as VIDEO_BUFNUM(3).
   * And all allocated memorys are VIDIOC_QBUFed.
   */

  ret = camera_prepare(v_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE,
                       V4L2_BUF_MODE_RING, V4L2_PIX_FMT_UYVY,
                       VIDEO_HSIZE_QVGA, VIDEO_VSIZE_QVGA,
                       &buffers_video, VIDEO_BUFNUM, IMAGE_YUV_SIZE);
  if (ret != OK)
    {
      goto exit_this_app;
    }

  printf("Take %d pictures as %s file in %s after %d mili-seconds.\n",
         capture_num,
         (capture_type == V4L2_BUF_TYPE_STILL_CAPTURE) ? "JPEG" : "YUV",
         save_dir, START_CAPTURE_TIME);
  printf(" After taking pictures, the app will be exit after %d ms.\n",
      KEEP_VIDEO_TIME);

  RESET_INITIAL_TIME(then);

  /* =====  Main Loop  ===== */

  while (1)
    {
      printf("Start capturing...\n");
      ret = start_stillcapture(v_fd, capture_type);
      if (ret != OK)
        {
          goto exit_this_app;
        }

      while (capture_num)
        {
          ret = get_camimage(v_fd, &v4l2_buf, capture_type);
          if (ret != OK)
            {
              goto exit_this_app;
            }

          futil_writeimage((uint8_t *)v4l2_buf.m.userptr,
           (size_t)v4l2_buf.bytesused,
           (capture_type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? "YUV" : "JPG");

          ret = release_camimage(v_fd, &v4l2_buf);
          if (ret != OK)
            {
              goto exit_this_app;
            }

          capture_num--;
        }

      ret = stop_stillcapture(v_fd, capture_type);
      if (ret != OK)
        {
          goto exit_this_app;
        }

      RESET_INITIAL_TIME(then);
      printf("Finished capturing...\n");
    }

exit_this_app:

  /* Close video device file makes dequqe all buffers */

  close(v_fd);

  free_buffer(buffers_video, VIDEO_BUFNUM);
  free_buffer(buffers_still, STILL_BUFNUM);

exit_without_cleaning_buffer:

  video_uninitialize();

  return ret;
}
