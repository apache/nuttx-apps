# C++ Standard Library Code Size Testing Program

## Introduction

This project provides a testing program designed to measure the code size of each class in the C++ Standard Library. Understanding the code size of various classes can help developers optimize their usage of the standard library and make informed design decisions in their applications. This program aims to provide clear insights into how much memory each component of the standard library consumes, categorized into text, data, BSS, and overall size (DEC).

## Purpose

The primary goals of this project are to:
- **Analyze the memory consumption** of standard library components.
- **Identify potential areas for optimization** in applications that utilize these components.
- **Aid developers** in making better decisions regarding the use of the C++ Standard Library depending on their application’s constraints.

## Testing Platform
This program has been tested on the x4b/release branch.

## C++ Compilation Configurations

- `CONFIG_HAVE_CXX`
- `CONFIG_HAVE_CXXINITIALIZE`
- `CONFIG_LIBCXX`
- `CONFIG_LIBSUPCXX`

## Optimization Configurations

- `CONFIG_DEBUG_OPTLEVEL="-Os"`
- `CONFIG_LTO_FULL`
- `-ffunction-section`
- `-fdata-section`

## Component-Specific Configurations

- **`iostream`**: enable `CONFIG_LIBC_LOCALE`.
- **`exception`**: enable `CONFIG_CXX_EXCEPTION`.
- **`rtti`**: enable `CONFIG_CXX_RTTI`.

## Compilation Instructions

To compile the code size testing program, use the following command:

```bash
./build.sh qemu-armv7a:nsh -j
```

## Code Size Breakdown

The following table summarizes the code size measurements for different components in the C++ Standard Library. The sizes are measured in bytes and broken down into the following categories:

- **Text**: Size of the code segment containing executable instructions.
- **Data**: Size of initialized global and static variables.
- **BSS**: Size of uninitialized global and static variables.
- **DEC**: Total size in bytes (sum of Text, Data, and BSS).

### Output Examples

#### Without C++ Config

```
   text    data     bss     dec     hex filename
 204388     300    9728  214416   34590 nuttx/nuttx
```

#### Only Basic C++ Configurations

```
   text    data     bss     dec     hex filename
 208100     300    9856  218256   35490 nuttx/nuttx
```

#### Compiling an Empty main Function with Basic C++ Configurations

```
   text    data     bss     dec     hex filename
 208132     300    9856  218288   354b0 nuttx/nuttx
```

### Code Size Measurements

| **Component**        | **Text** | **Data** | **BSS** | **DEC** |
| ---------------------|---------|---------|---------|--------|
| basic c++            | 3712    | 0       | 128     | 3840   |
| array                | 16      | 0       | 0       | 16      |
| condition_variable   | 144     | 0       | 0       | 144     |
| deque                | 240     | 0       | 8       | 248     |
| exception            | 16      | 0       | 0       | 16      |
| forward_list         | 144     | 0       | 8       | 152     |
| future               | 3920    | 48      | 72      | 4040    |
| iostream             | 66192   | 120     | 1704    | 68016   |
| list                 | 528     | 0       | 8       | 536     |
| map                  | 4256    | 0       | 8       | 4264    |
| multiset             | 3680    | 0       | 8       | 3688    |
| mutex                | 112     | 0       | 0       | 112     |
| rtti                 | 2208    | 0       | 8       | 2216    |
| semaphore            | 1300    | 12      | 84      | 1392    |
| set                  | 3616    | 0       | 8       | 3624    |
| shared_ptr           | 464     | 0       | 8       | 472     |
| string_view          | 432     | 0       | 0       | 432     |
| string               | 1376    | 0       | 8       | 1384    |
| thread               | 1040    | 48      | 72      | 1160    |
| unordered_map        | 6616    | 0       | 0       | 6616    |
| unordered_multimap   | 5864    | 0       | 0       | 5864    |
| unordered_multiset   | 5816    | 0       | 0       | 5816    |
| unordered_set        | 5432    | 0       | 0       | 5432    |
| vector               | 456     | 0       | 8       | 464     |
| weak_ptr             | 624     | 0       | 8       | 632     |

## Conclusion

By analyzing the code size of each component in the C++ Standard Library, developers can better understand the memory implications of their usage of these classes. The details provided in this README serve as a reference point for optimizing application performance and making educated design choices. We hope you find this information valuable as you work with the C++ Standard Library.
