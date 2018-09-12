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
#if defined(OGLES_GPGPU_IOS)
#  include "../../platform/ios/memtransfer_ios.h"
#else
#  include "../../platform/opengl/memtransfer_generic.h"
#endif
// clang-on

using namespace ogles_gpgpu;

bool MemTransferFactory::usePlatformOptimizations = false;

std::unique_ptr<MemTransfer> MemTransferFactory::createInstance() {
    std::unique_ptr<MemTransfer> instance;

    if (usePlatformOptimizations) { // create specialized instance
#if defined(OGLES_GPGPU_IOS)
        instance = std::unique_ptr<MemTransfer>(new MemTransferIOS);
#else
        instance = std::unique_ptr<MemTransfer>(new MemTransfer);
#endif
    }

    if (!instance) { // create default instance
        instance = std::unique_ptr<MemTransfer>(new MemTransfer);
    }

    return instance;
}

bool MemTransferFactory::tryEnablePlatformOptimizations() {
#if defined(OGLES_GPGPU_IOS)
    usePlatformOptimizations = MemTransferIOS::initPlatformOptimizations();
#else
    usePlatformOptimizations = false;
#endif

    return usePlatformOptimizations;
}
