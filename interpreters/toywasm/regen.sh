#! /bin/sh

# A script to generate a few files for nuttx-apps build.

REPO=https://github.com/yamt/toywasm
REF=${REF:-master}

TMP=$(mktemp -d)
trap "rm -rf ${TMP}" 0
DIR="${TMP}/src"
BUILDDIR="${TMP}/build"

mkdir -p ${DIR}
git -C ${DIR} init
git -C ${DIR} fetch --tags ${REPO} ${REF}
git -C ${DIR} checkout FETCH_HEAD

# Note: for this build, TOYWASM_USE_SHORT_ENUMS is only used for
# the "toywasm --version" output.

# Note: Disable TOYWASM_USE_TAILCALL because it isn't safe for some
# targets and/or compilers. For example, xtensa windowed ABI doesn't
# allow tail call optimization. (At least it isn't straightforward
# unless frame sizes of the caller and the callee happens to match.)
# REVISIT: This should probably be a Kconfig knob. For now, disable
# it globally.

cmake -B ${BUILDDIR} \
-DTOYWASM_USE_SHORT_ENUMS=OFF \
-DTOYWASM_USE_TAILCALL=OFF \
-DTOYWASM_ENABLE_WASM_EXTENDED_CONST=ON \
-DTOYWASM_ENABLE_WASM_MULTI_MEMORY=ON \
-DTOYWASM_ENABLE_WASM_TAILCALL=ON \
-DTOYWASM_ENABLE_WASM_THREADS=ON \
-DTOYWASM_ENABLE_WASI_THREADS=ON \
${DIR}

for fn in \
    include/toywasm_config.h \
    include/toywasm_version.h \
    src/toywasm_config.c; do

    (m4 -DFILENAME=$fn tmpl/license.in
    case ${fn} in
    *.c)
        cat tmpl/c-sections.in
        ;;
    esac
    # remove comments to appease nxstyle
    sed -e '/^\/\*/d' ${BUILDDIR}/$(basename $fn)) > ${fn}
done
