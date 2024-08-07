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
| array                | 32      | 0       | 0       | 32     |
| condition_variable   | 584     | 8       | 16      | 608    |
| deque                | 41196   | 300     | 40      | 41544  |
| exception            | 32      | 0       | 32      | 32     |
| forward_list         | 41164   | 308     | 40      | 41512  |
| future               | 43504   | 356     | 112     | 43972  |
| iostream             | 148004  | 404     | 3160    | 151568 |
| list                 | 41516   | 308     | 40      | 41864  |
| map                  | 45252   | 308     | 40      | 45600  |
| multiset             | 44676   | 308     | 40      | 45024  |
| mutex                | 552     | 8       | 16      | 576    |
| rtti                 | 41420   | 308     | 40      | 41768  |
| semaphore            | 1352    | 8       | 16488   | 17808  |
| set                  | 44644   | 308     | 40      | 44992  |
| shared_ptr           | 43204   | 308     | 40      | 41712  |
| string               | 42044   | 308     | 40      | 42392  |
| string_view          | 448     | 0       | 0       | 448    |
| thread               | 41956   | 356     | 12      | 42424  |
| unordered_map        | 47668   | 308     | 40      | 48016  |
| unordered_multimap   | 46868   | 308     | 40      | 47216  |
| unordered_multiset   | 46836   | 308     | 40      | 47184  |
| unordered_set        | 46452   | 308     | 40      | 46800  |
| vector               | 41444   | 308     | 40      | 41792  |
| weak_ptr             | 43460   | 308     | 40      | 41936  |

## Conclusion

By analyzing the code size of each component in the C++ Standard Library, developers can better understand the memory implications of their usage of these classes. The details provided in this README serve as a reference point for optimizing application performance and making educated design choices. We hope you find this information valuable as you work with the C++ Standard Library.
