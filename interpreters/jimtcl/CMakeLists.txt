if(CONFIG_INTERPRETERS_JIMTCL)
  set(JIMTCL_VERSION 0.83)
  set(JIMTCL_TARBALL ${JIMTCL_VERSION}.tar.gz)
  set(JIMTCL_UNPACK jimtcl)
  set(JIMTCL_URL_BASE https://github.com/msteveb/jimtcl/archive/refs/tags)
  set(JIMTCL_URL ${JIMTCL_URL_BASE}/${JIMTCL_TARBALL})
  set(JIMTCL_SRC ${CMAKE_CURRENT_BINARY_DIR}/${JIMTCL_UNPACK})

  # We need the source available before declaring the application, because
  # jimsh.c is added as an application source, so we can not integrate download
  # into the external project.
  FetchContent_Declare(jimtcl_fetch URL ${JIMTCL_URL} SOURCE_DIR
                                        ${JIMTCL_UNPACK})

  FetchContent_MakeAvailable(jimtcl_fetch)

  ExternalProject_Add(
    jimtcl_build
    SOURCE_DIR ${JIMTCL_SRC}
    PATCH_COMMAND sed -i -e "/^void Jim_SetEnviron/,/^/{s/^/\\/* /}" -e
                  "/^void Jim_SetEnviron/,/^/{s/\$/ *\\//}" jim.c
    CONFIGURE_COMMAND
      ./configure --host=${CMAKE_C_COMPILER_TARGET}
      "CFLAGS=$<JOIN:$<TARGET_PROPERTY:apps_jimsh,COMPILE_OPTIONS>, > -D$<JOIN:$<TARGET_PROPERTY:apps_jimsh,COMPILE_DEFINITIONS>, -D> -I$<JOIN:$<TARGET_PROPERTY:apps_jimsh,INCLUDE_DIRECTORIES>, -I> -isystem ${CMAKE_SOURCE_DIR}/include -isystem ${CMAKE_BINARY_DIR}/include"
    BUILD_COMMAND make libjim.a initjimsh.o
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_BYPRODUCTS ${JIMTCL_SRC}/libjim.a ${JIMTCL_SRC}/_initjimsh.c)

  nuttx_add_application(
    NAME
    jimsh
    PRIORITY
    100
    STACKSIZE
    8192
    INCLUDE_DIRECTORIES
    ${JIMTCL_SRC}
    DEPENDS
    jimtcl_build
    SRCS
    ${JIMTCL_SRC}/jimsh.c
    ${JIMTCL_SRC}/_initjimsh.c)

  set_property(GLOBAL APPEND PROPERTY NUTTX_EXTRA_LIBRARIES
                                      ${JIMTCL_SRC}/libjim.a)

endif()
