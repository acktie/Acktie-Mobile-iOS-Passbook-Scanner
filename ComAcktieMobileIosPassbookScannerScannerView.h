//
//  ComAcktieMobileIosQrView.h
//  acktie mobile qr ios
//
//  Created by Tony Nuzzi on 3/3/13.
//
//

#import "TiUIView.h"
#import "AcktieZXing.h"

@interface ComAcktieMobileIosPassbookScannerScannerView : TiUIView
{
    @protected ZXingWidgetController *scanner;
}
@property(readwrite,retain) AcktieZXing *zXing;

- (void) turnLightOff:(id)args;
- (void) turnLightOn:(id)args;
- (void) stop:(id)args;

@end
