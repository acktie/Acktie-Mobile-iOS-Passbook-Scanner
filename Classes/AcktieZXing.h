//
//  AcktieZXing.h
//  acktie mobile ios passbook scanner
//
//  Created by Tony Nuzzi on 4/7/13.
//
//

#import <Foundation/Foundation.h>
#import "ZXingWidgetController.h"
#import "TiModule.h"
#import "TiUIViewProxy.h"

@interface AcktieZXing : NSObject <UINavigationControllerDelegate, ZXingDelegate>
{
    NSTimer *continuousScanTimer;
}
@property(readwrite,retain) ZXingWidgetController *scanner;

@property(readwrite,retain) KrollCallback *successCallback;
@property(readwrite,retain) KrollCallback *errorCallback;
@property(readwrite,retain) KrollCallback *cancelCallback;

@property(readwrite,retain) TiViewProxy* proxy;
@property(readwrite,assign) BOOL scanHorizontal;
@property(readwrite,assign) BOOL enableQRScanning;
@property(readwrite,assign) BOOL enablePDF417Scanning;
@property(readwrite,assign) BOOL enableAztecScanning;
@property(readwrite,assign) double scanInterval;
@property(readwrite,assign) BOOL useFrontCamera;

- (void) initScanner: (TiUIView*) view;
- (id)init: (NSDictionary*) withDictionary;
- (void) turnLightOff;
- (void) turnLightOn;
- (void) stop;

@end
