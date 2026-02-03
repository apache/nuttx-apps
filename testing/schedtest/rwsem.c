/****************************************************************************
 * apps/testing/schedtest/rwsem.c
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
#include <nuttx/rwsem.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

typedef struct
{
  rw_semaphore_t rwsem;
  int shared_resource;
}shared_data_t;

typedef struct
{
    shared_data_t *data;
    int id;
}reader_arg_t;

static void *reader_thread(void *arg);
static void *writer_thread(void *arg);
static void test_rwsem(void **state);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *writer_thread(void *arg)
{
    shared_data_t *data = (shared_data_t *)arg;
    int id = 1;

    down_write(&data->rwsem);
    printf("writer %d acquired write lock.\n", id);
    data->shared_resource += 10;
    printf("writer %d updated shared_resource "
           "to %d.\n", id, data->shared_resource);
    sleep(2);
    up_write(&data->rwsem);
    printf("writer %d released write lock.\n", id);

  return NULL;
}

static void *reader_thread(void *arg)
{
  reader_arg_t *args = (reader_arg_t *)arg;
  shared_data_t *data = args->data;
  int id = args->id;

  switch (id)
    {
      case 1:
        printf("Reader %d acquired read lock (blocking).\n", id);
        down_read(&data->rwsem);
        printf("Reader %d acquired read lock.\n", id);
        assert_int_equal(data->shared_resource, 10);
        up_read(&data->rwsem);
        printf("Reader %d released read lock.\n", id);
        break;

      case 2:
        printf("Reader %d acquired read lock (blocking).\n", id);
        int acquired = down_read_trylock(&data->rwsem);
        assert_int_equal(acquired, 0);
        printf("Reader %d failed to acquire read lock as expected.\n", id);
        break;

      case 3:
        printf("Reader %d attempt to acquire first read lock.\n", id);
        down_read(&data->rwsem);
        printf("Reader %d acquired first read lock.\n", id);

        printf("Reader %d attempt to acquire second read lock.\n", id);
        down_read(&data->rwsem);
        assert_int_equal(data->shared_resource, 10);

        up_read(&data->rwsem);
        printf("Reader %d released read lock.\n", id);

        up_read(&data->rwsem);
        printf("Reader %d released read lock.\n", id);
        break;

      default:
        printf("Unknwown reader id: %d\n", id);
        break;
    }

  free(args);
  return NULL;
}

static void test_rwsem(void **state)
{
  shared_data_t *data = (shared_data_t *)*state;
  pthread_t readers[3], writer;
  int rd_id[] =
    {
      1, 2, 3
    };

  int i;
  init_rwsem(&data->rwsem);
  data->shared_resource = 0;

  pthread_create(&writer,  NULL, writer_thread, data);

  for (i = 0; i < 3; i++)
    {
        reader_arg_t *args = malloc(sizeof(reader_arg_t));
        args->data = data;
        args->id = rd_id[i];
        pthread_create(&readers[i], NULL, reader_thread, args);
    }

  for (i = 0; i < 3; i++)
    {
      pthread_join(readers[i], NULL);
    }

  pthread_join(writer, NULL);

  destroy_rwsem(&data->rwsem);
  printf("rwsem tests completed.\n");
}

int main(int argc, FAR char *argv[])
{
  cmocka_set_message_output(CM_OUTPUT_STDOUT);

  shared_data_t data;
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_prestate(test_rwsem, &data),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
