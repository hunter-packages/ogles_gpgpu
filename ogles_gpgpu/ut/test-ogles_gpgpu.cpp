#ifdef NDEBUG
#undef NDEBUG
#endif

#include <aglet/GLContext.h>
#include <aglet/aglet.h>

#include <gtest/gtest.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// NOTE: GL_BGRA is absent in Android NDK
// clang-format off
#ifdef ANDROID
#  define TEXTURE_FORMAT GL_RGBA
static cv::Vec4b torgba(const cv::Vec4b &p) { return cv::Vec4b(p[0], p[1], p[2], p[3]); }
static cv::Vec3b torgb(const cv::Vec3b &p) { return cv::Vec3b(p[0], p[1], p[2]); }
#else
#  define TEXTURE_FORMAT GL_BGRA
static cv::Vec4b torgba(const cv::Vec4b &p) { return cv::Vec4b(p[2], p[1], p[0], p[3]); }
static cv::Vec3b torgb(const cv::Vec3b &p) { return cv::Vec3b(p[2], p[1], p[0]); }
#endif
// clang-format off

#define OGLES_GPGPU_DEBUG_YUV 0

#include "../common/gl/memtransfer_optimized.h"

// clang-format off

#include "../common/proc/letterbox.h"    // [x]
#include "../common/proc/rgb2luv.h"      // [x]
#include "../common/proc/swizzle.h"      // [x]
#include "../common/proc/yuv2rgb.h"      // [x]
#include "../common/proc/lnorm.h"        // [0]
#include "../common/proc/video.h"        // [x]
#include "../common/proc/adapt_thresh.h" // [x]
#include "../common/proc/gain.h"         // [x]
#include "../common/proc/blend.h"        // [x]
#include "../common/proc/grayscale.h"    // [x]
#include "../common/proc/fifo.h"         // [x]
#include "../common/proc/diff.h"         // [x]
#include "../common/proc/transform.h"    // [0]
#include "../common/proc/gauss.h"        // [0]
#include "../common/proc/gauss_opt.h"    // [0]
#include "../common/proc/box_opt.h"      // [0]
#include "../common/proc/hessian.h"      // [0]
#include "../common/proc/lbp.h"          // [0]
#include "../common/proc/median.h"       // [0]
#include "../common/proc/fir3.h"         // [0]
#include "../common/proc/grad.h"         // [0]
#include "../common/proc/iir.h"          // [?]
#include "../common/proc/lowpass.h"      // [0]
#include "../common/proc/highpass.h"     // [0]
#include "../common/proc/thresh.h"       // [0]
#include "../common/proc/pyramid.h"      // [0]
#include "../common/proc/ixyt.h"         // [0]
#include "../common/proc/tensor.h"       // [0]
#include "../common/proc/shitomasi.h"    // [0]
#include "../common/proc/harris.h"       // [0]
#include "../common/proc/nms.h"          // [0]
#include "../common/proc/flow.h"         // [0]
#include "../common/proc/rgb2hsv.h"      // [0]
#include "../common/proc/hsv2rgb.h"      // [0]
#include "../common/proc/remap.h"        // [-]
#include "../common/proc/mesh.h"         // [x]
// clang-format on

// virtual (tested indirectly)
//#include "../common/proc/filter3x3.h"
//#include "../common/proc/two.h"
//#include "../common/proc/three.h"

#include <type_traits>

// https://stackoverflow.com/a/8024562
template <typename T, bool> struct absdiff_aux;

template <typename T> struct absdiff_aux<T, true>
{
    static T absdiff(T a, T b)
    {
        return (a < b) ? (b - a) : (a - b);
    }
};

template <typename T> struct absdiff_aux<T, false>
{
    typedef typename std::make_unsigned<T>::type UT;
    static UT absdiff(T a, T b)
    {
        if ((a >= 0 && b >= 0) || (a < 0 && b < 0))
        {
            return (a < b) ? (b - a) : (a - b);
        }

        if (b > 0)
        {
            UT d = -a;
            return UT(b) + d;
        }
        else
        {
            UT d = -b;
            return UT(a) + d;
        }
    }
};

template <typename T> typename std::make_unsigned<T>::type absdiff(T a, T b)
{
  return absdiff_aux<T, std::is_signed<T>::value>::absdiff(a, b);
}

namespace cv
{
    template <typename T, int N>
    cv::Vec<T, N> absdiff(const cv::Vec<T, N> &a, const cv::Vec<T, N> &b)
    {
        cv::Vec<T, N> d;
        for (int i = 0; i < N; i++)
        {
            d[i] = ::absdiff(a[i], b[i]);
        }
        return d;
    }
}

template <typename T, int N>
T max_element(const cv::Vec<T, N> &a)
{
    T b = a[0];
    for (int i = 1; i < N; i++)
    {
        if (a[i] > b)
        {
            b = a[i];
        }
    }
    return b;
}

template <typename T, int N>
bool almost_equal(const cv::Mat_<cv::Vec<T,N>> &a, const cv::Mat_<cv::Vec<T,N>> &b, T error)
{
    using VecType = cv::Vec<T,N>;

    auto almost_equal = [error](const VecType& a, const VecType& b)
    {
        return max_element(cv::absdiff(a, b)) <= error;
    };

    return std::equal(a.begin(), a.end(), b.begin(), almost_equal);
}
    
static cv::Vec3f cvtColorRgb2Luv(const cv::Vec3f& rgb);

int gauze_main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto code = RUN_ALL_TESTS();
    return code;
}

struct GLTexture {
    GLTexture(std::size_t width, std::size_t height, GLenum texType, void* data, GLint texFormat = GL_RGBA) {
        glGenTextures(1, &texId);
        assert(glGetError() == GL_NO_ERROR);

        glBindTexture(GL_TEXTURE_2D, texId);
        assert(glGetError() == GL_NO_ERROR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        assert(glGetError() == GL_NO_ERROR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, texFormat, width, height, 0, texType, GL_UNSIGNED_BYTE, data);
        assert(glGetError() == GL_NO_ERROR);

        glBindTexture(GL_TEXTURE_2D, 0);
        assert(glGetError() == GL_NO_ERROR);
    }

    ~GLTexture() {
        glDeleteTextures(1, &texId);
        assert(glGetError() == GL_NO_ERROR);
    }

    operator GLuint() const {
        return texId;
    }

    GLuint texId;
};

static cv::Mat getImage(ogles_gpgpu::ProcInterface& proc, cv::Mat& frame) {
    if (dynamic_cast<ogles_gpgpu::MemTransferOptimized*>(proc.getMemTransferObj())) {
        ogles_gpgpu::MemTransfer::FrameDelegate delegate = [&](const ogles_gpgpu::Size2d& size, const void* pixels, size_t bytesPerRow) {
            frame = cv::Mat(size.height, size.width, CV_8UC4, (void*)pixels, bytesPerRow).clone();
        };
        proc.getResultData(delegate);
    } else {
        frame.create(proc.getOutFrameH(), proc.getOutFrameW(), CV_8UC4); // noop if preallocated
        proc.getResultData(frame.ptr());
    }
    return frame;
}

static cv::Mat getTestImage(int width, int height, int stripe, bool alpha, GLenum format) {
    // Create a test image:
    cv::Mat test(height, width, CV_8UC3, cv::Scalar::all(0));
    cv::Point center(test.cols / 2, test.rows / 2);
    for (int i = test.cols / 2; i > 0; i -= stripe) {
        cv::circle(test, center, i, cv::Scalar(rand() % 255, rand() % 255, rand() % 255), -1, 8);
    }

    if (format == GL_RGBA /* or GL_RGB */) {
        cv::cvtColor(test, test, cv::COLOR_BGR2RGB);
    }

    if (alpha) {
        cv::cvtColor(test, test, cv::COLOR_BGR2BGRA); // add alpha
    }

    return test;
}

//######################
//### Shader Testing ###
//######################

#include <chrono>

static int gWidth = 640;
static int gHeight = 480;
static aglet::GLContext::GLVersion gVersion = aglet::GLContext::kGLES30;

#if defined(OGLES_GPGPU_OPENGL_ES3)
TEST(OGLESGPGPUTest, PingPong) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        static const int value = 1, g = 10, width = 2048, height = 2048;
        cv::Mat test(height, width, CV_8UC4, cv::Scalar(value, value, value, 255));

        glActiveTexture(GL_TEXTURE0);

        ogles_gpgpu::GainProc gain(g);
        gain.setOutputPboCount(2); // support ping-pong

        ogles_gpgpu::VideoSource video;
        video.set(&gain);

        cv::Mat tmp(height, width, CV_8UC4, cv::Scalar::all(0));
        for (int i = 0; i < 4; i++) {
            video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

            gain.getResultData(nullptr, (i + 0) % 2); // queue up transfer for current frame:
            if (i > 0) {
                gain.getResultData(tmp.ptr<uint8_t>(), (i - 1) % 2);
                ASSERT_EQ(static_cast<int>(cv::mean(tmp)[0]), (value * g));
            }
        }

        cv::Mat result;
        getImage(gain, result);
        ASSERT_EQ(static_cast<int>(cv::mean(result)[0]), (value * g));
    }
}
#endif // defined(OGLES_GPGPU_OPENGL_ES3)


inline cv::Vec4b convert(const cv::Scalar &value)
{
    return cv::Vec4b(value[0], value[1], value[2], value[3]);
}

TEST(OGLESGPGPUTest, LetterboxProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::LetterboxProc letterbox;

        letterbox.setHeight(0.25f);
        letterbox.setColor(1.0f, 1.0f, 1.0f);

        video.set(&letterbox);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result;
        getImage(letterbox, result);
        ASSERT_FALSE(result.empty());

        // Check for expected color in top and bottom bands:

        int band = test.rows/8;

        cv::Rect roi(0,0, result.cols, band);
        const auto colorUpper = cv::mean(result(roi));
        const auto colorLower = cv::mean(result(roi + cv::Point(0,test.rows-1-band)));

        // Check for expected color in top and bottom bands:
        ASSERT_EQ(convert(colorUpper), convert(cv::Scalar::all(255)));
        ASSERT_EQ(convert(colorLower), convert(cv::Scalar::all(255)));
    }
}

TEST(OGLESGPGPUTest, Rgb2LuvProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::Rgb2LuvProc rgb2luv;

        video.set(&rgb2luv);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result;
        getImage(rgb2luv, result);
        ASSERT_FALSE(result.empty());

        for(int i = 0; i < std::min(result.rows, result.cols); i++)
        {
            cv::Vec4b in = test.at<cv::Vec4b>(i, i);
            cv::Vec4b out = torgba(result.at<cv::Vec4b>(i, i));

            cv::Vec3b rgb = torgb(cv::Vec3b(in[0], in[1], in[2]));
            cv::Vec3b luv = cvtColorRgb2Luv(cv::Vec3f(rgb) * (1.0/255.0)) * 255.0;

            ASSERT_LE(max_element(cv::absdiff(cv::Vec3b(out[0],out[1],out[2]), luv)), 2);
        }
    }
}

TEST(OGLESGPGPUTest, SwizzleProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::SwizzleProc noop(ogles_gpgpu::SwizzleProc::kSwizzleRGBA);
        ogles_gpgpu::SwizzleProc swizzle(ogles_gpgpu::SwizzleProc::kSwizzleBGRA);

        video.set(&noop);
        noop.add(&swizzle);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result1, result2;

        getImage(noop, result1);
        ASSERT_FALSE(result1.empty());
                
        getImage(swizzle, result2);
        ASSERT_FALSE(result2.empty());

        cv::cvtColor(result2, result2, cv::COLOR_RGBA2BGRA);

        ASSERT_TRUE(almost_equal(cv::Mat4b(result1), cv::Mat4b(result2), static_cast<unsigned char>(2)));
    }
}

TEST(OGLESGPGPUTest, GrayScaleProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GrayscaleProc gray;

        gray.setGrayscaleConvType(ogles_gpgpu::GRAYSCALE_INPUT_CONVERSION_RGB);

        video.set(&gray);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result;
        getImage(gray, result);
        ASSERT_FALSE(result.empty());

        cv::Mat truth;
        cv::cvtColor(test, truth, (TEXTURE_FORMAT == GL_RGBA) ? cv::COLOR_RGBA2GRAY : cv::COLOR_BGRA2GRAY);
        cv::cvtColor(truth, truth, cv::COLOR_GRAY2BGRA);
        ASSERT_TRUE(almost_equal(cv::Mat4b(result), cv::Mat4b(truth), static_cast<std::uint8_t>(2)));
    }
}

TEST(OGLESGPGPUTest, WriteAndRead) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);

        ogles_gpgpu::GainProc gain;
        auto* filter = dynamic_cast<ogles_gpgpu::ProcInterface*>(&gain);

        static const bool prepareForExternalInput = true;

        // Set pixel data format for input data to <fmt>. Must be set before init() / reinit().
        filter->setExternalInputDataFormat(TEXTURE_FORMAT);

        // Init the processor for input frames of size <inW>x<inH> which is at position <order>
        // in the processing pipeline.
        filter->init(test.cols, test.rows, 0, prepareForExternalInput);

        // Insert external data into this processor. It will be used as input texture.
        // Note: init() must have been called with prepareForExternalInput = true for that.
        filter->setExternalInputData(test.ptr<std::uint8_t>());

        // Createa and bind an output FBO + texture.
        filter->createFBOTex(false);

        // Notify the MemTransfer object that we are using raw data, as opposed to
        // platform specific image types (i.e., CMSampleBuffer)
        filter->getInputMemTransferObj()->setUseRawPixels(true);

        // Perform off screen rendering to output FBO:
        gain.render();

        cv::Mat result;
        getImage(gain, result);
        ASSERT_TRUE(almost_equal(cv::Mat4b(test), cv::Mat4b(result), static_cast<std::uint8_t>(2)));
    }
}

TEST(OGLESGPGPUTest, Yuv2RgbProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {

        cv::Mat green(1, 1, CV_8UC3, cv::Scalar(0, 255, 0)), yuv;
        cv::cvtColor(green, yuv, cv::COLOR_BGR2YUV);
        cv::Vec3b& value = yuv.at<cv::Vec3b>(0, 0);

        // Create constant color buffers:

        // Test interleaved NV12 format:
        std::vector<std::uint8_t> y(gWidth * gHeight, value[0]);
        
        { // ##### Test interleaved NV21 format ###########
            std::vector<std::uint8_t> uv(gWidth * gHeight / 2);
            for (int i = 0; i < uv.size(); i += 2) {
                uv[i + 0] = value[1];
                uv[i + 1] = value[2];
            }
            
#if defined(OGLES_GPGPU_OPENGL_ES3)
            // Luminance texture:
            GLTexture luminanceTexture(gWidth, gHeight, GL_RED, y.data(), GL_R8);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);
            std::cout << "ES3 k601VideoRange kRG" << std::endl;

            // Chrominance texture (interleaved):
            GLTexture chrominanceTexture(gWidth / 2, gHeight / 2, GL_RG, uv.data(), GL_RG8);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);

            ogles_gpgpu::Yuv2RgbProc yuv2rgb(ogles_gpgpu::Yuv2RgbProc::k601VideoRange, ogles_gpgpu::Yuv2RgbProc::kRG);
#else
            // Luminance texture:
            GLTexture luminanceTexture(gWidth, gHeight, GL_LUMINANCE, y.data(), GL_LUMINANCE);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);

            // Chrominance texture (interleaved):
            GLTexture chrominanceTexture(gWidth / 2, gHeight / 2, GL_LUMINANCE_ALPHA, uv.data(), GL_LUMINANCE_ALPHA);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);

            ogles_gpgpu::Yuv2RgbProc yuv2rgb(ogles_gpgpu::Yuv2RgbProc::k601VideoRange, ogles_gpgpu::Yuv2RgbProc::kLA);
#endif

            // Set pixel data format for input data to <fmt>. Must be set before init() / reinit().
            yuv2rgb.setExternalInputDataFormat(0); // for yuv

            // Init the processor for input frames of size <inW>x<inH> which is at position <order>
            // in the processing pipeline.
            yuv2rgb.init(gWidth, gHeight, 0, true);

            // Be sure to specify standard (GL_RGBA) output texture type
            // for this case where input textures == 0 are handled as special
            // separate Y and UV textures.
            yuv2rgb.getMemTransferObj()->setOutputPixelFormat(TEXTURE_FORMAT);

            // Create an FBO
            yuv2rgb.createFBOTex(false);

            // Provide the input Y and UV textures:
            yuv2rgb.setTextures(luminanceTexture, chrominanceTexture);

            // Perform the rendering
            yuv2rgb.render();

            cv::Mat result;
            getImage(yuv2rgb, result);
            ASSERT_FALSE(result.empty());

            auto mu = cv::mean(result);

#if OGLES_GPGPU_DEBUG_YUV
            ogles_gpgpu::GainProc yProc;
            yProc.prepare(gWidth, gHeight);
            yProc.process(luminanceTexture, 1, GL_TEXTURE_2D);
            cv::Mat yProcOut;
            getImage(yProc, yProcOut);

            ogles_gpgpu::GainProc uvProc;
            uvProc.prepare(gWidth, gHeight);
            uvProc.process(chrominanceTexture, 1, GL_TEXTURE_2D);
            cv::Mat uvProcOut;
            getImage(uvProc, uvProcOut);

            std::cout << "yuv_in  : " << value << std::endl;
            std::cout << "rgb_out : " << mu << std::endl;
            std::cout << "y_      : " << cv::mean(yProcOut) << std::endl;
            std::cout << "uv_     : " << cv::mean(uvProcOut) << std::endl;
            std::cout << "Format: " << int(yProc.getMemTransferObj()->getOutputPixelFormat()) << std::endl;
#endif // OGLES_GPGPU_DEBUG_YUV

            ASSERT_LE(mu[0], 8);
            ASSERT_GE(mu[1], 250);
            ASSERT_LE(mu[2], 8);
        }

        {
            // ########### Test planar YV12 format ############
            std::vector<std::uint8_t> u(gWidth * gHeight/4), v(gWidth * gHeight/4);
            for (int i = 0; i < u.size(); i ++) {
                u[i] = value[1];
                v[i] = value[2];
            }        
        
#if defined(GL_LUMINANCE)
            const GLenum glLuminance = GL_LUMINANCE;
#else
            const GLenum glLuminance = GL_RED;
#endif
            
            // Luminance texture:
            GLTexture luminanceTexture(gWidth, gHeight, glLuminance, y.data(), glLuminance);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);

            // U texture:
            GLTexture uTexture(gWidth / 2, gHeight / 2, glLuminance, u.data(), glLuminance);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);
            
            // V texture:
            GLTexture vTexture(gWidth / 2, gHeight / 2, glLuminance, v.data(), glLuminance);
            ASSERT_EQ(glGetError(), GL_NO_ERROR);            

            ogles_gpgpu::Yuv2RgbProc yuv2rgb(ogles_gpgpu::Yuv2RgbProc::k601VideoRange, ogles_gpgpu::Yuv2RgbProc::kYUV12);

            // Set pixel data format for input data to <fmt>. Must be set before init() / reinit().
            yuv2rgb.setExternalInputDataFormat(0); // for yuv

            // Init the processor for input frames of size <inW>x<inH> which is at position <order>
            // in the processing pipeline.
            yuv2rgb.init(gWidth, gHeight, 0, true);

            // Be sure to specify standard (GL_RGBA) output texture type
            // for this case where input textures == 0 are handled as special
            // separate Y and UV textures.
            yuv2rgb.getMemTransferObj()->setOutputPixelFormat(TEXTURE_FORMAT);

            // Create an FBO
            yuv2rgb.createFBOTex(false);

            // Provide the input Y and UV textures:
            yuv2rgb.setTextures(luminanceTexture, uTexture, vTexture);

            // Perform the rendering
            yuv2rgb.render();

            cv::Mat result;
            getImage(yuv2rgb, result);
            ASSERT_FALSE(result.empty());

            auto mu = cv::mean(result);

            ASSERT_LE(mu[0], 8);
            ASSERT_GE(mu[1], 250);
            ASSERT_LE(mu[2], 8);
        }
    }
}

TEST(OGLESGPGPUTest, AdaptThreshProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::AdaptThreshProc thresh;

        video.set(&thresh);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result;
        getImage(thresh, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, GainProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        static const int value = 1, g = 10;
        cv::Mat test(gWidth, gHeight, CV_8UC4, cv::Scalar(value, value, value, 255));

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain(g);

        video.set(&gain);
        video({ { test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT });

        cv::Mat result;
        getImage(gain, result);
        ASSERT_EQ(static_cast<int>(cv::mean(result)[0]), (value * g));
    }
}

TEST(OGLESGPGPUTest, BlendProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        const float alpha = 0.5f;
        const int value = 2;
        const int a = 1, b = 10;
        cv::Mat test(gWidth, gHeight, CV_8UC4, cv::Scalar(value, value, value, 255));

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain1(a), gain10(b);
        ogles_gpgpu::BlendProc blend(alpha);

        gain1.add(&blend, 0);
        gain1.add(&gain10);
        gain10.add(&blend, 1);

        video.set(&gain1);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(blend, result);
        ASSERT_EQ(static_cast<int>(cv::mean(result)[0]), static_cast<int>(static_cast<float>((value * a) + (value * a * b)) * alpha));
    }
}

TEST(OGLESGPGPUTest, FIFOProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::FIFOPRoc fifo(3);
        video.set(&fifo);

        for (int i = 0; i < 3; i++) {
            cv::Mat test(gWidth, gHeight, CV_8UC4, cv::Scalar(i, i, i, 255));
            video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        for (int i = 0; i < 3; i++) {
            cv::Mat result;
            getImage(*fifo[i], result);
            ASSERT_EQ(cv::mean(result)[0], i);
        }
    }
}

TEST(OGLESGPGPUTest, TransformProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain(1.f);
        ogles_gpgpu::TransformProc transform;

        video.set(&gain);
        gain.add(&transform);

        ogles_gpgpu::Mat44f matrix;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                matrix.data[y][x] = 0;
            }
            matrix.data[y][y] = 1.f;
        }
        matrix.data[0][0] = 0.25f;

        transform.setTransformMatrix(matrix);

        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(transform, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, MeshProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {

        cv::Mat test = getTestImage(gWidth, gHeight, 10, true, TEXTURE_FORMAT);        

        const float w = gWidth;
        const float h = gHeight;
        std::vector<std::array<float,3>> verticesT, vertices
        {
            {{0.f, 0.f, 0.f}},
            {{w, 0.f, 0.f}},
            {{w, h, 0.f}},
            {{0.f, h, 0.f}},
            {{w/2.f,h/2.f,0.f}} // demonstrate stretching center point
        };
        
        std::vector<std::array<float,2>> coordsT, coords
        {
            {{0.f, 0.f}},
            {{1.f, 0.f}},
            {{1.f, 1.f}},
            {{0.f, 1.f}},
            {{0.5f,0.5f}} // center point
        };
        
        // Create explicit GL_TRIANGLES vertices and coords
        cv::Mat2f xy(gHeight, gWidth);
        
        for(int y = 0; y < 4; y++)
        {
            for(const auto &i : {(y+0)%4,(y+1)%4,4})
            {
                coordsT.push_back(coords[i]);
                verticesT.push_back(vertices[i]);
            }
        }
        
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::MeshShaderProc mesh(verticesT, coordsT);
        mesh.setTriangleKind(GL_TRIANGLES);

        video.set(&mesh);

        const cv::Matx33f S(cv::Matx33f::diag({(2.f/gWidth),2.f/gHeight,1.f}));
        const cv::Matx33f T(1.f,0.f,-1.f,0.f,1.f,-1.f,0.f,0.f,1.f);
        const cv::Matx33f Ht = (T * S).t();

        const std::array<int,4> index {{0,1,3}};
        ogles_gpgpu::Mat44f matrix;
        for(int y = 0; y < 4; y++) {
            for(int x = 0; x < 4; x++) {
                matrix.data[y][x] = 0.f;
            }
            matrix.data[y][y] = 1.f;            
        }
        
        for(int y = 0; y < 3; y++) {
            for(int x = 0; x < 3; x++) {
                matrix.data[index[y]][index[x]] = Ht(y,x);
            }
        }

        mesh.setModelViewProjection(matrix);

        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(mesh, result);
        ASSERT_FALSE(result.empty());

        auto error = cv::norm(result, test, cv::NORM_L1) / static_cast<float>(gWidth*gHeight*4);
        ASSERT_LE(error, 1);
    }
}

TEST(OGLESGPGPUTest, DiffProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        const int value = 2;
        const int a = 1, b = 10;
        cv::Mat test(gWidth, gHeight, CV_8UC4, cv::Scalar(value, value, value, 255));

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain1(a), gain10(b);
        ogles_gpgpu::DiffProc diff;

        gain1.add(&diff, 1);
        gain1.add(&gain10);
        gain10.add(&diff, 0);

        video.set(&gain1);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(diff, result);
        ASSERT_EQ(static_cast<int>(cv::mean(result)[0]), static_cast<int>(static_cast<float>((value * a * b) - (value * a))));
    }
}

TEST(OGLESGPGPUTest, GaussianProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 1, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GaussProc gauss1, gauss2;

        video.set(&gauss1);
        gauss1.add(&gauss2);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(gauss2, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, GaussianOptProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 1, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GaussOptProc gauss;

        video.set(&gauss);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(gauss, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, BoxOptProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 1, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::BoxOptProc box;

        video.set(&box);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(box, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, HessianProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test(gWidth, gHeight, CV_8UC1, cv::Scalar::all(0));

        static const int tic = 20;
        for (int y = tic; y <= test.rows - tic; y += tic) {
            for (int x = tic; x <= test.cols - tic; x += tic) {
                cv::circle(test, { x, y }, 1, 255, -1, 8);
            }
        }
        cv::GaussianBlur(test, test, { 7, 7 }, 1.0);
        cv::cvtColor(test, test, cv::COLOR_GRAY2BGR);
        cv::cvtColor(test, test, cv::COLOR_BGR2BGRA);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::HessianProc hessian(100.f);

        video.set(&hessian);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result, alpha;
        getImage(hessian, result);
        cv::extractChannel(result, alpha, 3);
        ASSERT_FALSE(alpha.empty());
    }
}

TEST(OGLESGPGPUTest, LbpProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 1, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::LbpProc lbp;

        video.set(&lbp);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(lbp, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, Fir3Proc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        const cv::Size size(gWidth, gHeight);
        const cv::Point center(size.width / 2, size.height / 2);
        const float radius = size.height / 2;

        std::vector<cv::Mat> test(3);
        std::vector<int> values{ 64, 128, 64 };

        for (int i = 0; i < 3; i++) {
            auto& value = values[i];
            cv::Mat canvas(size, CV_8UC3, cv::Scalar(value, value, value, 255));
            cv::circle(canvas, center, radius, values[i], -1, 4);
            cv::cvtColor(canvas, canvas, cv::COLOR_BGR2BGRA);
            test[i] = canvas;
        }

        static const bool doRgb = true;

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain(1.f); // noop
        ogles_gpgpu::FifoProc fifo(3);
        ogles_gpgpu::Fir3Proc fir3(doRgb);
        fir3.setWeights({ -0.25f, -0.25f, -0.25f }, { +0.50f, +0.50f, +0.50f }, { -0.25f, -0.25f, -0.25f });

        video.set(&gain);
        gain.add(&fifo);
        fifo.addWithDelay(&fir3, 0, 0);
        fifo.addWithDelay(&fir3, 1, 1);
        fifo.addWithDelay(&fir3, 2, 2);

        for (int i = 0; i < 3; i++) {
            video({ test[i].cols, test[i].rows }, test[i].ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        cv::Mat result;
        getImage(fir3, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, GradProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GradProc grad;

        video.set(&grad);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(grad, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, LowPassProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        std::vector<cv::Mat> images{
            cv::Mat(gWidth, gHeight, CV_8UC4, cv::Scalar(0, 0, 0, 255)),
            cv::Mat(gWidth, gHeight, CV_8UC4, cv::Scalar(255, 255, 255, 255))
        };

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::LowPassFilterProc low;
        video.set(&low);

        for (int i = 0; i < 5; i++) {
            cv::Mat& test = images[i % 2];
            video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        cv::Mat result;
        getImage(low, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, HighPassProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        std::vector<cv::Mat> images{
            cv::Mat(gWidth, gHeight, CV_8UC4, cv::Scalar(0, 0, 0, 255)),
            cv::Mat(gWidth, gHeight, CV_8UC4, cv::Scalar(255, 255, 255, 255))
        };

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::HighPassFilterProc high;
        video.set(&high);

        for (int i = 0; i < 5; i++) {
            cv::Mat& test = images[i % 2];
            video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        cv::Mat result;
        getImage(high, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, ThreshProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::ThreshProc thresh;

        video.set(&thresh);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(thresh, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, PyramidProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::PyramidProc pyramid(10);
        video.set(&pyramid);

        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(pyramid, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, IxytProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain(1.f);
        ogles_gpgpu::FifoProc fifo(2);
        ogles_gpgpu::IxytProc ixyt;
        ixyt.setWaitForSecondTexture(true);

        video.set(&gain);
        gain.add(&fifo);
        fifo.addWithDelay(&ixyt, 1, 1);
        fifo.addWithDelay(&ixyt, 0, 0);

        for (int i = 0; i < 5; i++) {
            cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);
            video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        cv::Mat result;
        getImage(ixyt, result);

        std::vector<cv::Mat> channels;
        cv::split(result, channels);
        cv::hconcat(channels, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, TensorProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::TensorProc tensor;

        video.set(&tensor);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(tensor, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, ShiTomasiProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);
        for (int i = 0; i < 100; i++) {
            cv::Point p0(rand() % test.cols, rand() % test.rows);
            cv::Point p1(rand() % test.cols, rand() % test.rows);
            cv::line(test, p0, p1, { 0, 255, 0 }, 2, 8);
        }

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GaussOptProc gauss;
        ogles_gpgpu::TensorProc tensor;
        ogles_gpgpu::ShiTomasiProc shiTomasi;

        tensor.setEdgeStrength(100.f);
        shiTomasi.setSensitivity(100000000.f);

        video.set(&gauss);
        gauss.add(&tensor);
        tensor.add(&shiTomasi);

        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(shiTomasi, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, HarrisProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 2, true, TEXTURE_FORMAT);
        for (int i = 0; i < 100; i++) {
            cv::Point p0(rand() % test.cols, rand() % test.rows);
            cv::Point p1(rand() % test.cols, rand() % test.rows);
            cv::line(test, p0, p1, { 0, 255, 0 }, 2, 8);
        }

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GaussOptProc gauss;
        ogles_gpgpu::TensorProc tensor;
        ogles_gpgpu::HarrisProc harris;

        tensor.setEdgeStrength(100.f);
        harris.setSensitivity(100000000.f);

        video.set(&gauss);
        gauss.add(&tensor);
        tensor.add(&harris);

        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(harris, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, NmsProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test(gWidth, gHeight, CV_8UC1, cv::Scalar::all(0));

        static const int tic = 20;
        for (int y = tic; y <= test.rows - tic; y += tic) {
            for (int x = tic; x <= test.cols - tic; x += tic) {
                cv::circle(test, { x, y }, 1, 255, -1, 8);
            }
        }
        cv::GaussianBlur(test, test, { 7, 7 }, 1.0);
        cv::cvtColor(test, test, cv::COLOR_GRAY2BGR);
        cv::cvtColor(test, test, cv::COLOR_BGR2BGRA);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::HessianProc hessian(100.f);
        ogles_gpgpu::NmsProc nms;

        video.set(&hessian);
        hessian.add(&nms);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result, alpha;
        getImage(nms, result);
        cv::extractChannel(result, result, 3);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, FlowProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 3, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gain(1.f);
        ogles_gpgpu::Flow2Pipeline flow;

        video.set(&gain);
        gain.add(&flow);

        for (int i = 0; i < 5; i++) {
            cv::Mat shifted;
            cv::Matx23f M(1, 0, i * 4, 0, 1, i * 4);
            cv::warpAffine(test, shifted, M, test.size());
            video({ shifted.cols, shifted.rows }, shifted.ptr<void>(), true, 0, TEXTURE_FORMAT);
        }

        cv::Mat result;
        getImage(flow, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, Rgb2HsvProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 3, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::Rgb2HsvProc rgb2hsv;

        video.set(&rgb2hsv);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(rgb2hsv, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, Hsv2RgbProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 3, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::Hsv2RgbProc hsv2rgb;

        video.set(&hsv2rgb);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(hsv2rgb, result);
        ASSERT_FALSE(result.empty());
    }
}

TEST(OGLESGPGPUTest, LNormProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 3, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::GainProc gainProc(1.f);
        ogles_gpgpu::LocalNormProc normProc;

        video.set(&gainProc);
        gainProc.add(&normProc);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(normProc, result);
        ASSERT_FALSE(result.empty());
    }
}

#if !defined(_WIN32) && !defined(_WIN64)
// vs-14-2015 GLSL reports the following error due to internal preprocessor #define
// > could not compile shader program.  error log:
// > 0:1(380): preprocessor error: syntax error, unexpected HASH_TOKEN
TEST(OGLESGPGPUTest, MedianProc) {
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    if (context && *context) {
        cv::Mat test = getTestImage(gWidth, gHeight, 20, true, TEXTURE_FORMAT);

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::MedianProc median;

        video.set(&median);

        cv::Mat noise = cv::Mat::zeros(test.rows, test.cols, CV_8UC1);
        cv::randu(noise, 0, 255);
        test.setTo(0, noise < 30);
        test.setTo(255, noise > 225);

        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, TEXTURE_FORMAT);

        cv::Mat result;
        getImage(median, result);
        ASSERT_FALSE(result.empty());
    }
}
#endif

static cv::Vec3f cvtColorRgb2Luv(const cv::Vec3f& rgb)
{
    // column major format (glsl)
    cv::Matx33f RGBtoXYZ(0.430574, 0.222015, 0.020183, 0.341550, 0.706655, 0.129553, 0.178325, 0.071330, 0.939180);
    RGBtoXYZ = RGBtoXYZ.t(); // to row major

    const float y0 = 0.00885645167f; //pow(6.0/29.0, 3.0);
    const float a = 903.296296296f;  //pow(29.0/3.0, 3.0);
    const float un = 0.197833f;
    const float vn = 0.468331f;
    const float maxi = 0.0037037037f; // 1.0/270.0;
    const float minu = maxi * -88.0f;
    const float minv = maxi * -134.0f;
    const cv::Vec3f k(1.0f, 15.0f, 3.0f);

    cv::Vec3f xyz = (RGBtoXYZ * rgb); // make like glsl col major
    const float c = (xyz.dot(k) + 1e-35);
    const float z = 1.0f / c;

    cv::Vec3f luv;
    luv[0] = ((xyz[1] > y0) ? (116.0f * std::pow(xyz[1], 0.3333333333f) - 16.0f) : (xyz[1] * a)) * maxi;
    luv[1] = luv[0] * ((52.0f * xyz[0] * z) - (13.0f * un)) - minu;
    luv[2] = luv[0] * ((117.0f * xyz[1] * z) - (13.0f * vn)) - minv;

    return luv;
}
