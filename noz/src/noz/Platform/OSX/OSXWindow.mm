
#import <noz.pch>

#define GLEW_STATIC
#import <GL/glew.h>

#import <noz/Platform/WindowHandle.h>
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import <OpenGL/OpenGL.h>
#import <GLUT/glut.h>


using namespace noz;
using namespace noz::Platform;

@interface OSXWindow : NSWindow
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

@implementation OSXWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}
@end

@interface OSXView : NSView
@end


@interface GLView : NSOpenGLView
{
  CVDisplayLinkRef displayLink; //display link for managing rendering thread
}

@end

@implementation GLView

- (id) initWithFrame: (NSRect) frame {
/*
  static bool glew=false;
  if(!glew) {
    glewInit();
    glew=true;
  }  
*/

  NSOpenGLPixelFormatAttribute windowedAttrs[] =
 	{
    kCGLPFAOpenGLProfile,  kCGLOGLPVersion_3_2_Core,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAColorSize, 32,
    NSOpenGLPFADepthSize, 24,
    NSOpenGLPFAAlphaSize, 8,
    NSOpenGLPFAStencilSize, 8,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
    0
 	};
  
  NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
  self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
  return self;
}

- (void)prepareOpenGL {
  // Synchronize buffer swaps with vertical refresh rate
  GLint swapInt = 1;
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
 
  // Create a display link capable of being used with all active displays
  CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
 
  // Set the renderer output callback function
  CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, self);
 
  // Set the display link for the current renderer
  CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
 
  CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
 
  CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
 
  // Activate the display link
  CVDisplayLinkStart(displayLink);
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
 
{
 
    CVReturn result = [(GLView*)displayLinkContext getFrameForTime:outputTime];
 
    return result;
 
}

- (void)queueRender {
  Application::Frame();
}


- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime {
  [self performSelectorOnMainThread:@selector(queueRender) withObject:nil waitUntilDone:YES ];
  // Add your drawing codes here
  return kCVReturnSuccess;
}


- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)dealloc {
  // Release the display link
  CVDisplayLinkRelease(displayLink);
  [super dealloc];
}
 


@end
namespace noz {
namespace Platform {
	class OSXWindowHandle : public noz::Platform::WindowHandle {
    private: NSWindow* nswindow_;
    private: GLView* glview_;
  
    public: OSXWindowHandle (Window* window, WindowAttributes attr) {
			NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
      NSRect frame = NSMakeRect(100, 100, 640, 480);
      NSUInteger styleMask = NSResizableWindowMask|NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask;
      NSRect rect = [NSWindow contentRectForFrameRect:frame styleMask:styleMask];
		  nswindow_ = [[OSXWindow alloc] initWithContentRect:rect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
      [nswindow_ makeKeyAndOrderFront: nswindow_];
      [nswindow_ setOpaque:YES];
      [nswindow_ setAcceptsMouseMovedEvents:YES];
      
      glview_=[[GLView alloc] initWithFrame:rect];
      [nswindow_ setContentView:glview_];
      [[glview_ openGLContext] makeCurrentContext];
    }

    

    public: virtual void Show (void) override {}

    public: virtual void SetTitle (String title) override {}

    public: virtual Vector2 GetSize (void) const override {return Vector2(640,480);}

    public: virtual Rect GetScreenRect(void) const override {return Rect(0,0,640,480);}

    public: virtual Rect GetClientRect(void) const override {return Rect(0,0,640,480);}

    public: virtual Rect ClientToScreen(const Rect& r) const override {return r;}

    public: virtual void BeginPaint(void) override {
      [[glview_ openGLContext] makeCurrentContext];
      glClearColor(1,0,1,1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    public: virtual void EndPaint(void) override {
      [[glview_ openGLContext] flushBuffer];
    }

    public: virtual void SetSize(noz_float width, noz_float height) override {}

    public: virtual void Move(noz_float x, noz_float y) override {}

    public: virtual void SetCapture(void) override {}

    public: virtual void ReleaseCapture(void) override {}
  };
}
}


WindowHandle* WindowHandle::New (Window* window, WindowAttributes attr) {
  return new Platform::OSXWindowHandle (window,attr);
}
