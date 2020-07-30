
#import <noz.pch.h>

#import "IOSApp.h"
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/QuartzCore.h>
#import <noz/noz.h>
#import <noz/IO/Directory.h>
#import <noz/Platform/OpenGL/OpenGLRenderContext.h>

namespace noz { void RegisterTypes(void); }

using namespace noz;


AppDelegate* g_delegate = nil;
BOOL g_disabled = false;
noz_int32 g_opengl_shader_version = 300;

@implementation AppDelegate

- (void)dealloc {
  [_window release];
  [super dealloc];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  g_delegate = self;
  g_disabled = false;

  [[UIApplication sharedApplication] setIdleTimerDisabled:YES];

  noz::String res_path = noz::String([[[NSBundle mainBundle] resourcePath] UTF8String]);

  const char* argv[] = {res_path.ToCString(), nullptr};
  noz::Application::Initialize(1, argv);
  
  noz::Main();
  
  // Display link for advancing frame..
  CADisplayLink* fl=[CADisplayLink displayLinkWithTarget:g_delegate selector:@selector(do_frame:)];
  fl.frameInterval = 1;
  [fl addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];  
  
  return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
  // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
  // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
  g_disabled = true;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
  // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
  // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
  // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
  // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
  g_disabled = false;
}

- (void)applicationWillTerminate:(UIApplication *)application {
  noz::Application::Uninitialize();
}

- (void)do_frame:(id)data {
  if (g_disabled) return;
  
  noz::Application::Frame();
}

@end


namespace noz { class IOSWindow; }

@interface IOSNavigationController : UINavigationController
@end

@implementation IOSNavigationController
- (BOOL)prefersStatusBarHidden
{
    return YES;
}
@end

@interface IOSView : GLKView <UIKeyInput>
@property (nonatomic) noz::IOSWindow* window_;
@end


namespace noz {
  namespace Platform {
    noz_int32 GetOpenGLShaderVersion(void) {
	    return g_opengl_shader_version;
	  }
  }
  
  class IOSWindow : public noz::Platform::WindowHandle {
    protected: UIWindow* m_window;
    protected: EAGLContext* m_gl;
    protected: GLuint m_render_buffer;
    protected: GLuint m_frame_buffer;
    protected: IOSView* view_;
    protected: CAEAGLLayer* m_layer;
    public: Window* noz_window_;
    public: bool m_dirty;
    
    public: IOSWindow(Window* w, UIWindow* window, const char* caption, noz_uint32 style) {
      noz_window_ = w;
      m_window = window;
      m_window.backgroundColor = [UIColor whiteColor];
      [m_window makeKeyAndVisible];

      // Create the gl context
      m_gl = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
      if(nullptr == m_gl) {
		    m_gl = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		    g_opengl_shader_version = 100;
      }
      [EAGLContext setCurrentContext:m_gl];

      view_ = [[[IOSView alloc] initWithFrame:window.bounds] autorelease];
      [m_window addSubview:view_];
  	  view_.multipleTouchEnabled = true;
      view_.userInteractionEnabled = true;
      view_.window_ = this;
            
      glGenRenderbuffers(1, &m_render_buffer);
      glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);

      [m_gl renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)view_.layer];

      glGenFramebuffers(1, &m_frame_buffer);
      glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_render_buffer);
      
      glClearColor(150.0/255.0, 200.0/255.0, 255.0/255.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT);
      [m_gl presentRenderbuffer:GL_RENDERBUFFER];
    }

    public: virtual void Show(void) override {
      
    }
    
    public: virtual void SetFocus (void) override {}

    public: virtual void SetTitle(String title) override {}
    
    public: virtual void Move(noz_float w, noz_float h) override {}
    
    public: virtual void SetSize(noz_float w, noz_float h) override {}
       
    public: virtual Rect GetScreenRect(void) const override {Vector2 ss=GetScreenSize(); return Rect(0,0,ss.x,ss.y);}
    
    public: virtual Rect GetClientRect(void) const override {return GetScreenRect();}
    
    public: virtual Rect ClientToScreen(const Rect& r) const override {return r;}
    
    public: virtual Vector2 GetScreenSize (void) const override {
      CGFloat scale = [[UIScreen mainScreen ]scale];
      CGRect screenRect = [[UIScreen mainScreen] bounds];
      return Vector2(screenRect.size.width * scale,screenRect.size.height*scale);
	  }
    
    public: virtual Vector2 LocalToScreen (const Vector2& v) const override {return v;}
    
    public: virtual void ShowKeyboard (bool show) override {
      if(show) {
        [view_ becomeFirstResponder];
      } else {
        [view_ resignFirstResponder];
      }
    }
    
    public: virtual void BeginPaint(void) override {
      [EAGLContext setCurrentContext:m_gl];
      glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);
      glClearColor(250.0/255.0, 200.0/255.0, 255.0/255.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT);
    }
    
    public: virtual void EndPaint(void) override {
       glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);
       [m_gl presentRenderbuffer:GL_RENDERBUFFER];
    }
    
    public: virtual void SetCapture(void) override { }
    
    public: virtual void ReleaseCapture(void) override {}
  };
    
  Platform::WindowHandle* Platform::WindowHandle::New(noz::Window* window, WindowAttributes attr) {
    g_delegate.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    UINavigationController* navigationController = [[IOSNavigationController alloc] init];
    g_delegate.window.rootViewController = navigationController;

    return new IOSWindow(window,g_delegate.window,"",0);
  }
}



@implementation IOSView
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  NSEnumerator* enumerator = [touches objectEnumerator];
  UITouch* touch;
  while ((touch = (UITouch*)[enumerator nextObject])) {
  	CGPoint pt = [touch locationInView:self];
    CGFloat scale = [[UIScreen mainScreen ]scale];
	noz_uint64 id = (noz_uint64)touch;
	SystemEvent::PushEvent(SystemEventType::TouchBegan,self.window_->noz_window_,id,Vector2(pt.x*scale,pt.y*scale),(noz_uint32)[touch tapCount]);
  }
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  NSEnumerator* enumerator = [touches objectEnumerator];
  UITouch* touch;
  while ((touch = (UITouch*)[enumerator nextObject])) {
  	CGPoint pt = [touch locationInView:self];
    CGFloat scale = [[UIScreen mainScreen ]scale];
	noz_uint64 id = (noz_uint64)touch;
	SystemEvent::PushEvent(SystemEventType::TouchMoved,self.window_->noz_window_,id,Vector2(pt.x*scale,pt.y*scale),(noz_uint32)[touch tapCount]);
  }
}

-(void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  NSEnumerator* enumerator = [touches objectEnumerator];
  UITouch* touch;
  while ((touch = (UITouch*)[enumerator nextObject])) {
	CGPoint pt = [touch locationInView:self];
  CGFloat scale = [[UIScreen mainScreen ]scale];
	noz_uint64 id = (noz_uint64)touch;
	SystemEvent::PushEvent(SystemEventType::TouchEnded,self.window_->noz_window_,id,Vector2(pt.x*scale,pt.y*scale),(noz_uint32)[touch tapCount]);
  }
}

-(void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
  NSEnumerator* enumerator = [touches objectEnumerator];
  UITouch* touch;
  while ((touch = (UITouch*)[enumerator nextObject])) {
  	CGPoint pt = [touch locationInView:self];
    CGFloat scale = [[UIScreen mainScreen ]scale];
  	noz_uint64 id = (noz_uint64)touch;
	  SystemEvent::PushEvent(SystemEventType::TouchCancelled,self.window_->noz_window_,id,Vector2(pt.x*scale,pt.y*scale),(noz_uint32)[touch tapCount]);
  }
}

- (void)insertText:(NSString *)text {
  for(const char* s = [text UTF8String]; *s; s++) {
    if(*s >= 'a' && *s <= 'z') {
      SystemEvent::PushEvent(SystemEventType::KeyDown, self.window_->noz_window_, Keys::A + (*s-'a'));
    } else if (*s >= 'A' && *s <='Z') {
      SystemEvent::PushEvent(SystemEventType::KeyDown, self.window_->noz_window_, (Keys::A + (*s-'A')) | Keys::Shift);
    } else {
      noz_uint32 key_code = 0;
      switch(*s) {
        case ' ': key_code = Keys::Space; break;
        case '.': key_code = Keys::OemPeriod; break;
        case ',': key_code = Keys::OemComma; break;
        default: break;
      }
      if(key_code != 0) {
        SystemEvent::PushEvent(SystemEventType::KeyDown, self.window_->noz_window_, key_code);
      }
    }
  }

}
- (void)deleteBackward {
  SystemEvent::PushEvent(SystemEventType::KeyDown, self.window_->noz_window_, Keys::Back);
}
- (BOOL)hasText {
  return NO;
}

- (BOOL)canBecomeFirstResponder {
  return YES;
}

@end



namespace noz {
void ConsolePrint(const char* value) {
  NSLog(@"%s", value);
}
}


String GetIOSFolderPath (NSSearchPathDirectory spd) {
  NSFileManager* sharedFM = [NSFileManager defaultManager];
  NSArray* possibleURLs = [sharedFM URLsForDirectory:spd inDomains:NSUserDomainMask];
 
  NSURL* appSupportDir = nil;
  NSURL* appDirectory = nil;
 
  if ([possibleURLs count] >= 1) {
    // Use the first directory (if multiple are returned)
    appSupportDir = [possibleURLs objectAtIndex:0];
  }
  
  return String([[appSupportDir path] UTF8String]);
}


String noz::Environment::GetFolderPath(SpecialFolder folder) {
  switch (folder) {
    case SpecialFolder::Application: {
      static String dir;
      if(dir.IsEmpty()) {
        dir = String([[[NSBundle mainBundle] resourcePath] UTF8String]);
      }
      return dir;
    }
    
    case SpecialFolder::Document: {
      static String dir;
      if(dir.IsEmpty()) {
        dir = GetIOSFolderPath(NSDocumentDirectory);
      }
      return dir;
    }
      
    case SpecialFolder::ApplicationSupport: {
      static String dir;
      if(dir.IsEmpty()) {
        dir = GetIOSFolderPath(NSApplicationSupportDirectory);
      }
      return dir;
    }
    
    case SpecialFolder::Cache: {
      static String dir;
      if(dir.IsEmpty()) {
        dir = GetIOSFolderPath(NSCachesDirectory);
      }
      return dir;
    }
    
    case SpecialFolder::Temp: {
      static String dir;
      if(dir.IsEmpty()) {
        dir = [NSTemporaryDirectory() UTF8String];
      }
      return dir;
    }      
  }
  

  return "";
}


bool Directory::CreateDirectoryInternal(const String& path) {
  NSFileManager* mgr = [NSFileManager defaultManager];
  NSArray* dirPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
		NSUserDomainMask, YES);
      NSString* nspath = [NSString stringWithUTF8String:path.ToCString()];
  if([mgr createDirectoryAtPath:nspath withIntermediateDirectories:YES attributes:nil error:NULL]==NO) {
    return false;
  }
  
  return true;
}

Object* Application::GetClipboard (Type* type) {
  return nullptr;
}

bool Application::IsClipboardTypeAvailable (Type* type) {
  return false;
}

DragDropEffects DragDrop::DoDragDropImplementation (Window* window, Object* data, DragDropEffects allowedEffects) {
  return DragDropEffects::None;
}

void noz::Application::SetClipboard (Object* data) {
}

#include <noz/Platform/ProcessHandle.h>
noz::Platform::ProcessHandle* noz::Platform::ProcessHandle::CreateInstance(const char*, const char*) {
  return nullptr;
}


