#!/bin/bash
############################################################################
# apps/benchmarks/tacle-bench/generate_taclebench_c.sh
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

MAIN_C=tacle-bench/taclebench.c
BENCH_PATH=tacle-bench/bench
GENERATE_PATH=tacle-bench

search_main() {
    local file="$1"
    local filename=$(basename "$file")
    local new_main="main_${filename%.*}"

    flag=0

    # try to modifiy main function declaration first
    if grep -q "int main();" "$file"; then
        flag=1
        sed -i 's/int main();/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main();/' "${file}"
    elif grep -q "int main( void );" "$file"; then
        flag=1
        sed -i 's/int main( void );/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main( void );/' "${file}"
    elif grep -q "int main ( void );" "$file"; then
        flag=1
        sed -i 's/int main ( void );/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main ( void );/' "${file}"
    fi

    # if no main function delaration, try to modify main function
    if [ "$flag" -eq 0 ]; then
        if grep -q "int main()$" "$file"; then
            flag=1
            sed -i 's/int main()$/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main()/' "${file}"
        elif grep -q "int main( void )$" "$file"; then
            flag=1
            sed -i 's/int main( void )$/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main( void )/' "${file}"
        elif grep -q "int main ( void )$" "$file"; then
            flag=1
            sed -i 's/int main ( void )$/#ifdef ALL_IN_ONE\n    #define main '"${new_main}"'\n#endif\nint main ( void )/' "${file}"
        fi
    fi

    if [ "$flag" -eq 1 ]; then
        echo "int $new_main (void);" >> $${MAIN_C}.tmp
        echo "  if ($new_main() != 0) { printf(\"$new_main error\n\"); }" >> $MAIN_C
        echo "" >> $MAIN_C
    else
        echo "Failed to modify main function in $file"
    fi
}

# search for c file
find_c_files() {
    local dir="$1"
    find "$dir" -type f -name "*.c" | while read -r file; do
        search_main "$file"
    done
}

> ${MAIN_C}.tmp
echo "#include <stdio.h>" >> ${MAIN_C}.tmp
echo "int main (void) {" >> $MAIN_C


# exclude bench/parallel
mv $BENCH_PATH/parallel ./parallel

find_c_files "$BENCH_PATH"

echo "  return 0;" >> $MAIN_C
echo "}" >> $MAIN_C

cat $MAIN_C >> ${MAIN_C}.tmp

mv ${MAIN_C}.tmp $MAIN_C
# restore bench/parallel
mv ./parallel $BENCH_PATH/parallel
