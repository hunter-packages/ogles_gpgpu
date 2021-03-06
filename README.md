# ogles_gpgpu - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0

| Linux/OSX/Android/iOS                           | Windows                                             |
|-------------------------------------------------|-----------------------------------------------------|
| [![Build Status][travis_status]][travis_builds] | [![Build Status][appveyor_status]][appveyor_builds] |

[travis_status]: https://travis-ci.org/hunter-packages/ogles_gpgpu.svg?branch=hunter
[travis_builds]: https://travis-ci.org/hunter-packages/ogles_gpgpu

[appveyor_status]: https://ci.appveyor.com/api/projects/status/5t9i7mwbdvhqn21i/branch/hunter?svg=true
[appveyor_builds]: https://ci.appveyor.com/project/ruslo/ogles-gpgpu/branch/hunter

This is an active fork of an original project by Markus Konrad.  
* [Original project](https://github.com/internaut/ogles_gpgpu)
* [Project documentation](http://www.mkonrad.net/projects/ogles_gpgpu.html) for more information

UPDATE: CMake support has been added via the [Hunter](https://github.com/ruslo/hunter) package manager with cross platform toolchains provided by [Polly](https://github.com/ruslo/polly).  See the README pages of these links for details.

The extensions in this fork include:
* OpenGL ES 3.0 support (primarily for asynchronous PBO use)
* Flexible bottom-up filter graph architectures: branch, merge, etc
* Various shader implementations (many adapted from Brad Larson's GPUImage)

## Features

* fast and portable C++ code
* platform optimizations for fast texture access
 * on iOS: [Core Video Texture Cache API](http://allmybrain.com/2011/12/08/rendering-to-a-texture-with-ios-5-texture-cache-api/)
 * on Android: EGL pixelbuffers and [KHRImage extensions](http://snorp.net/2011/12/16/android-direct-texture.html)
* well documented
* contains several example applications (deprecated)
* License: [Apache 2](http://www.apache.org/licenses/LICENSE-2.0.txt)

## License

This project is licensed under [Apache 2 licensed](http://www.apache.org/licenses/LICENSE-2.0.txt). See `LICENSE` file for the full text.
