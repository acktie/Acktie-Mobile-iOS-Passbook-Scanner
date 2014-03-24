//
//  ComAcktieMobileIosQrView.m
//  acktie mobile qr ios
//
//  Created by Tony Nuzzi on 3/3/13.
//
//

#import "ComAcktieMobileIosPassbookScannerScannerView.h"
#import "ComAcktieMobileIosPassbookScannerScannerViewProxy.h"

@class ComAcktieMobileIosPassbookScannerModule;

@implementation ComAcktieMobileIosPassbookScannerScannerView
@synthesize zXing;

-(void)dealloc
{
    NSLog(@"Inside view dealloc");
    NSLog(@"self.zXing: %@", self.zXing);
    
    if(self.zXing != nil)
    {
         NSLog(@"Shutting down Scanner!");
        [[self zXing] stop];
    }
    
    RELEASE_TO_NIL(self.zXing);
    
    if(scanner != nil)
    {
        [scanner dismissModalViewControllerAnimated:NO];
        [scanner viewWillDisappear:NO];
        [scanner viewDidDisappear:NO];
        RELEASE_TO_NIL(scanner);
    }
    
    [super dealloc];
}

-(id) init
{
    NSLog(@"(View) -(id) init");
    
    self = [super init];
    
    return self;
}

-(void)scanner
{
    NSLog(@"(View) -(void)scanner");
    if (scanner == nil)
    {
        
        NSLog(@"(View) if (scanner == nil)");
        AcktieZXing* aZXing = [self getZXing];
        [aZXing initScanner:self];
        scanner = aZXing.scanner;
        
        [self addSubview:aZXing.scanner.view];
    }
}

-(void)frameSizeChanged:(CGRect)frame bounds:(CGRect)bounds
{
    NSLog(@"Inside frameSizeChanged");
    if (scanner == nil)
    {
        NSLog(@"Inside frameSizeChanged : scanner == nil");
        [self scanner];
        
        [TiUtils setView:scanner.view positionRect:bounds];
        [scanner viewDidLoad];
        [scanner viewWillAppear:YES];
        [scanner viewDidAppear:YES];
    }
    else
    {
        [TiUtils setView:scanner.view positionRect:bounds];
    }
}

-(AcktieZXing*) getZXing
{
    NSLog(@"Inside getZXing");
    
    if (zXing == nil) {
        ComAcktieMobileIosPassbookScannerScannerViewProxy* viewProxy = (ComAcktieMobileIosPassbookScannerScannerViewProxy*)[self proxy];
        self.zXing = [viewProxy zXing];
    }
    
    return self.zXing;
    
}

- (void) turnLightOff:(id)args
{
    NSLog(@"turnLightOff in view");
    AcktieZXing* aZXing = [self getZXing];
    [aZXing turnLightOff];
}

- (void) turnLightOn:(id)args
{
    NSLog(@"turnLightOn in view");
    AcktieZXing* aZXing = [self getZXing];
    [aZXing turnLightOn];
}

- (void) stop:(id)args
{
    NSLog(@"stop in view");
    AcktieZXing* aZXing = [self getZXing];
    [aZXing stop];
}
@end


