
APP=opencl_demo
HALIDE=~/git/OAL/Halide
HALIDE_SO=${HALIDE}/build/src
BIN_DIR=./bin/x86
mkdir -p ${BIN_DIR}
export LD_LIBRARY_PATH=${HALIDE_SO}

g++ ${APP}_gen.cpp ${HALIDE}/tools/GenGen.cpp -g -fno-rtti -std=c++17 -I ${HALIDE}/build/include -I ${HALIDE}/src -L ${HALIDE_SO} -lHalide -lpthread -ldl -o ${APP}.gen

# gen func lib
./${APP}.gen \
-g ${APP} \
-e c_header,c_source,static_library \
-o ${BIN_DIR} \
target=x86-64-linux-opencl-no_runtime-no_asserts

# runtime
./${APP}.gen \
-r ${APP}.runtime \
-o ${BIN_DIR} \
-e static_library \
target=x86-64-linux-opencl

# runner
g++ ${APP}_run.cpp \
-o ${APP}.run \
-I ${HALIDE_BUILD_DIR}/include \
-I ${BIN_DIR} \
./${BIN_DIR}/${APP}.runtime.a \
./${BIN_DIR}/${APP}.a \
-pthread -std=c++17  -ldl \

# GXX=aarch64-linux-gnu-g++-8