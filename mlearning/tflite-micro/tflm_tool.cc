/****************************************************************************
 * apps/mlearning/tflite-micro/tflm_tool.cc
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <memory>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  printf("\nUtility to use tflite micro on nuttx.\n"
    "[ -C       ] Compile tflite model into c++ codes.\n"
    "[ -E       ] Do once evaluation (for profiling).\n"
    "[ -i <str> ] Readable model file path.\n"
    "[ -o <str> ] Writable c++ file path (required with -C).\n"
    "[ -p <str> ] Prefix of compiled code.\n"
    "[ -a <int> ] Arena size (mempool).\n"
    "[ -h       ] Print this message.\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" int main(int argc, FAR char* argv[])
{
  const char* modelFileName = nullptr;
  const char* codeFileName = nullptr;
  const char* prefix = "NXAI";
  bool need_compile = false;
  bool need_invoke = false;
  int arenaSize = 1024 * 8;

  int ch;
  while ((ch = getopt(argc, argv, "CEhi:o:p:a:")) != EOF)
    {
      switch (ch)
        {
          case 'C':
            need_compile = true;
            break;
          case 'E':
            need_invoke = true;
            break;
          case 'p':
            prefix = optarg;
            break;
          case 'i':
            modelFileName = optarg;
            break;
          case 'o':
            codeFileName = optarg;
            break;
          case 'a':
            arenaSize = strtol(optarg, NULL, 0);
            break;
          case 'h':
          default:
            usage();
            return -1;
        }
    }

  if (!modelFileName || (need_compile && !codeFileName))
    {
      usage();
      return -1;
    }

  std::ifstream ifs(modelFileName, std::ios::binary);
  if (!ifs)
    {
      printf("Failed to open model file: %s\n", modelFileName);
      return -1;
    }

  ifs.seekg(0, std::ios::end);
  size_t modelSize = ifs.tellg();
  std::unique_ptr<uint8_t[]> pModel(new uint8_t[modelSize]);

  ifs.seekg(0, std::ios::beg);
  ifs.read(reinterpret_cast<char*>(pModel.get()), modelSize);
  ifs.close();

  tflite::MicroMutableOpResolver<8> resolver;
  resolver.AddConv2D();
  resolver.AddMaxPool2D();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddMean();
  resolver.AddReshape();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();

  std::unique_ptr<uint8_t[]> pArena(new uint8_t[arenaSize]);

  tflite::MicroProfiler profiler;
  tflite::MicroInterpreter interpreter(tflite::GetModel(pModel.get()),
    resolver, pArena.get(), arenaSize, nullptr,
    reinterpret_cast<tflite::MicroProfilerInterface*>(&profiler));

  TfLiteStatus status = interpreter.AllocateTensors();
  if (status != kTfLiteOk)
    {
      printf("AllocateTensors failed: %d\n", status);
      return -1;
    }

  if (need_invoke)
    {
      status = interpreter.Invoke();
      if (status != kTfLiteOk)
        {
          printf("Invoke failed: %d\n", status);
          return -1;
        }

      profiler.LogCsv();
      profiler.LogTicksPerTagCsv();
    }

  if (need_compile)
    {
#ifdef TFLITE_MODEL_COMPILER
      std::ofstream ofs(codeFileName);
      interpreter.Compile(ofs, prefix);
      ofs.close();
#else
      printf("Not supported compiling %s.\n", prefix);
#endif
    }

  printf("nxai done!\n");
  return 0;
}