#!/bin/bash

TOOLCHAIN=xcode
CONFIG=Release

OGLES_GPGPU_OPENGL_ES3="OFF"
if [[ "$#" -gt 0 ]] ; then
    OGLES_GPGPU_OPENGL_ES3="ON"
fi

ARGS=(
    OGLES_GPGPU_BUILD_TESTS=ON
    OGLES_GPGPU_OPENGL_ES3=${OGLES_GPGPU_OPENGL_ES3}
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
)
#polly.py --toolchain ${TOOLCHAIN} --verbose --config ${CONFIG} --reconfig --test --fwd ${ARGS[@]} --test
polly.py --toolchain ${TOOLCHAIN} --verbose --config ${CONFIG} --reconfig --test --fwd ${ARGS[@]} --nobuild --open
