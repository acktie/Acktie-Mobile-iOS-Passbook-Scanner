//
//  ComAcktieMobileIosQrViewProxy.h
//  acktie mobile qr ios
//
//  Created by Tony Nuzzi on 3/3/13.
//
//

#import "TiViewProxy.h"
#import "AcktieZXing.h"

@interface ComAcktieMobileIosPassbookScannerScannerViewProxy : TiViewProxy
{
}

#pragma mark public API
@property(readwrite,retain) KrollCallback *successCallback;
@property(readwrite,retain) AcktieZXing* zXing;

#pragma Public APIs (available in javascript)
- (void) turnLightOff:(id)args;
- (void) turnLightOn:(id)args;
- (void) stop:(id)args;
@end
