#!/bin/bash

set -euxo pipefail

TNN_LIB_PATH=third_party/tnn_x86_lib/build_linux_native
rm -rf tnn_demo
mkdir tnn_demo
cd tnn_demo

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DTNN_LIB_PATH=$TNN_LIB_PATH \
    -DTNN_DEMO_WITH_WEBCAM=OFF \

make -j4
