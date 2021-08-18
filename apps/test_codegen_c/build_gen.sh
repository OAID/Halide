# APP: add/gemm
APP=$1
HALIDE=~/git/OAL/Halide
HALIDE_SO=${HALIDE}/build/src
g++ ${APP}_gen.cpp ${HALIDE}/tools/GenGen.cpp -g -fno-rtti -std=c++17 -I ${HALIDE}/build/include -I ${HALIDE}/src -L ${HALIDE_SO} -lHalide -lpthread -ldl -o ${APP}.gen
export LD_LIBRARY_PATH=${HALIDE_SO}
export HL_DEBUG_CODEGEN=0

mkdir -p ./bin/${APP}/x86
mkdir -p ./bin/${APP}/arm64
# x86
./${APP}.gen -g ${APP} -e c_header,assembly,c_source,static_library,stmt \
  -o ./bin/${APP}/x86 target=host-linux-no_runtime-no_asserts

# arm64
./${APP}.gen -g ${APP} -e c_header,assembly,c_source,static_library,stmt \
  -o ./bin/${APP}/arm64 target=arm-64-linux-no_runtime-no_asserts

