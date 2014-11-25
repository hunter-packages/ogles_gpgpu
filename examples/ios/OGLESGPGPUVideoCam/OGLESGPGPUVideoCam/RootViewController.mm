#import "RootViewController.h"

/**
 * Small helper function to convert a fourCC <code> to
 * a character string <fourCC> for printf and the like
 */
void fourCCStringFromCode(int code, char fourCC[5]) {
    for (int i = 0; i < 4; i++) {
        fourCC[3 - i] = code >> (i * 8);
    }
    fourCC[4] = '\0';
}

@interface RootViewController(Private)
/**
 * Init views
 */
- (void)initUI;

/**
 * initialize camera
 */
- (void)initCam;

/**
 * Init ogles_gpgpu. Set up processing pipeline.
 */
- (void)initOGLESGPGPU;

/**
 * Notify the video session about the interface orientation change
 */
- (void)interfaceOrientationChanged:(UIInterfaceOrientation)o;

/**
 * handler that is called when a output selection button is pressed
 */
- (void)procOutputSelectBtnAction:(UIButton *)sender;

/**
 * force to redraw views. this method is only to display the intermediate
 * frame processing output for debugging
 */
- (void)updateViews;

- (void)prepareForFramesOfSize:(CGSize)size;

- (void)setCorrectedFrameForViews:(NSValue *)newFrameRect;

@end


@implementation RootViewController


#pragma mark init/dealloc

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        showCamPreview = YES;
        firstFrame = YES;
    }
    
    return self;
}

- (void)dealloc {
    // delete ogles_gpgpu singleton object
    ogles_gpgpu::Core::destroy();
    gpgpuMngr = NULL;
    
    // release camera stuff
    [vidDataOutput release];
    [camDeviceInput release];
    [camSession release];
    
    // release views
    [camView release];
    [baseView release];
    
    [super dealloc];
}

#pragma mark parent methods

- (void)didReceiveMemoryWarning {
    NSLog(@"memory warning!!!");
    
    [super didReceiveMemoryWarning];
}

- (void)loadView {
    [self initUI];
}

- (void)viewWillAppear:(BOOL)animated {
    NSLog(@"view will appear - start camera session");
    
    [camSession startRunning];
}

- (void)viewDidDisappear:(BOOL)animated {
    NSLog(@"view did disappear - stop camera session");
    
    [camSession stopRunning];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // set up camera
    [self initCam];
    
    // create an OpenGL context
    eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (![EAGLContext setCurrentContext:eaglContext]) {
        NSLog(@"failed setting current EAGL context");
    }
    
    [self initOGLESGPGPU];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)o duration:(NSTimeInterval)duration {
    [self interfaceOrientationChanged:o];
}

#pragma mark AVCaptureVideoDataOutputSampleBufferDelegate methods

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    // note that this method does *not* run in the main thread!
}

#pragma mark private methods

- (void)initUI {
    const CGRect screenRect = [[UIScreen mainScreen] bounds];
    
    NSLog(@"loading view of size %dx%d", (int)screenRect.size.width, (int)screenRect.size.height);
    
    // create an empty base view
    CGRect baseFrame = CGRectMake(0, 0, screenRect.size.height, screenRect.size.width);
    baseView = [[UIView alloc] initWithFrame:baseFrame];
    
    // create the image view for the camera frames
    camView = [[CamView alloc] initWithFrame:baseFrame];
    [baseView addSubview:camView];
    
    // set a list of buttons for processing output display
    NSArray *btnTitles = [NSArray arrayWithObjects:
                          @"Normal",
                          @"Processed",
                          nil];
    for (int btnIdx = 0; btnIdx < btnTitles.count; btnIdx++) {
        UIButton *procOutputSelectBtn = [UIButton buttonWithType:UIButtonTypeSystem];
        [procOutputSelectBtn setTag:btnIdx - 1];
        [procOutputSelectBtn setTitle:[btnTitles objectAtIndex:btnIdx]
                             forState:UIControlStateNormal];
        int btnW = 120;
        [procOutputSelectBtn setFrame:CGRectMake(10 + (btnW + 20) * btnIdx, 10, btnW, 35)];
        [procOutputSelectBtn setOpaque:YES];
        [procOutputSelectBtn addTarget:self
                                action:@selector(procOutputSelectBtnAction:)
                      forControlEvents:UIControlEventTouchUpInside];
        
        [baseView addSubview:procOutputSelectBtn];
    }
    
    // finally set the base view as view for this controller
    [self setView:baseView];

}

- (void)initOGLESGPGPU {
    NSLog(@"initializing ogles_gpgpu");
    
    // get ogles_gpgpu::Core singleton instance
    gpgpuMngr = ogles_gpgpu::Core::getInstance();
    
    // enable iOS optimizations (fast texture access)
    ogles_gpgpu::Core::usePlatformOptimizations = true;
    
    // do not use mipmaps (will not work with NPOT images)
    gpgpuMngr->setUseMipmaps(false);
    
    // set up grayscale processor
    grayscaleProc.setOutputSize(0.5f);  // downscale to half size
    grayscaleProc.setGrayscaleConvType(ogles_gpgpu::GRAYSCALE_INPUT_CONVERSION_BGR);    // needed, because we actually have BGRA input data when we use iOS optimized memory access
    
    // set up adaptive thresholding (two passes)
    adaptThreshProc[0].setThreshType(ogles_gpgpu::THRESH_ADAPTIVE_PASS_1);
    adaptThreshProc[1].setThreshType(ogles_gpgpu::THRESH_ADAPTIVE_PASS_2);
    
    // create the pipeline
    gpgpuMngr->addProcToPipeline(&grayscaleProc);
    //    gpgpuMngr->addProcToPipeline(&simpleThreshProc);
    gpgpuMngr->addProcToPipeline(&adaptThreshProc[0]);
    gpgpuMngr->addProcToPipeline(&adaptThreshProc[1]);
    
    // initialize the pipeline
    gpgpuMngr->init(eaglContext);

}

- (void)initCam {
    NSLog(@"initializing cam");
    
    NSError *error = nil;
    
    // set up the camera capture session
    camSession = [[AVCaptureSession alloc] init];
    [camSession setSessionPreset:CAM_SESSION_PRESET];
    [camView setSession:camSession];
    
    // get the camera device
	NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    assert(devices.count > 0);
    
	AVCaptureDevice *camDevice = [devices firstObject];
	for (AVCaptureDevice *device in devices) {
		if ([device position] == CAM_POSITION) {
			camDevice = device;
			break;
		}
	}
    
    camDeviceInput = [[AVCaptureDeviceInput deviceInputWithDevice:camDevice error:&error] retain];
    
    if (error) {
        NSLog(@"error getting camera device: %@", error);
        return;
    }
    
    assert(camDeviceInput);
    
    // add the camera device to the session
    if ([camSession canAddInput:camDeviceInput]) {
        [camSession addInput:camDeviceInput];
        [self interfaceOrientationChanged:self.interfaceOrientation];
    }
    
    // create camera output
    vidDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [camSession addOutput:vidDataOutput];
    
    // set output delegate to self
    dispatch_queue_t queue = dispatch_queue_create("vid_output_queue", NULL);
    [vidDataOutput setSampleBufferDelegate:self queue:queue];
    dispatch_release(queue);
    
    // get best output video format
    NSArray *outputPixelFormats = vidDataOutput.availableVideoCVPixelFormatTypes;
    int bestPixelFormatCode = -1;
    for (NSNumber *format in outputPixelFormats) {
        int code = [format intValue];
        if (bestPixelFormatCode == -1) bestPixelFormatCode = code;  // choose the first as best
        char fourCC[5];
        fourCCStringFromCode(code, fourCC);
        NSLog(@"available video output format: %s (code %d)", fourCC, code);
    }

    // specify output video format
    NSDictionary *outputSettings = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:bestPixelFormatCode]
                                                               forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    [vidDataOutput setVideoSettings:outputSettings];
    
//    // cap to 15 fps
//    [vidDataOutput setMinFrameDuration:CMTimeMake(1, 15)];
}

- (void)procOutputSelectBtnAction:(UIButton *)sender {
    NSLog(@"proc output selection button pressed: %@ (proc type %ld)",
          [sender titleForState:UIControlStateNormal], (long)sender.tag);
    
    showCamPreview = (sender.tag < 0);
    [camView setHidden:!showCamPreview];       // only show original camera frames in "normal" display mode
//    [procFrameView setHidden:showCamPreview];  // only show processed frames for other than "normal" display mode
}

- (void)interfaceOrientationChanged:(UIInterfaceOrientation)o {
    [[(AVCaptureVideoPreviewLayer *)camView.layer connection] setVideoOrientation:(AVCaptureVideoOrientation)o];
}

- (void)prepareForFramesOfSize:(CGSize)size {
    // WARNING: this method will not be called from the main thead!
    
//    float frameAspectRatio = size.width / size.height;
//    NSLog(@"camera frames are of size %dx%d (aspect %f)", (int)size.width, (int)size.height, frameAspectRatio);
//    
//    // update proc frame view size
//    float newViewH = procFrameView.frame.size.width / frameAspectRatio;   // calc new height
//    float viewYOff = (procFrameView.frame.size.height - newViewH) / 2;
//    
//    CGRect correctedViewRect = CGRectMake(0, viewYOff, procFrameView.frame.size.width, newViewH);
//    [self performSelectorOnMainThread:@selector(setCorrectedFrameForViews:)         // we need to execute this on the main thead
//                           withObject:[NSValue valueWithCGRect:correctedViewRect]   // otherwise it will have no effect
//                        waitUntilDone:NO];
}

- (void)setCorrectedFrameForViews:(NSValue *)newFrameRect {
    // WARNING: this *must* be executed on the main thread
    
//    // set the corrected frame for the proc frame view
//    CGRect r = [newFrameRect CGRectValue];
//    [procFrameView setFrame:r];
}

@end
