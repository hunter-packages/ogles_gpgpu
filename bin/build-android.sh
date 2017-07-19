#!/bin/bash

TOOLCHAIN=android-ndk-r10e-api-19-armeabi-v7a-neon
CONFIG=Release

OGLES_GPGPU_OPENGL_ES3="OFF"
if [[ "$#" -gt 0 ]] ; then
    OGLES_GPGPU_OPENGL_ES3="ON"
fi

ARGS=(
    OGLES_GPGPU_BUILD_TESTS=ON
    OGLES_GPGPU_OPENGL_ES3="${OGLES_GPGPU_OPENGL_ES3}"
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
)

polly.py --toolchain ${TOOLCHAIN} --verbose --config ${CONFIG} --reconfig --open --test --fwd ${ARGS[@]} --test
