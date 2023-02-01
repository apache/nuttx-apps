/*
 * Espressif Systems Wireless LAN device driver
 *
 * Copyright (C) 2015-2022 Espressif Systems (Shanghai) PTE LTD
 * SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include "serial_if.h"
#include "platform_wrapper.h"
#include "ctrl_api.h"
#include "esp_hosted_config.pb-c.h"
#include <pthread.h>
#include "string.h"
#include <time.h>
#include <signal.h>

#define SUCCESS                 0
#define FAILURE                 -1
#define DUMMY_READ_BUF_LEN      64
#define EAGAIN                  11

#define HOSTED_CALLOC(buff,nbytes) do {                           \
    buff = (uint8_t *)hosted_calloc(1, nbytes);                   \
    if (!buff) {                                                  \
        printf("%s, Failed to allocate memory \n", __func__);     \
        goto free_bufs;                                           \
    }                                                             \
} while(0);

#define thread_handle_t pthread_t
#define semaphore_handle_t sem_t


struct serial_drv_handle_t {
	int file_desc;
};

struct timer_handle_t {
	timer_t timer_id;
};

static struct serial_drv_handle_t* serial_drv_handle;

extern int errno;

int control_path_platform_init(void)
{
	/* 1. Open serial file
	 * 2. Flush all data available for reading
	 * 3. Close serial file
	 **/
	int ret = 0, count = 0;
	uint8_t *buf = NULL;
	struct serial_drv_handle_t init_serial_handle = {0};
	const char* transport = SERIAL_IF_FILE;

	/* open file */
	init_serial_handle.file_desc = open(transport,O_NONBLOCK|O_RDWR);
	if (init_serial_handle.file_desc == -1) {
		printf("Failed to open driver interface \n");
		return FAILURE;
	}

	buf = (uint8_t *)hosted_calloc(1, DUMMY_READ_BUF_LEN);
	if (!buf) {
		printf("%s, Failed to allocate memory \n", __func__);
		goto close1;
	}

	do {
		/* dummy read, discard data */
		count = read(init_serial_handle.file_desc,
				(buf), (DUMMY_READ_BUF_LEN));
		if (count < 0) {
			if (-errno != -EAGAIN) {
				perror("Failed to read ringbuffer:\n");
				goto close;
			}
			break;
		}
	} while (count>0);

	mem_free(buf);
	/* close file */
	ret = close(init_serial_handle.file_desc);
	if (ret < 0) {
		perror("close:");
		return FAILURE;
	}
	return SUCCESS;

close:
	mem_free(buf);
close1:
	ret = close(init_serial_handle.file_desc);
	if (ret < 0) {
		perror("close: Failed to close interface for control path platform init:");
	}
	return FAILURE;

}

int control_path_platform_deinit(void)
{
	/* Empty as if now */
	return SUCCESS;
}

/* -------- Memory ---------- */
void* hosted_malloc(size_t size)
{
	return malloc(size);
}

void* hosted_calloc(size_t blk_no, size_t size)
{
	return calloc(blk_no, size);
}

void hosted_free(void *ptr)
{
	free(ptr);
}

/* -------- Threads ---------- */

typedef void (*hosted_thread_cb_t) (void const* arg);

struct thread_arg_t {
	hosted_thread_cb_t thread_cb;
	void * arg;
};

static void *thread_routine(void *arg)
{
	struct thread_arg_t *thread_arg = arg;
	struct thread_arg_t new_thread_arg = {0};

	new_thread_arg.thread_cb = thread_arg->thread_cb;
	new_thread_arg.arg = thread_arg->arg;

	mem_free(thread_arg);

	if (new_thread_arg.thread_cb)
		new_thread_arg.thread_cb(new_thread_arg.arg);
	else
		printf("NULL func, failed to call callback\n");
	return NULL;
}

void *hosted_thread_create(void (*start_routine)(void const *), void *arg)
{
	struct thread_arg_t *thread_arg = (struct thread_arg_t *)hosted_malloc(sizeof(struct thread_arg_t));
	thread_handle_t *thread_handle = (thread_handle_t *)hosted_malloc(
			sizeof(thread_handle_t));

	if (!thread_arg || !thread_handle) {
		printf("Falied to allocate thread handle\n");
		return NULL;
	}

	thread_arg->thread_cb = start_routine;
	thread_arg->arg = arg;

	if (pthread_create(thread_handle,
			NULL, thread_routine, thread_arg)) {
		printf("Failed in pthread_create\n");
		mem_free(thread_handle);
		return NULL;
	}
	return thread_handle;
}

int hosted_thread_cancel(void *thread_handle)
{
	thread_handle_t *thread_hdl = NULL;

	if (!thread_handle)
		return FAILURE;
	thread_hdl = (thread_handle_t *)thread_handle;

	int s = pthread_cancel(*thread_hdl);
	if (s != 0) {
		mem_free(thread_handle);
		printf("Prob in pthread_cancel\n");
		return FAILURE;
	}

	s = pthread_join(*thread_hdl, NULL);
	if (s != 0) {
		mem_free(thread_handle);
		printf("prob in pthread_join\n");
		return FAILURE;
	}
	mem_free(thread_handle);
	return SUCCESS;
}

/* -------- Semaphores ---------- */
void * hosted_create_semaphore(int init_value)
{
	semaphore_handle_t *sem_id = NULL;

	sem_id = (semaphore_handle_t*)hosted_malloc(
			sizeof(semaphore_handle_t));

	if (!sem_id)
		return NULL;

	if (sem_init(sem_id, 0, init_value)) {
		printf("read sem init failed\n");
		mem_free(sem_id);
		return NULL;
	}

	return sem_id;
}

static int wait_for_timeout(sem_t *sem_id, int timeout_sec)
{
	int ret = 0;
	struct timespec ts;

	/* current time stamp, (used later for timeout calculation) */
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
	{
		/* handle error */
		printf("Failed to get current timestamp\n");
		return FAILURE;
	}

	/* Wait for timeout duration or till someone post this sem */
	ts.tv_sec += timeout_sec;
	while ((ret = sem_timedwait(sem_id, &ts)) == -1 && errno == EINTR)
		continue;       /* Restart if interrupted by handler */

	if (ret<0)
		return FAILURE;
	return SUCCESS;
}

int hosted_get_semaphore(void * semaphore_handle, int timeout)
{
	semaphore_handle_t *sem_id = NULL;

	if (!semaphore_handle) {
		printf("uninitialised sem id\n");
		return FAILURE;
	}

	sem_id = (semaphore_handle_t *)semaphore_handle;

	if (!timeout) {
		/* non blocking */
		return sem_trywait(sem_id);
	} else if (timeout<0) {
		/* Blocking */
		return sem_wait(sem_id);
	} else {
		return wait_for_timeout(sem_id, timeout);
	}
}

int hosted_post_semaphore(void * semaphore_handle)
{
	semaphore_handle_t *sem_id = NULL;

	if (!semaphore_handle) {
		printf("uninitialised sem id\n");
		return FAILURE;
	}

	sem_id = (semaphore_handle_t *)semaphore_handle;
	return sem_post(sem_id);
}

int hosted_destroy_semaphore(void * semaphore_handle)
{
	int ret = SUCCESS;
	semaphore_handle_t *sem_id = NULL;

	if (!semaphore_handle) {
		printf("uninitialised sem id\n");
		return FAILURE;
	}

	sem_id = (semaphore_handle_t *)semaphore_handle;

	ret = sem_destroy(sem_id);
	if(ret)
		printf("Failed to destroy sem\n");

	mem_free(semaphore_handle);

	return ret;
}

/* -------- Timers  ---------- */

int hosted_timer_stop(void *timer_handle)
{
	if (timer_handle) {
		int ret = timer_delete(((struct timer_handle_t *)timer_handle)->timer_id);
		if (ret < 0)
			printf("Failed to stop timer\n");

		mem_free(timer_handle);
		return ret;
	}
	return FAILURE;
}

/* Sample timer_handler looks like this:
 *
 * void expired(union sigval timer_data){
 *     struct mystruct *a = timer_data.sival_ptr;
 * 	printf("Expired %u\n", a->mydata++);
 * }
 **/

typedef void (*hosted_timer_cb_t) (void const* resp);

struct timer_arg_t {
	hosted_timer_cb_t timer_cb;
	void * arg;
};

static void timer_ll_callback(union sigval timer_data)
{
	struct timer_arg_t *timer_arg = timer_data.sival_ptr;

	if (timer_arg->timer_cb)
		timer_arg->timer_cb(timer_arg->arg);
	else
		printf("NULL func, failed to call callback\n");
}

void *hosted_timer_start(int duration, int type,
		void (*timeout_handler)(void const *), void * arg)
{
	int res = 0;
	struct timer_arg_t timer_arg;
	struct timer_handle_t *timer_handle = (struct timer_handle_t *)hosted_malloc(
			sizeof(struct timer_handle_t));

	if (!timer_handle) {
		printf("Mem Alloc for Timer failed\n");
		mem_free(timer_handle);
		return NULL;
	}

	/*  sigevent specifies behaviour on expiration  */
	struct sigevent sev = { 0 };

	/* specify start delay and interval
	 * it_value and it_interval must not be zero */


	struct itimerspec its = {   .it_value.tv_sec  = duration,
		.it_value.tv_nsec = 0,
		.it_interval.tv_sec  = 0,
		.it_interval.tv_nsec = 0
	};

	timer_arg.timer_cb = timeout_handler;
	timer_arg.arg = arg;

	if (type == CTRL__TIMER_PERIODIC) {
		its.it_interval.tv_sec = duration;
	}

	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = timer_ll_callback;
	sev.sigev_signo = SIGRTMAX-1;
	sev.sigev_value.sival_ptr = &timer_arg;


	/* create timer */
	res = timer_create(CLOCK_REALTIME, &sev, &(timer_handle->timer_id));

	if (res != 0){
		fprintf(stderr, "Error timer_create: %s\n", strerror(errno));
		mem_free(timer_handle);
		return NULL;
	}

	/* start timer */
	res = timer_settime(timer_handle->timer_id, 0, &its, NULL);

	if (res != 0){
		fprintf(stderr, "Error timer_settime: %s\n", strerror(errno));
		mem_free(timer_handle);
		return NULL;
	}

	return timer_handle;
}


/* -------- Serial Drv ---------- */
struct serial_drv_handle_t* serial_drv_open(const char *transport)
{
	if (!transport) {
		return NULL;
	}

	if(serial_drv_handle) {
		//printf("return orig hndl\n");
		return serial_drv_handle;
	}

	serial_drv_handle = (struct serial_drv_handle_t *)
		hosted_calloc(1, sizeof(struct serial_drv_handle_t));
	if (!serial_drv_handle) {
		printf("%s, Failed to allocate memory \n",__func__);
		return NULL;
	}

	serial_drv_handle->file_desc = open(transport, O_RDWR);
	if (serial_drv_handle->file_desc == -1) {
		mem_free(serial_drv_handle);
		return NULL;
	}

	return serial_drv_handle;
}

int serial_drv_write (struct serial_drv_handle_t *serial_drv_handle,
		uint8_t *buf, int in_count, int *out_count)
{
	if (!serial_drv_handle ||
	    serial_drv_handle->file_desc < 0 ||
	    !buf || !in_count || !out_count) {
		printf("%s:%u Invalid arguments\n", __func__, __LINE__);
		goto free_bufs;
	}

	*out_count = write(serial_drv_handle->file_desc, buf, in_count);
	if (*out_count <= 0) {
		perror("write: ");
		goto free_bufs;
	}
	mem_free(buf);
	return SUCCESS;

free_bufs:
	mem_free(buf);
	return FAILURE;
}

int serial_drv_close(struct serial_drv_handle_t **serial_drv_handle)
{
	if (!serial_drv_handle ||
	    !(*serial_drv_handle) ||
	    (*serial_drv_handle)->file_desc < 0) {
		return FAILURE;
	}
	if(close((*serial_drv_handle)->file_desc) < 0) {
		perror("close:");
		mem_free(*serial_drv_handle);
		return FAILURE;
	}
	if (*serial_drv_handle) {
		mem_free(*serial_drv_handle);
	}
	return SUCCESS;
}

/* This whole processing of two step parsing TLV is common for MPU and MCU
 * and ideally this processing should have been done in serial_if.c.
 * But the problem is there is difference in reading in MPU and MCU.
 * For MPU, it is straight forward, read on character driver file and
 * partial reads are supported.
 * But For MCU, the problem is it doesn't have that capability and gets complete
 * serial buffer on transport.
 * To keep it simple, two step parsing TLV buffer is kept in platform specific code
 */
uint8_t * serial_drv_read(struct serial_drv_handle_t *serial_drv_handle,
		uint32_t *out_nbyte)
{
	int ret = 0, count = 0, total_read_len = 0;
	const char* ep_name = CTRL_EP_NAME_RESP;
	uint16_t init_read_len = SIZE_OF_TYPE + SIZE_OF_LENGTH + strlen(ep_name) +
		SIZE_OF_TYPE + SIZE_OF_LENGTH;
	uint8_t init_read_buf[init_read_len];
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	/* Any of `CTRL_EP_NAME_EVENT` and `CTRL_EP_NAME_RESP` could be used,
	 * as both have same strlen in adapter.h */

/*
 * Read fixed length of received data in below format:
 * ----------------------------------------------------------------------------
 *  Endpoint Type | Endpoint Length | Endpoint Value  | Data Type | Data Length
 * ----------------------------------------------------------------------------
 *
 *  Bytes used per field as follows:
 *  ---------------------------------------------------------------------------
 *      1         |       2         | Endpoint Length |     1     |     2     |
 *  ---------------------------------------------------------------------------
 */

	if (!serial_drv_handle ||
	    serial_drv_handle->file_desc < 0 ||
	    !out_nbyte) {
		printf("%s:%u Invalid parameter\n",__func__,__LINE__);
		return NULL;
	}

	memset(init_read_buf, 0, sizeof(init_read_buf));

	total_read_len = 0;
	do {
		count = read(serial_drv_handle->file_desc,
				(init_read_buf+total_read_len), (init_read_len-total_read_len));
		if (count <= 0) {
			perror("read fail:");
			printf("Exp read of %u bytes: ret[%d]\n",
					(init_read_len-total_read_len), count);
			goto free_bufs;
		}
		total_read_len += count;
	} while (total_read_len < init_read_len);

	if (total_read_len != init_read_len) {
		printf("%s, read_bytes exp[%d] vs recvd[%d]\n"
				,__func__, init_read_len, total_read_len);
		goto free_bufs;
	}

	ret = parse_tlv(init_read_buf, &buf_len);
	if ((ret != SUCCESS) || !buf_len) {
		goto free_bufs;
	}

	/*
	 * Read variable length of received data.
	 * Variable length is obtained after
	 * parsing of previously read data.
	 */
	HOSTED_CALLOC(buf,buf_len);

	total_read_len = 0;
	do {
		count = read(serial_drv_handle->file_desc,
				(buf+total_read_len), (buf_len-total_read_len));
		if (count <= 0) {
			perror("Fail to read serial data");
			break;
		}
		total_read_len += count;
	} while (total_read_len < buf_len);

	if (total_read_len != buf_len) {
		printf("%s, Exp num_bytes[%d] != recvd[%d]\n",
				__func__, buf_len, total_read_len);
		goto free_bufs;
	}

	*out_nbyte = buf_len;
	return buf;

free_bufs:
	mem_free(buf);
	*out_nbyte = 0;
	return NULL;
}
