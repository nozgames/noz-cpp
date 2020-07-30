///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#import "OSXApplication.h"

int main(int argc, char *argv[]) {
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];

  AppDelegate *appDelegate = [[AppDelegate alloc] init];
  [NSApp setDelegate:appDelegate];
  [NSApp run];
  [pool release];
}
