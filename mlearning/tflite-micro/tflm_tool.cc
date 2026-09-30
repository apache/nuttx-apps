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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <new>
#include <utility>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/micro_utils.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/schema/schema_utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum number of distinct operators a model may use */

#define TFLM_MAX_OPS        32

/* Maximum number of input tensors that can be given data with -d/-x */

#define TFLM_MAX_INPUTS     4

/* Maximum number of elements printed per output tensor */

#define TFLM_MAX_PRINT      64

#define TFLM_OP(code, name) \
  case tflite::BuiltinOperator_##code: \
    return resolver.Add##name();

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef tflite::MicroMutableOpResolver<TFLM_MAX_OPS> tflm_resolver_t;

struct tflm_input_s
{
  FAR const char *arg;
  bool is_file;
};

/* TFLM classes may hide operator delete, so destroy them explicitly */

template <typename T>
struct tflm_deleter
{
  void operator()(FAR T *obj) const
  {
    obj->~T();
    ::operator delete(obj);
  }
};

template <typename T>
using tflm_ptr = std::unique_ptr<T, tflm_deleter<T>>;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

template <typename T, typename... Args>
static FAR T *tflm_new(Args &&... args)
{
  FAR void *mem = ::operator new(sizeof(T), std::nothrow);

  return mem ? new (mem) T(std::forward<Args>(args)...) : nullptr;
}

static void usage(void)
{
  printf("\nUtility to use tflite micro on nuttx.\n"
    "[ -C       ] Compile tflite model into c++ codes.\n"
    "[ -E       ] Run inference and print the outputs.\n"
    "[ -I       ] Print model information and arena usage.\n"
    "[ -i <str> ] Readable model file path.\n"
    "[ -o <str> ] Writable c++ file path (required with -C).\n"
    "[ -p <str> ] Prefix of compiled code.\n"
    "[ -a <int> ] Arena size (mempool).\n"
    "[ -d <str> ] Comma separated input values, e.g. 0.5,1,2 (implies\n"
    "             -E). One value fills the whole tensor. Quantized\n"
    "             inputs are quantized automatically.\n"
    "[ -x <str> ] Raw input tensor file (implies -E).\n"
    "             -d/-x may be repeated for models with several inputs.\n"
    "[ -n <int> ] Number of inferences to run for timing (implies -E).\n"
    "[ -h       ] Print this message.\n");
}

static void print_float(float value)
{
  uint32_t ipart;
  uint32_t fpart;

  if (value != value)
    {
      printf("nan");
      return;
    }

  if (value < 0.0f)
    {
      putchar('-');
      value = -value;
    }

  if (value >= 4294967295.0f)
    {
      printf("inf");
      return;
    }

  ipart = (uint32_t)value;
  fpart = (uint32_t)((value - ipart) * 1000000.0f + 0.5f);
  if (fpart >= 1000000)
    {
      ipart++;
      fpart -= 1000000;
    }

  printf("%" PRIu32 ".%06" PRIu32, ipart, fpart);
}

static int32_t quantize_value(float value, FAR const TfLiteTensor *tensor,
                              int32_t min, int32_t max)
{
  float scaled = value;

  if (tensor->params.scale != 0.0f)
    {
      scaled = value / tensor->params.scale + tensor->params.zero_point;
    }

  scaled = scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f;
  if (scaled < (float)min)
    {
      return min;
    }

  if (scaled > (float)max)
    {
      return max;
    }

  return (int32_t)scaled;
}

static int set_element(FAR TfLiteTensor *tensor, size_t index, float value)
{
  switch (tensor->type)
    {
      case kTfLiteFloat32:
        tensor->data.f[index] = value;
        break;
      case kTfLiteInt8:
        tensor->data.int8[index] = quantize_value(value, tensor,
                                                  INT8_MIN, INT8_MAX);
        break;
      case kTfLiteUInt8:
        tensor->data.uint8[index] = quantize_value(value, tensor,
                                                   0, UINT8_MAX);
        break;
      case kTfLiteInt16:
        tensor->data.i16[index] = quantize_value(value, tensor,
                                                 INT16_MIN, INT16_MAX);
        break;
      case kTfLiteInt32:
        tensor->data.i32[index] = quantize_value(value, tensor,
                                                 INT32_MIN, INT32_MAX);
        break;
      case kTfLiteBool:
        tensor->data.b[index] = value != 0.0f;
        break;
      default:
        return -ENOTSUP;
    }

  return 0;
}

static int get_element(FAR const TfLiteTensor *tensor, size_t index,
                       FAR float *value)
{
  int32_t raw;

  switch (tensor->type)
    {
      case kTfLiteFloat32:
        *value = tensor->data.f[index];
        return 0;
      case kTfLiteInt8:
        raw = tensor->data.int8[index];
        break;
      case kTfLiteUInt8:
        raw = tensor->data.uint8[index];
        break;
      case kTfLiteInt16:
        raw = tensor->data.i16[index];
        break;
      case kTfLiteInt32:
        raw = tensor->data.i32[index];
        break;
      case kTfLiteBool:
        raw = tensor->data.b[index];
        break;
      default:
        return -ENOTSUP;
    }

  if (tensor->params.scale != 0.0f)
    {
      *value = (raw - tensor->params.zero_point) * tensor->params.scale;
    }
  else
    {
      *value = (float)raw;
    }

  return 0;
}

static size_t element_count(FAR const TfLiteTensor *tensor)
{
  return tflite::ElementCount(*tensor->dims);
}

static FAR const char *tensor_name(FAR const tflite::Model *model,
                                   int32_t index)
{
  FAR const tflite::SubGraph *subgraph = model->subgraphs()->Get(0);
  FAR const tflite::Tensor *tensor;

  if (subgraph->tensors() == nullptr || index < 0 ||
      (uint32_t)index >= subgraph->tensors()->size())
    {
      return "";
    }

  tensor = subgraph->tensors()->Get(index);
  return tensor->name() != nullptr ? tensor->name()->c_str() : "";
}

static void print_tensor_info(FAR const char *label, size_t index,
                              FAR const char *name,
                              FAR const TfLiteTensor *tensor)
{
  int i;

  printf("%s[%zu] \"%s\" %s [", label, index, name,
         TfLiteTypeGetName(tensor->type));
  for (i = 0; i < tensor->dims->size; i++)
    {
      printf(i ? ",%d" : "%d", tensor->dims->data[i]);
    }

  printf("] %zu bytes", tensor->bytes);
  if (tensor->params.scale != 0.0f)
    {
      printf(" scale=");
      print_float(tensor->params.scale);
      printf(" zero_point=%" PRId32, tensor->params.zero_point);
    }

  printf("\n");
}

static void print_output(size_t index, FAR const char *name,
                         FAR const TfLiteTensor *tensor)
{
  size_t count = element_count(tensor);
  size_t best = 0;
  float best_value = 0.0f;
  float value;
  size_t i;

  print_tensor_info("output", index, name, tensor);
  for (i = 0; i < count; i++)
    {
      if (get_element(tensor, i, &value) < 0)
        {
          printf("  (type %s cannot be printed)\n",
                 TfLiteTypeGetName(tensor->type));
          return;
        }

      if (i == 0 || value > best_value)
        {
          best = i;
          best_value = value;
        }

      if (i < TFLM_MAX_PRINT)
        {
          printf("  [%zu] ", i);
          print_float(value);
          printf("\n");
        }
    }

  if (count > TFLM_MAX_PRINT)
    {
      printf("  ... %zu more\n", count - TFLM_MAX_PRINT);
    }

  if (count > 1)
    {
      printf("  argmax: %zu (", best);
      print_float(best_value);
      printf(")\n");
    }
}

static TfLiteStatus add_op(tflm_resolver_t &resolver,
                           tflite::BuiltinOperator op)
{
  switch (op)
    {
      TFLM_OP(ABS, Abs)
      TFLM_OP(ADD, Add)
      TFLM_OP(ADD_N, AddN)
      TFLM_OP(ARG_MAX, ArgMax)
      TFLM_OP(ARG_MIN, ArgMin)
      TFLM_OP(AVERAGE_POOL_2D, AveragePool2D)
      TFLM_OP(BATCH_MATMUL, BatchMatMul)
      TFLM_OP(BATCH_TO_SPACE_ND, BatchToSpaceNd)
      TFLM_OP(BROADCAST_ARGS, BroadcastArgs)
      TFLM_OP(BROADCAST_TO, BroadcastTo)
      TFLM_OP(CAST, Cast)
      TFLM_OP(CEIL, Ceil)
      TFLM_OP(CONCATENATION, Concatenation)
      TFLM_OP(CONV_2D, Conv2D)
      TFLM_OP(COS, Cos)
      TFLM_OP(CUMSUM, CumSum)
      TFLM_OP(DEPTH_TO_SPACE, DepthToSpace)
      TFLM_OP(DEPTHWISE_CONV_2D, DepthwiseConv2D)
      TFLM_OP(DEQUANTIZE, Dequantize)
      TFLM_OP(DIV, Div)
      TFLM_OP(ELU, Elu)
      TFLM_OP(EMBEDDING_LOOKUP, EmbeddingLookup)
      TFLM_OP(EQUAL, Equal)
      TFLM_OP(EXP, Exp)
      TFLM_OP(EXPAND_DIMS, ExpandDims)
      TFLM_OP(FILL, Fill)
      TFLM_OP(FLOOR, Floor)
      TFLM_OP(FLOOR_DIV, FloorDiv)
      TFLM_OP(FLOOR_MOD, FloorMod)
      TFLM_OP(FULLY_CONNECTED, FullyConnected)
      TFLM_OP(GATHER, Gather)
      TFLM_OP(GATHER_ND, GatherNd)
      TFLM_OP(GREATER, Greater)
      TFLM_OP(GREATER_EQUAL, GreaterEqual)
      TFLM_OP(HARD_SWISH, HardSwish)
      TFLM_OP(L2_NORMALIZATION, L2Normalization)
      TFLM_OP(L2_POOL_2D, L2Pool2D)
      TFLM_OP(LEAKY_RELU, LeakyRelu)
      TFLM_OP(LESS, Less)
      TFLM_OP(LESS_EQUAL, LessEqual)
      TFLM_OP(LOG, Log)
      TFLM_OP(LOGICAL_AND, LogicalAnd)
      TFLM_OP(LOGICAL_NOT, LogicalNot)
      TFLM_OP(LOGICAL_OR, LogicalOr)
      TFLM_OP(LOGISTIC, Logistic)
      TFLM_OP(LOG_SOFTMAX, LogSoftmax)
      TFLM_OP(MAXIMUM, Maximum)
      TFLM_OP(MAX_POOL_2D, MaxPool2D)
      TFLM_OP(MEAN, Mean)
      TFLM_OP(MINIMUM, Minimum)
      TFLM_OP(MIRROR_PAD, MirrorPad)
      TFLM_OP(MUL, Mul)
      TFLM_OP(NEG, Neg)
      TFLM_OP(NOT_EQUAL, NotEqual)
      TFLM_OP(PACK, Pack)
      TFLM_OP(PAD, Pad)
      TFLM_OP(PADV2, PadV2)
      TFLM_OP(PRELU, Prelu)
      TFLM_OP(QUANTIZE, Quantize)
      TFLM_OP(REDUCE_MAX, ReduceMax)
      TFLM_OP(RELU, Relu)
      TFLM_OP(RELU6, Relu6)
      TFLM_OP(RESHAPE, Reshape)
      TFLM_OP(RESIZE_BILINEAR, ResizeBilinear)
      TFLM_OP(RESIZE_NEAREST_NEIGHBOR, ResizeNearestNeighbor)
      TFLM_OP(ROUND, Round)
      TFLM_OP(RSQRT, Rsqrt)
      TFLM_OP(SELECT_V2, SelectV2)
      TFLM_OP(SHAPE, Shape)
      TFLM_OP(SIN, Sin)
      TFLM_OP(SLICE, Slice)
      TFLM_OP(SOFTMAX, Softmax)
      TFLM_OP(SPACE_TO_BATCH_ND, SpaceToBatchNd)
      TFLM_OP(SPACE_TO_DEPTH, SpaceToDepth)
      TFLM_OP(SPLIT, Split)
      TFLM_OP(SPLIT_V, SplitV)
      TFLM_OP(SQRT, Sqrt)
      TFLM_OP(SQUARE, Square)
      TFLM_OP(SQUARED_DIFFERENCE, SquaredDifference)
      TFLM_OP(SQUEEZE, Squeeze)
      TFLM_OP(STRIDED_SLICE, StridedSlice)
      TFLM_OP(SUB, Sub)
      TFLM_OP(SUM, Sum)
      TFLM_OP(SVDF, Svdf)
      TFLM_OP(TANH, Tanh)
      TFLM_OP(TRANSPOSE, Transpose)
      TFLM_OP(TRANSPOSE_CONV, TransposeConv)
      TFLM_OP(UNIDIRECTIONAL_SEQUENCE_LSTM, UnidirectionalSequenceLSTM)
      TFLM_OP(UNPACK, Unpack)
      TFLM_OP(ZEROS_LIKE, ZerosLike)
      default:
        printf("Unsupported operator: %s (%d)\n",
               tflite::EnumNameBuiltinOperator(op), op);
        return kTfLiteError;
    }
}

static int register_ops(FAR const tflite::Model *model,
                        tflm_resolver_t &resolver)
{
  FAR const auto *codes = model->operator_codes();
  tflite::BuiltinOperator op;
  uint32_t i;

  if (codes == nullptr)
    {
      return 0;
    }

  for (i = 0; i < codes->size(); i++)
    {
      op = tflite::GetBuiltinCode(codes->Get(i));
      if (op == tflite::BuiltinOperator_CUSTOM)
        {
          printf("Unsupported custom operator: %s\n",
                 codes->Get(i)->custom_code() ?
                 codes->Get(i)->custom_code()->c_str() : "");
          return -ENOTSUP;
        }

      if (resolver.FindOp(op) != nullptr)
        {
          continue;
        }

      if (add_op(resolver, op) != kTfLiteOk)
        {
          return -ENOTSUP;
        }
    }

  return 0;
}

static void print_model_info(FAR const tflite::Model *model,
                             size_t model_size)
{
  FAR const auto *codes = model->operator_codes();
  FAR const tflite::SubGraph *subgraph = model->subgraphs()->Get(0);
  FAR const auto *operators = subgraph->operators();
  tflite::BuiltinOperator op;
  uint32_t count;
  uint32_t i;
  uint32_t j;

  printf("model: %zu bytes, schema v%" PRIu32 ", %" PRIu32 " subgraph(s)\n",
         model_size, model->version(), model->subgraphs()->size());
  if (model->description() != nullptr)
    {
      printf("description: %s\n", model->description()->c_str());
    }

  if (operators == nullptr || codes == nullptr)
    {
      return;
    }

  printf("operators: %" PRIu32 "\n", operators->size());
  for (i = 0; i < codes->size(); i++)
    {
      op = tflite::GetBuiltinCode(codes->Get(i));
      count = 0;
      for (j = 0; j < operators->size(); j++)
        {
          if (operators->Get(j)->opcode_index() == i)
            {
              count++;
            }
        }

      if (op == tflite::BuiltinOperator_CUSTOM &&
          codes->Get(i)->custom_code() != nullptr)
        {
          printf("  CUSTOM(%s) x%" PRIu32 "\n",
                 codes->Get(i)->custom_code()->c_str(), count);
        }
      else
        {
          printf("  %s x%" PRIu32 "\n",
                 tflite::EnumNameBuiltinOperator(op), count);
        }
    }
}

static int read_file(FAR const char *path, std::unique_ptr<uint8_t[]> &buf,
                     FAR size_t *size)
{
  std::ifstream ifs(path, std::ios::binary);
  std::streamoff len;

  if (!ifs)
    {
      printf("Failed to open file: %s\n", path);
      return -ENOENT;
    }

  ifs.seekg(0, std::ios::end);
  len = ifs.tellg();
  if (len <= 0)
    {
      printf("Empty or unreadable file: %s\n", path);
      return -EINVAL;
    }

  buf.reset(new (std::nothrow) uint8_t[len]);
  if (!buf)
    {
      printf("Failed to allocate %lld bytes for %s\n", (long long)len, path);
      return -ENOMEM;
    }

  ifs.seekg(0, std::ios::beg);
  ifs.read(reinterpret_cast<char *>(buf.get()), len);
  if (!ifs)
    {
      printf("Failed to read file: %s\n", path);
      return -EIO;
    }

  *size = len;
  return 0;
}

static int load_input_values(FAR TfLiteTensor *tensor,
                             FAR const char *values)
{
  size_t count = element_count(tensor);
  FAR const char *ptr = values;
  FAR char *end;
  float first = 0.0f;
  float value;
  size_t n = 0;
  size_t i;

  while (*ptr != '\0')
    {
      value = strtof(ptr, &end);
      if (end == ptr || (*end != ',' && *end != '\0'))
        {
          printf("Invalid input value: %s\n", ptr);
          return -EINVAL;
        }

      if (n >= count)
        {
          printf("Too many input values, tensor has %zu elements\n", count);
          return -E2BIG;
        }

      if (set_element(tensor, n, value) < 0)
        {
          printf("Unsupported input type: %s\n",
                 TfLiteTypeGetName(tensor->type));
          return -ENOTSUP;
        }

      if (n++ == 0)
        {
          first = value;
        }

      ptr = *end == ',' ? end + 1 : end;
    }

  if (n == 1)
    {
      for (i = 1; i < count; i++)
        {
          set_element(tensor, i, first);
        }
    }
  else if (n != count)
    {
      printf("Expected 1 or %zu input values, got %zu\n", count, n);
      return -EINVAL;
    }

  return 0;
}

static int load_input_file(FAR TfLiteTensor *tensor, FAR const char *path)
{
  std::unique_ptr<uint8_t[]> buf;
  size_t size;
  int ret;

  ret = read_file(path, buf, &size);
  if (ret < 0)
    {
      return ret;
    }

  if (size != tensor->bytes)
    {
      printf("Input file %s is %zu bytes, tensor needs %zu bytes\n",
             path, size, tensor->bytes);
      return -EINVAL;
    }

  memcpy(tensor->data.raw, buf.get(), size);
  return 0;
}

static int prepare_inputs(tflite::MicroInterpreter &interpreter,
                          FAR const struct tflm_input_s *inputs,
                          size_t ninputs)
{
  FAR TfLiteTensor *tensor;
  size_t count;
  size_t i;
  size_t j;
  int ret;

  if (ninputs > interpreter.inputs_size())
    {
      printf("Model has %zu input(s), %zu given\n",
             interpreter.inputs_size(), ninputs);
      return -EINVAL;
    }

  for (i = 0; i < interpreter.inputs_size(); i++)
    {
      tensor = interpreter.input(i);
      if (i < ninputs)
        {
          ret = inputs[i].is_file ? load_input_file(tensor, inputs[i].arg) :
                                    load_input_values(tensor, inputs[i].arg);
          if (ret < 0)
            {
              return ret;
            }

          continue;
        }

      /* Inputs without data are set to zero instead of arena leftovers */

      count = element_count(tensor);
      for (j = 0; j < count; j++)
        {
          if (set_element(tensor, j, 0.0f) < 0)
            {
              memset(tensor->data.raw, 0, tensor->bytes);
              break;
            }
        }
    }

  return 0;
}

static void copy_inputs(tflite::MicroInterpreter &interpreter,
                        FAR uint8_t *buf, bool save)
{
  FAR TfLiteTensor *tensor;
  size_t i;

  for (i = 0; i < interpreter.inputs_size(); i++)
    {
      tensor = interpreter.input(i);
      if (save)
        {
          memcpy(buf, tensor->data.raw, tensor->bytes);
        }
      else
        {
          memcpy(tensor->data.raw, buf, tensor->bytes);
        }

      buf += tensor->bytes;
    }
}

static int64_t elapsed_us(FAR const struct timespec *start,
                          FAR const struct timespec *end)
{
  return (int64_t)(end->tv_sec - start->tv_sec) * 1000000 +
         (end->tv_nsec - start->tv_nsec) / 1000;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  struct tflm_input_s inputs[TFLM_MAX_INPUTS];
  FAR const char *model_path = nullptr;
  FAR const char *code_path = nullptr;
  FAR const char *prefix = "NXAI";
  FAR const tflite::Model *model;
  std::unique_ptr<uint8_t[]> model_buf;
  std::unique_ptr<uint8_t[]> arena;
  std::unique_ptr<uint8_t[]> input_copy;
  tflm_ptr<tflm_resolver_t> resolver;
  tflm_ptr<tflite::MicroProfiler> profiler;
  tflm_ptr<tflite::MicroInterpreter> interpreter;
  struct timespec start;
  struct timespec end;
  bool need_compile = false;
  bool need_invoke = false;
  bool need_info = false;
  size_t ninputs = 0;
  size_t input_bytes;
  size_t model_size;
  size_t i;
  long arena_size = 1024 * 8;
  long runs = 1;
  long run;
  int64_t total_us;
  int ch;

  while ((ch = getopt(argc, argv, "CEIhi:o:p:a:d:x:n:")) != EOF)
    {
      switch (ch)
        {
          case 'C':
            need_compile = true;
            break;
          case 'E':
            need_invoke = true;
            break;
          case 'I':
            need_info = true;
            break;
          case 'p':
            prefix = optarg;
            break;
          case 'i':
            model_path = optarg;
            break;
          case 'o':
            code_path = optarg;
            break;
          case 'a':
            arena_size = strtol(optarg, NULL, 0);
            break;
          case 'd':
          case 'x':
            if (ninputs >= TFLM_MAX_INPUTS)
              {
                printf("At most %d inputs can be given\n", TFLM_MAX_INPUTS);
                return -1;
              }

            inputs[ninputs].arg = optarg;
            inputs[ninputs].is_file = ch == 'x';
            ninputs++;
            need_invoke = true;
            break;
          case 'n':
            runs = strtol(optarg, NULL, 0);
            need_invoke = true;
            break;
          case 'h':
          default:
            usage();
            return -1;
        }
    }

  if (!model_path || (need_compile && !code_path) || arena_size <= 0 ||
      runs <= 0)
    {
      usage();
      return -1;
    }

  if (read_file(model_path, model_buf, &model_size) < 0)
    {
      return -1;
    }

  flatbuffers::Verifier verifier(model_buf.get(), model_size);
  if (!tflite::VerifyModelBuffer(verifier))
    {
      printf("Not a valid tflite model: %s\n", model_path);
      return -1;
    }

  model = tflite::GetModel(model_buf.get());
  if (model->version() != TFLITE_SCHEMA_VERSION)
    {
      printf("Model schema v%" PRIu32 " is not supported (expected v%d)\n",
             model->version(), TFLITE_SCHEMA_VERSION);
      return -1;
    }

  if (model->subgraphs() == nullptr || model->subgraphs()->size() == 0)
    {
      printf("Model has no subgraph\n");
      return -1;
    }

  if (need_info)
    {
      print_model_info(model, model_size);
    }

  /* These objects are too large for the task stack (the profiler alone
   * records up to 4096 events), so keep them on the heap.
   */

  resolver.reset(tflm_new<tflm_resolver_t>());
  profiler.reset(tflm_new<tflite::MicroProfiler>());
  arena.reset(new (std::nothrow) uint8_t[arena_size]);
  if (!resolver || !profiler || !arena)
    {
      printf("Failed to allocate memory (arena %ld bytes)\n", arena_size);
      return -1;
    }

  if (register_ops(model, *resolver) < 0)
    {
      return -1;
    }

  interpreter.reset(tflm_new<tflite::MicroInterpreter>(model, *resolver,
    arena.get(), (size_t)arena_size, nullptr, profiler.get()));
  if (!interpreter)
    {
      printf("Failed to allocate interpreter\n");
      return -1;
    }

  TfLiteStatus status = interpreter->AllocateTensors();
  if (status != kTfLiteOk)
    {
      printf("AllocateTensors failed: %d (arena %ld bytes, try a larger "
             "-a)\n", status, arena_size);
      return -1;
    }

  if (need_info)
    {
      for (i = 0; i < interpreter->inputs_size(); i++)
        {
          print_tensor_info("input", i,
                            tensor_name(model,
                              model->subgraphs()->Get(0)->inputs()->Get(i)),
                            interpreter->input(i));
        }

      for (i = 0; i < interpreter->outputs_size(); i++)
        {
          print_tensor_info("output", i,
                            tensor_name(model,
                              model->subgraphs()->Get(0)->outputs()->Get(i)),
                            interpreter->output(i));
        }

      printf("arena: %zu of %ld bytes used\n",
             interpreter->arena_used_bytes(), arena_size);
    }

  if (need_invoke)
    {
      if (prepare_inputs(*interpreter, inputs, ninputs) < 0)
        {
          return -1;
        }

      /* The memory planner may reuse input buffers for intermediate
       * tensors, so restore the inputs before every run.
       */

      input_bytes = 0;
      for (i = 0; i < interpreter->inputs_size(); i++)
        {
          input_bytes += interpreter->input(i)->bytes;
        }

      input_copy.reset(new (std::nothrow) uint8_t[input_bytes]);
      if (!input_copy)
        {
          printf("Failed to allocate %zu bytes for inputs\n", input_bytes);
          return -1;
        }

      copy_inputs(*interpreter, input_copy.get(), true);

      total_us = 0;
      for (run = 0; run < runs; run++)
        {
          copy_inputs(*interpreter, input_copy.get(), false);
          profiler->ClearEvents();

          clock_gettime(CLOCK_MONOTONIC, &start);
          status = interpreter->Invoke();
          clock_gettime(CLOCK_MONOTONIC, &end);
          if (status != kTfLiteOk)
            {
              printf("Invoke failed: %d\n", status);
              return -1;
            }

          total_us += elapsed_us(&start, &end);
        }

      profiler->LogCsv();
      profiler->LogTicksPerTagCsv();

      for (i = 0; i < interpreter->outputs_size(); i++)
        {
          print_output(i, tensor_name(model,
                            model->subgraphs()->Get(0)->outputs()->Get(i)),
                       interpreter->output(i));
        }

      printf("inference: %ld run(s), total %lld us, average %lld us\n",
             runs, (long long)total_us, (long long)(total_us / runs));
    }

  if (need_compile)
    {
#ifdef TFLITE_MODEL_COMPILER
      std::ofstream ofs(code_path);
      interpreter->Compile(ofs, prefix);
      ofs.close();
#else
      printf("Not supported compiling %s.\n", prefix);
#endif
    }

  printf("nxai done!\n");
  return 0;
}
