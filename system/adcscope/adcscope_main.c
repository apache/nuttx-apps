/****************************************************************************
 * apps/system/adcscope/adcscope_main.c
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

#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#include "logging/nxscope/nxscope.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Channel name length */

#define CHNAME_MAX   8

/* ADC data type */

#ifdef CONFIG_SYSTEM_ADCSCOPE_INT16
#  define ADATA_PUT    nxscope_put_int16
#  define ADATA_DTYPE  NXSCOPE_TYPE_INT16
#elif defined(CONFIG_SYSTEM_ADCSCOPE_INT32)
#  define ADATA_PUT    nxscope_put_int32
#  define ADATA_DTYPE  NXSCOPE_TYPE_INT32
#else
#  error ADC data type not selected
#endif

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

struct adcscope_thr_env_s
{
  /* NxScope data */

  struct nxscope_s           nxs;
  struct nxscope_cfg_s       nxs_cfg;
  struct nxscope_intf_s      intf;
  struct nxscope_proto_s     proto;
  struct nxscope_callbacks_s cbs;
  struct nxscope_ser_cfg_s   nxs_ser_cfg;

  /* ADC data */

  int                        fd;
  size_t                     channels;
  size_t                     data_size;
  FAR struct adc_msg_s      *data;
  FAR char                  *chname;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adcscope_env_init
 ****************************************************************************/

static int adcscope_env_init(FAR struct adcscope_thr_env_s *envp)
{
  int ret = OK;

  /* Open ADC device */

  envp->fd = open(CONFIG_SYSTEM_ADCSCOPE_ADC_PATH, O_RDWR);
  if (envp->fd <= 0)
    {
      printf("ERROR: failed to open %s\n", CONFIG_SYSTEM_ADCSCOPE_ADC_PATH);
      ret = -errno;
      goto errout;
    }

  /* Get number of channels */

  envp->channels = ioctl(envp->fd, ANIOC_GET_NCHANNELS, 0);
  if (envp->channels <= 0)
    {
      printf("ANIOC_GET_NCHANNELS ioctl failed: %d\n", envp->channels);
      ret = -errno;
      goto errout;
    }

  /* Allocate data buffer */

  envp->data_size = sizeof(struct adc_msg_s) * envp->channels *
    CONFIG_SYSTEM_ADCSCOPE_ADCBUF_LEN;
  envp->data      = zalloc(envp->data_size);
  if (!envp->data)
    {
      printf("ERROR: zalloc failed %d\n", -errno);
      ret = -errno;
      goto errout;
    }

  /* Allocate channels name buffer */

  envp->chname = zalloc(envp->channels * CHNAME_MAX);
  if (!envp->chname)
    {
      printf("ERROR: zalloc failed %d\n", -errno);
      ret = -errno;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: adcscope_env_clean
 ****************************************************************************/

static void adcscope_env_clean(FAR struct adcscope_thr_env_s *envp)
{
  /* Free buffers */

  free(envp->data);
  free(envp->chname);
}

/****************************************************************************
 * Name: nxscope_cb_start
 ****************************************************************************/

static int nxscope_cb_start(FAR void *priv, bool start)
{
  UNUSED(priv);

  printf("--> nxscope_cb_start: start=%d\n", start);

  return OK;
}

/****************************************************************************
 * Name: adcscope_nxscope_init
 ****************************************************************************/

static int adcscope_nxscope_init(FAR struct adcscope_thr_env_s *envp)
{
  int ret;

  /* Default serial protocol */

  ret = nxscope_proto_ser_init(&envp->proto, NULL);
  if (ret < 0)
    {
      printf("ERROR: nxscope_proto_ser_init failed %d\n", ret);
      goto errout_noproto;
    }

  /* Configuration */

  envp->nxs_ser_cfg.path      = CONFIG_SYSTEM_ADCSCOPE_SERIAL_PATH;
  envp->nxs_ser_cfg.nonblock  = true;
  envp->nxs_ser_cfg.baud      = CONFIG_SYSTEM_ADCSCOPE_SERIAL_BAUD;

  /* Initialize serial interface */

  ret = nxscope_ser_init(&envp->intf, &envp->nxs_ser_cfg);
  if (ret < 0)
    {
      printf("ERROR: nxscope_ser_init failed %d\n", ret);
      goto errout_nointf;
    }

  /* Connect callbacks */

  envp->cbs.start_priv        = NULL;
  envp->cbs.start             = nxscope_cb_start;

  /* Initialize nxscope */

  envp->nxs_cfg.intf_cmd      = &envp->intf;
  envp->nxs_cfg.intf_stream   = &envp->intf;
  envp->nxs_cfg.proto_cmd     = &envp->proto;
  envp->nxs_cfg.proto_stream  = &envp->proto;
  envp->nxs_cfg.callbacks     = &envp->cbs;
  envp->nxs_cfg.channels      = envp->channels;
  envp->nxs_cfg.streambuf_len = CONFIG_SYSTEM_ADCSCOPE_STREAMBUF_LEN;
  envp->nxs_cfg.rxbuf_len     = CONFIG_SYSTEM_ADCSCOPE_RXBUF_LEN;
  envp->nxs_cfg.rx_padding    = CONFIG_SYSTEM_ADCSCOPE_RX_PADDING;

  ret = nxscope_init(&envp->nxs, &envp->nxs_cfg);
  if (ret < 0)
    {
      printf("ERROR: nxscope_init failed %d\n", ret);
      goto errout_nonxscope;
    }

  return OK;

errout_nonxscope:
  nxscope_proto_ser_deinit(&envp->proto);
errout_nointf:
  nxscope_ser_deinit(&envp->intf);
errout_noproto:
  return ret;
}

/****************************************************************************
 * Name: adcscope_nxscope_clean
 ****************************************************************************/

static void adcscope_nxscope_clean(FAR struct adcscope_thr_env_s *envp)
{
  /* Deinit nxscope */

  nxscope_deinit(&envp->nxs);

  /* Deinit protocol */

  nxscope_proto_ser_deinit(&envp->proto);

  /* Deinit interface */

  nxscope_ser_deinit(&envp->intf);
}

/****************************************************************************
 * Name: adcscope_samples_thr
 ****************************************************************************/

static FAR void *adcscope_samples_thr(FAR void *arg)
{
  FAR struct adcscope_thr_env_s *envp = arg;
  int                           ret;
  size_t                        ptr;
  size_t                        i;

  DEBUGASSERT(envp);

  printf("adcscope_samples_thr\n");

  while (1)
    {
#ifdef CONFIG_SYSTEM_ADCSCOPE_SWTRIG
      /* Issue the software trigger to start ADC conversion */

      ret = ioctl(envp->fd, ANIOC_TRIGGER, 0);
      if (ret < 0)
        {
          printf("ERROR: ANIOC_TRIGGER ioctl failed: %d\n", -errno);
          return NULL;
        }
#endif

      /* Read data from ADC */

      ret = read(envp->fd, envp->data, envp->data_size);
      if (ret < 0)
        {
          printf("ERROR: read failed %d\n", -errno);
        }
      else
        {
          /* Feed data to nxscope */

          for (ptr = 0; ptr < ret / sizeof(struct adc_msg_s); )
            {
              for (i = 0; i < envp->channels; i++)
                {
                  ADATA_PUT(&envp->nxs, i, envp->data[ptr].am_data);
                  ptr += 1;
                }
            }
        }

#if CONFIG_SYSTEM_ADCSCOPE_FETCH_INTERVAL > 0
      usleep(CONFIG_SYSTEM_ADCSCOPE_FETCH_INTERVAL);
#endif
    }

  return NULL;
}

#ifdef CONFIG_SYSTEM_ADCSCOPE_CDCACM
/****************************************************************************
 * Name: adcscope_cdcacm_init
 ****************************************************************************/

static int adcscope_cdcacm_init(void)
{
  struct boardioc_usbdev_ctrl_s  ctrl;
  FAR void                      *handle;
  int                            ret;

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.handle   = &handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("ERROR: BOARDIOC_USBDEV_CONTROL failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: adcscope_channels
 ****************************************************************************/

static int adcscope_channels(FAR struct adcscope_thr_env_s *envp)
{
  union nxscope_chinfo_type_u  u;
  FAR char *chname;
  size_t i;

  for (i = 0; i < envp->channels; i++)
    {
      /* Get channel name */

      chname = &envp->chname[i * CHNAME_MAX];
      snprintf(chname, CHNAME_MAX, "chan%d", i);

      /* Register nxscope channel */

      u.s.dtype = ADATA_DTYPE;
      u.s._res  = 0;
      u.s.cri   = 0;

      nxscope_chan_init(&envp->nxs, i, chname, u.u8, 1, 0);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adcscope_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct adcscope_thr_env_s env;
  pthread_t                 thread;
  int                       ret;

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#  ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#  endif
#endif

#ifdef CONFIG_SYSTEM_ADCSCOPE_CDCACM
  /* Initialize the USB CDCACM device */

  ret = adcscope_cdcacm_init();
  if (ret < 0)
    {
      printf("ERROR: adcscope_cdcacm_init failed %d\n", ret);
      goto errout_noenv;
    }
#endif

  /* Initialize environment */

  ret = adcscope_env_init(&env);
  if (ret < 0)
    {
      printf("ERROR: failed to open %s\n", CONFIG_SYSTEM_ADCSCOPE_ADC_PATH);
      goto errout_nonxscope;
    }

  /* Initialize NxScope */

  ret = adcscope_nxscope_init(&env);
  if (ret < 0)
    {
      printf("ERROR: failed to init nxscope\n");
      goto errout_nonxscope;
    }

  /* Create channels */

  ret = adcscope_channels(&env);
  if (ret != OK)
    {
      printf("ERROR: adcscope_channels failed %d\n", ret);
      goto errout;
    }

  /* Create samples thread */

  ret = pthread_create(&thread, NULL, adcscope_samples_thr, &env);
  if (ret != OK)
    {
      printf("ERROR: pthread_create failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_SYSTEM_ADCSCOPE_FORCE_ENABLE
  /* Enable channels and enable stream */

  nxscope_chan_all_en(&nxs, true);
  nxscope_stream_start(&nxs, true);
#endif

  /* Main loop */

  while (1)
    {
      /* Flush stream data */

      ret = nxscope_stream(&env.nxs);
      if (ret < 0)
        {
          printf("ERROR: nxscope_stream failed %d\n", ret);
        }

      /* Handle recv data */

      ret = nxscope_recv(&env.nxs);
      if (ret < 0)
        {
          printf("ERROR: nxscope_recv failed %d\n", ret);
        }

      usleep(CONFIG_SYSTEM_ADCSCOPE_MAIN_INTERVAL);
    }

errout:
  adcscope_nxscope_clean(&env);
errout_nonxscope:
  adcscope_env_clean(&env);
#ifdef CONFIG_SYSTEM_ADCSCOPE_CDCACM
errout_noenv:
#endif
  return 0;
}
