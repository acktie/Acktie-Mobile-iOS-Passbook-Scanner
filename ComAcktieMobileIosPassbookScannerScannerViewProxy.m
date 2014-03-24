//
//  ComAcktieMobileIosQrViewProxy.m
//  acktie mobile qr ios
//
//  Created by Tony Nuzzi on 3/3/13.
//
//

#import "ComAcktieMobileIosPassbookScannerScannerViewProxy.h"
#import "AcktieZXing.h"

@class AcktieZXing;
@implementation ComAcktieMobileIosPassbookScannerScannerViewProxy

@synthesize successCallback;

-(void)_initWithProperties:(NSDictionary *)args
{
    NSLog(@"Inside _initWithProperties");
    
	[super _initWithProperties:args];
    AcktieZXing* localAZxing = [[[AcktieZXing alloc] init:args] autorelease];
    [localAZxing setProxy:self];
    
    [self setZXing:localAZxing];
    
}

- (void) turnLightOff:(id)args
{
    NSLog(@"turnLightOff in proxy");
    [[self view] performSelectorOnMainThread:@selector(turnLightOff:)
                                  withObject:args waitUntilDone:YES];
}

- (void) turnLightOn:(id)args
{
    NSLog(@"turnLightOn in proxy");
    [[self view] performSelectorOnMainThread:@selector(turnLightOn:)
                                  withObject:args waitUntilDone:YES];
}

- (void) stop:(id)args
{
    NSLog(@"stop in proxy");
    [[self view] performSelectorOnMainThread:@selector(stop:)
                                  withObject:args waitUntilDone:YES];
}

@end
