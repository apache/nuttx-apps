#!/usr/bin/env bash

USAGE="$0 <Ficl-dir>"

FICLDIR=$1
if [ -z "${FICLDIR}" ]; then
    echo "Missing command line argument"
    echo $USAGE
    exit 1
fi

if [ ! -d "${FICLDIR}" ]; then
    echo "Sub-directory ${FICLDIR} does not exist"
    echo $USAGE
    exit 1
fi

if [ ! -r "${FICLDIR}/Nuttx.mk" ]; then
    echo "Readable ${FICLDIR}/Nuttx.mk does not exist"
    echo $USAGE
    exit 1
fi

OBJECTS=`grep "^OBJECTS" ${FICLDIR}/Nuttx.mk`
if [ -z "${OBJECTS}" ]; then
    echo "No OBJECTS found in ${FICLDIR}/Nuttx.mk"
    echo $USAGE
    exit 1
fi

OBJLIST=`echo ${OBJECTS} | cut -d'=' -f2 | sed -e "s/unix\.o//g"`

rm -f Nuttx.srcs
echo "# apps/interpreters/ficl/Make.obs" >> Nuttx.srcs
echo "# Auto-generated file.. Do not edit" >> Nuttx.srcs
echo "" >> Nuttx.srcs
echo "FICL_SUBDIR = ${1}" >> Nuttx.srcs
echo "FICL_DEPPATH = --dep-path ${1}" >> Nuttx.srcs

unset CSRCS
for OBJ in ${OBJLIST}; do
    SRC=`echo ${OBJ} | sed -e "s/\.o/\.c/g"`
    CSRCS=${CSRCS}" ${SRC}"
done
echo "FICL_ASRCS = " >> Nuttx.srcs
echo "FICL_CXXSRCS = " >> Nuttx.srcs
echo "FICL_CSRCS = ${CSRCS}" >> Nuttx.srcs
