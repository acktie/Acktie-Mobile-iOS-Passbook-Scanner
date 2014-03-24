/*
 *  PDF417Reader.cpp
 *  zxing
 *
 *  Created by Hartmut Neubauer on 2010-05-21.
 *  Copyright 2010 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <zxing/pdf417/PDF417Reader.h>
#include <zxing/pdf417/detector/Detector.h>
#include <iostream>

using zxing::pdf417::PDF417Reader;
using zxing::Ref;
using zxing::ArrayRef;
using zxing::Result;
using zxing::BitMatrix;
using zxing::pdf417::Decoder;



PDF417Reader::PDF417Reader() :
    decoder_() {
}

Ref<Result> PDF417Reader::decode(Ref<BinaryBitmap> image, DecodeHints hints)
{
    Ref<DecoderResult> decoderResult;
	/* 2012-05-30 hfn C++ DecodeHintType does not yet know a type "PURE_BARCODE", */
	/* therefore skip this for now, todo: may be add this type later */
		/*
		if (!hints.isEmpty() && hints.containsKey(DecodeHintType.PURE_BARCODE)) {
		  BitMatrix bits = extractPureBits(image.getBlackMatrix());
		  decoderResult = decoder.decode(bits);
		  points = NO_POINTS;
		} else {
		*/
	Detector detector(image);
	Ref<DetectorResult> detectorResult = detector.detect();
#if (defined _WIN32 && defined DEBUG)
	{
		WCHAR sz[256];
		wsprintf(sz,L"PDF417Reader::decode: detected, %d\n",detectorResult->getPoints()[0]);
		OutputDebugString(sz);
	}
#endif
	ArrayRef< Ref<ResultPoint> >  points(detectorResult->getPoints());

	if (!hints.isEmpty()) {
        Ref<ResultPointCallback> rpcb = hints.getResultPointCallback();
            /* .get(DecodeHintType.NEED_RESULT_POINT_CALLBACK); */
        if (rpcb != NULL) {
          for (int i = 0; i < (int)points->size(); i++) {
            rpcb->foundPossibleResultPoint(*(points[i]));
          }
        }
      }
	decoderResult = decoder_.decode(detectorResult->getBits());
		/*
		}
		*/
	Ref<Result> r(new Result(decoderResult->getText(), decoderResult->getRawBytes(), points,
                             BarcodeFormat::PDF_417));
#if (defined _WIN32 && defined(DEBUG))
	  {
		  WCHAR sz[1024];
		  wsprintf(sz,L"PDF417Reader::decode: \"%S\"\n",decoderResult->getText().object_->getText().c_str());
		  OutputDebugString(sz);
	  }
#endif
    fprintf( stdout, "PDF417Reader::decode: \"%s\"\n", decoderResult->getText().object_->getText().c_str());
    return r;
}
		
PDF417Reader::~PDF417Reader() {
}

Decoder& PDF417Reader::getDecoder() {
	return decoder_;
}

void PDF417Reader::reset() { /* do nothing */
}

Ref<BitMatrix> PDF417Reader::extractPureBits(Ref<BitMatrix> image)
{
    ArrayRef<int> leftTopBlack = image->getTopLeftOnBit();
    ArrayRef<int> rightBottomBlack = image->getBottomRightOnBit();
	/* see BitMatrix::getTopLeftOnBit etc.:
    if (leftTopBlack == null || rightBottomBlack == null) {
      throw NotFoundException.getNotFoundInstance();
    } */

    int nModuleSize = moduleSize(leftTopBlack, image);

    int top = leftTopBlack[1];
    int bottom = rightBottomBlack[1];
    int left = findPatternStart(leftTopBlack[0], top, image);
    int right = findPatternEnd(leftTopBlack[0], top, image);

    int matrixWidth = (right - left + 1) / nModuleSize;
    int matrixHeight = (bottom - top + 1) / nModuleSize;
    if (matrixWidth <= 0 || matrixHeight <= 0) {
      throw NotFoundException("PDF417Reader::extractPureBits: no matrix found!");
    }

    // Push in the "border" by half the module width so that we start
    // sampling in the middle of the module. Just in case the image is a
    // little off, this will help recover.
    int nudge = nModuleSize >> 1;
    top += nudge;
    left += nudge;

    // Now just read off the bits
    Ref<BitMatrix> bits(new BitMatrix(matrixWidth, matrixHeight));
    for (int y = 0; y < matrixHeight; y++) {
      int iOffset = top + y * nModuleSize;
      for (int x = 0; x < matrixWidth; x++) {
        if (image->get(left + x * nModuleSize, iOffset)) {
          bits->set(x, y);
        }
      }
    }
    return bits;
}

int PDF417Reader::moduleSize(ArrayRef<int> leftTopBlack, Ref<BitMatrix> image)
{
    int x = leftTopBlack[0];
    int y = leftTopBlack[1];
    int width = image->getWidth();
    while (x < width && image->get(x, y)) {
      x++;
    }
    if (x == width) {
      throw NotFoundException("PDF417Reader::moduleSize: not found!");
    }

    int nModuleSize = (int)(((unsigned)(x - leftTopBlack[0])) >> 3); // We've crossed left first bar, which is 8x
    if (nModuleSize == 0) {
      throw NotFoundException("PDF417Reader::moduleSize: is zero!");
    }

    return nModuleSize;
}

int PDF417Reader::findPatternStart(int x, int y, Ref<BitMatrix> image)
{
    int width = image->getWidth();
    int start = x;
    // start should be on black
    int transitions = 0;
    bool black = true;
    while (start < width - 1 && transitions < 8) {
      start++;
      bool newBlack = image->get(start, y);
      if (black != newBlack) {
        transitions++;
      }
      black = newBlack;
    }
    if (start == width - 1) {
      throw NotFoundException("PDF417Reader::findPatternStart: no pattern start found!");
    }
    return start;
}

int PDF417Reader::findPatternEnd(int x, int y, Ref<BitMatrix> image)
{
    int width = image->getWidth();
    int end = width - 1;
    // end should be on black
    while (end > x && !image->get(end, y)) {
      end--;
    }
    int transitions = 0;
    bool black = true;
    while (end > x && transitions < 9) {
      end--;
      bool newBlack = image->get(end, y);
      if (black != newBlack) {
        transitions++;
      }
      black = newBlack;
    }
    if (end == x) {
      throw NotFoundException("PDF417Reader::findPatternEnd: no pattern end found!");
    }
    return end;
}	