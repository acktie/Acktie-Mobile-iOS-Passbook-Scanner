//
//  AcktieZXing.m
//  acktie mobile ios passbook scanner
//
//  Created by Tony Nuzzi on 4/7/13.
//
//

#import "AcktieZXing.h"
#import "QRCodeReader.h"
#import "AztecReader.h"
#import "PDF417Reader.h"
#import <MultiFormatReader.h>



@implementation AcktieZXing
@synthesize scanner;
@synthesize successCallback;
@synthesize cancelCallback;
@synthesize errorCallback;
@synthesize proxy;
@synthesize scanHorizontal;
@synthesize enableQRScanning, enablePDF417Scanning, enableAztecScanning;
@synthesize scanInterval;
@synthesize useFrontCamera;

-(id)init
{
    NSLog(@"Inside -(id)init");
    self = [super init];
    if(self)
    {

    }
    
    return self;
}

-(id)init: (NSDictionary*) withDictionary
{
    NSLog(@"Inside -(id)init: (NSDictionary*) withDictionary");
    self = [self init];
    
    if(self)
    {
        [self setCallbacks:withDictionary];
        [self setScanHorizontalArg:withDictionary];
        [self setEnableQRScanningArg:withDictionary];
        [self setEnablePDF417ScanningArg:withDictionary];
        [self setEnableAztecScanningArg:withDictionary];
        [self setScanIntervalArg:withDictionary];
        [self setUseFrontCameraArg:withDictionary];
    }
    
    NSLog(@"AcktieZXing init: %@", self);
    return self;
}

-(NSString*)moduleId
{
	return @"com.acktie.mobile.ios.passbook.scanner";
}

- (void) initScanner: (TiUIView*) view
{
     NSLog(@"Inside - (void) initScanner");
    
    if(scanner != nil)
    {
        [scanner release];
        scanner = nil;
    }
    
    //scanner = [[ZXingWidgetController alloc] initWithDelegate:self frame:view.frame showCancel:NO OneDMode:NO showLicense:NO scanHorizontal:scanHorizontal];
    
    // scanner = [[ZXingWidgetController alloc] initWithDelegate:self showCancel:NO OneDMode:NO showLicense:NO];
    
    scanner = [[ZXingWidgetController alloc] initWithDelegate:self showCancel:NO OneDMode:NO showLicense:NO useFrontCamera:useFrontCamera];
    
    NSMutableSet *readers = [[NSMutableSet alloc ] init];
    
//    if(enableQRScanning)
//    {
//        QRCodeReader* qrcodeReader = [[QRCodeReader alloc] init];
//        [readers addObject:qrcodeReader];
//        [qrcodeReader release];
//    }
//    
//    if(enablePDF417Scanning)
//    {
//        PDF417Reader *pdf417Reader = [[PDF417Reader alloc] init];
//        [readers addObject:pdf417Reader];
//        [pdf417Reader release];
//    }
//    
//    if(enableAztecScanning)
//    {
//        AztecReader *aztecReader = [[AztecReader alloc] init];
//        [readers addObject:aztecReader];
//        [aztecReader release];
//    }
    
    MultiFormatReader* mfr = [[MultiFormatReader alloc] init];
    [readers addObject:mfr];
    [mfr release];
    
    scanner.readers = readers;
    [readers release];

}

- (void) setCallbacks: (NSDictionary*)args
{
    // callbacks
    if ([args objectForKey:@"success"] != nil)
    {
        NSLog(@"Received success callback");
        
        successCallback = [args objectForKey:@"success"];
        [successCallback retain];
    }
    
    if ([args objectForKey:@"error"] != nil)
    {
        NSLog(@"Received error callback");
        
        errorCallback = [args objectForKey:@"error"];
        [errorCallback retain];
    }
    
    if ([args objectForKey:@"cancel"] != nil)
    {
        NSLog(@"Received cancel callback");
        
        cancelCallback = [args objectForKey:@"cancel"];
        [cancelCallback retain];
    }
}

- (void) setScanHorizontalArg:(NSDictionary*)args
{
    if([args objectForKey:@"scanHorizontal"] != nil)
    {
        scanHorizontal = [TiUtils boolValue:[args objectForKey:@"scanHorizontal"]];
    }
    else
    {
        scanHorizontal = NO;
    }
    
    NSLog(@"scanHorizontal set: %d", scanHorizontal);

}

- (void) setEnableQRScanningArg:(NSDictionary*)args
{
    if([args objectForKey:@"enableQR"] != nil)
    {
        enableQRScanning = [TiUtils boolValue:[args objectForKey:@"enableQR"]];
    }
    else
    {
        enableQRScanning = YES;
    }
    
    NSLog(@"enableQRScanning set: %d", enableQRScanning);
}

- (void) setEnablePDF417ScanningArg:(NSDictionary*)args
{
    if([args objectForKey:@"enablePDF417"] != nil)
    {
        enablePDF417Scanning = [TiUtils boolValue:[args objectForKey:@"enablePDF417"]];
    }
    else
    {
        enablePDF417Scanning = YES;
    }
    
    NSLog(@"enablePDF417Scanning set: %d", enablePDF417Scanning);
}

- (void) setEnableAztecScanningArg:(NSDictionary*)args
{
    if([args objectForKey:@"enableAztec"] != nil)
    {
        enableAztecScanning = [TiUtils boolValue:[args objectForKey:@"enableAztec"]];
    }
    else
    {
        enableAztecScanning = YES;
    }
    
    NSLog(@"enableAztecScanning set: %d", enableAztecScanning);
}

- (void) setScanIntervalArg:(NSDictionary*)args
{
    if([args objectForKey:@"scanInterval"] != nil)
    {
        scanInterval = [TiUtils doubleValue:[args objectForKey:@"scanInterval"]];
    }
    else
    {
        scanInterval = 3.0;
    }
    
    NSLog(@"scanInterval set: %f", scanInterval);
}

- (void) setUseFrontCameraArg:(NSDictionary*)args
{
    if([args objectForKey:@"useFrontCamera"] != nil)
    {
        useFrontCamera = [TiUtils boolValue:[args objectForKey:@"useFrontCamera"]];
    }
    else
    {
        useFrontCamera = NO;
    }
    
    NSLog(@"useFrontCamera set: %d", useFrontCamera);
}

- (void) turnLightOff
{
    [scanner setTorch:NO];
}

- (void) turnLightOn
{
    [scanner setTorch:YES];
}

- (void) stop
{
    NSLog(@"Inside stop");
    
    if(continuousScanTimer != nil && [continuousScanTimer isValid])
    {
        NSLog(@"Invalidating continuousScanTimer");
        [continuousScanTimer invalidate];
        continuousScanTimer = nil;
    }
    
    [scanner cancelled];
}

- (void) startDecoding
{
    if(scanner != nil)
    {
        [scanner setDecoding:YES];
    }
    
    [continuousScanTimer invalidate];
    continuousScanTimer = nil;
}

- (void)zxingController:(ZXingWidgetController*)controller didScanResult:(NSString *)result {
    NSLog(@"- (void)zxingController:(ZXingWidgetController*)controller didScanResult:(NSString *)result");
    
    if(successCallback != nil)
    {
        id listener = [[successCallback retain] autorelease];
    
        // Populate Callback data
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
    
    
        [dictionary setObject:result forKey:@"data"];
        [proxy _fireEventToListener:@"success" withObject:dictionary listener:listener thisObject:nil];
    }
    
    continuousScanTimer = [NSTimer scheduledTimerWithTimeInterval:scanInterval target:self selector:@selector(startDecoding) userInfo:nil repeats:NO];
}

- (void)zxingControllerDidCancel:(ZXingWidgetController*)controller {
    NSLog(@"- (void)zxingControllerDidCancel:(ZXingWidgetController*)controller");
    
    if(cancelCallback != nil)
    {
        id listener = [[cancelCallback retain] autorelease];
        
        // Populate Callback data
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        
        
        [proxy _fireEventToListener:@"cancel" withObject:nil listener:listener thisObject:nil];
    }
}

@end
