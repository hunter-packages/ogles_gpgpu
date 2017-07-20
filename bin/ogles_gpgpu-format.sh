#!/bin/bash

# Note: When using clang-format the ${OGLES_GPGPU_ROOT}/.clang-format file
# seems to be ignored if the root directory to this script is not
# specified relative to the working directory:
#
# Suggested usage:
#
# cd ${OGLES_GPGPU_ROOT}
# ./bin/ogles_gpgpu-format.sh lib
# ./bin/ogles_gpgpu-format.sh test

# Find all internal files, making sure to exlude 3rdparty subprojects
function find_ogles_gpgpu_source()
{
    find $1 -name "*.h" -or -name "*.cpp" -or -name "*.hpp" -or -name "*.m" -or -name "*.mm"
}

input_dir=$1

echo ${input_dir}

find_ogles_gpgpu_source ${input_dir} | while read name
do
    echo $name
    clang-format -i -style=file ${name}
done


