#import <UIKit/UIKit.h>

#import <AVFoundation/AVFoundation.h>
#import <AssetsLibrary/AssetsLibrary.h>

// include ogles_gpgpu main header
#include "../../../../ogles_gpgpu/ogles_gpgpu.h"

#import "CamView.h"

// Camera parameters

#define CAM_SESSION_PRESET  AVCaptureSessionPresetHigh
#define CAM_POSITION        AVCaptureDevicePositionBack

/**
 * Main view controller.
 * Handles UI initialization and interactions. Handles camera frame input.
 */
@interface RootViewController : UIViewController<AVCaptureVideoDataOutputSampleBufferDelegate> {
    AVCaptureSession *camSession;               // controlls the camera session
    AVCaptureDeviceInput *camDeviceInput;       // input device: camera
    AVCaptureVideoDataOutput *vidDataOutput;    // controlls the video output

    BOOL showCamPreview;
    BOOL firstFrame;
    
    CGSize frameSize;                   // currently grabbed camera frame size
    
    UIView *baseView;           // root view
    CamView *camView;           // shows the grabbed video frames ("camera preview")
    
    EAGLContext *eaglContext;   // OpenGL ES 2.0 context
    
    ogles_gpgpu::Core *gpgpuMngr;   // ogles_gpgpu manager
    
    ogles_gpgpu::GrayscaleProc grayscaleProc;   // pipeline processor 1: convert input to grayscale image
    ogles_gpgpu::ThreshProc simpleThreshProc;   // pipeline processor 2 (alternative 1): simple thresholding
    ogles_gpgpu::ThreshProc adaptThreshProc[2]; // pipeline processor 2 (alternative 2): adaptive thresholding (two passes)
}

@end
