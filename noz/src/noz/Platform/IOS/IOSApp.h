//
//  AppDelegate.h
//  ev_player
//
//  Created by Bryan Dube on 7/5/13.
//  Copyright (c) 2013 Radius Software. All rights reserved.
//

#import <UIKit/UIKit.h>

namespace noz { class Window; }

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (nonatomic) noz::Window* noz_window_;

@end
