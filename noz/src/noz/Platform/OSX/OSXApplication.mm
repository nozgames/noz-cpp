
#import <noz.pch>

#import "OSXApplication.h"
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <CoreVideo/CVDisplayLink.h>

using namespace noz;

AppDelegate* g_delegate = nil;
BOOL g_disabled = false;

@implementation AppDelegate


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  g_delegate = self;
  g_disabled = false;

  const char* argv[] = {"app.exe",nullptr};
  noz::Application::Initialize(1, argv);
  
  noz::Main();

/*
    CADisplayLink* fl=[CADisplayLink displayLinkWithTarget:g_delegate selector:@selector(do_frame:)];
    fl.frameInterval = 1;
    [fl addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
*/
}

- (void)applicationWillTerminate:(NSApplication *)application {
	noz::Application::Uninitialize();
}


- (void)do_frame:(id)data
{
  if(g_disabled) {
    return;
    
  }
  
  noz::Application::Frame();
}

@end


void Application::Run(void) {
}
