#!/bin/bash

TOOLCHAIN=libcxx
CONFIG=Release

ARGS=(
    OGLES_GPGPU_BUILD_TESTS=ON
    OGLES_GPGPU_OPENGL_ES3=ON # not really, but enable pbo etc
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
)
polly.py --toolchain ${TOOLCHAIN} --verbose --config ${CONFIG} --reconfig --test --fwd ${ARGS[@]} --test

