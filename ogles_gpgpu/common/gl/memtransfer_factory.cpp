//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

#include "memtransfer_factory.h"
#include "../core.h"

// clang-off
#if defined(OGLES_GPGPU_IOS) && !defined(OGLES_GPGPU_OPENGL_ES3)
#  include "../../platform/ios/memtransfer_ios.h"
#elif defined(OGLES_GPGPU_ANDROID) && !defined(OGLES_GPGPU_OPENGL_ES3)
#  include "../../platform/android/memtransfer_android.h"
#else
#  include "../../platform/opengl/memtransfer_generic.h"
#endif
// clang-on

using namespace ogles_gpgpu;

bool MemTransferFactory::usePlatformOptimizations = false;

MemTransfer* MemTransferFactory::createInstance() {
    MemTransfer* instance = NULL;

    if (usePlatformOptimizations) { // create specialized instance
#if defined(OGLES_GPGPU_IOS) && !defined(OGLES_GPGPU_OPENGL_ES3)
        instance = (MemTransfer*)new MemTransferIOS();
#elif defined(OGLES_GPGPU_ANDROID) && !defined(OGLES_GPGPU_OPENGL_ES3)
        instance = (MemTransfer*)new MemTransferAndroid();
#else
        instance = (MemTransfer*)new MemTransfer();
#endif
    }

    if (!instance) { // create default instance
        instance = new MemTransfer();
    }

    return instance;
}

bool MemTransferFactory::tryEnablePlatformOptimizations() {
#if defined(OGLES_GPGPU_IOS) && !defined(OGLES_GPGPU_OPENGL_ES3)
    usePlatformOptimizations = MemTransferIOS::initPlatformOptimizations();
#elif defined(OGLES_GPGPU_ANDROID) && !defined(OGLES_GPGPU_OPENGL_ES3)
    usePlatformOptimizations = MemTransferAndroid::initPlatformOptimizations();
#else
    usePlatformOptimizations = false;
#endif

    return usePlatformOptimizations;
}
