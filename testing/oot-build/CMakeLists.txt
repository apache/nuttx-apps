cmake_minimum_required(VERSION 3.12...3.31)

project(OOTCpp
    VERSION 1.0
    DESCRIPTION "Out Of Tree Build C++ NuttX"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use CMAKE_SOURCE_DIR for source files
set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/HelloWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

set(EXE_NAME oot)

add_executable(${EXE_NAME} ${SOURCE_FILES})

# Add include directory so you can #include "..." from include/
target_include_directories(${EXE_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Generate a .bin file from the ELF after build
add_custom_command(
    TARGET ${EXE_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -S -O binary
            ${CMAKE_BINARY_DIR}/${EXE_NAME}
            ${CMAKE_BINARY_DIR}/${EXE_NAME}.bin
    COMMENT "Generating binary image ${EXE_NAME}.bin"
)
