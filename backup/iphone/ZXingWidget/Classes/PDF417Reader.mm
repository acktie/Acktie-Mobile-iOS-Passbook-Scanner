//
//  PDR417Reader.m
//  ZXingWidget
//
//  Created by Adam Freeman on 09.03.13.
//  Copyright (c) 2012 EOS UPTRADE GmbH. All rights reserved.
//

#import "PDF417Reader.h"
#import <zxing/PDF417/PDF417Reader.h>

@implementation PDF417Reader

- (id)init {
    zxing::pdf417::PDF417Reader *reader = new zxing::pdf417::PDF417Reader();
    
    return [super initWithReader:reader];
}

- (zxing::Ref<zxing::Result>)decode:(zxing::Ref<zxing::BinaryBitmap>)grayImage andCallback:(zxing::Ref<zxing::ResultPointCallback>)callback {
    //NSLog(@"no callbacks supported for PDF417");
    return [self decode:grayImage];
}

@end
