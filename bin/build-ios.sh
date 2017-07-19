#!/bin/bash


TOOLCHAIN=ios-10-1-arm64-dep-8-0-hid-sections
CONFIG=Release

echo "ARGC: ${#}"
OGLES_GPGPU_OPENGL_ES3="OFF"
if [[ "$#" -gt 0 ]] ; then
    OGLES_GPGPU_OPENGL_ES3="ON"
fi

ARGS=(
    OGLES_GPGPU_BUILD_TESTS=ON
    OGLES_GPGPU_OPENGL_ES3=${OGLES_GPGPU_OPENGL_ES3}
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
)

polly.py --toolchain ${TOOLCHAIN} --verbose --config ${CONFIG} --reconfig --open --fwd ${ARGS[@]} --test


