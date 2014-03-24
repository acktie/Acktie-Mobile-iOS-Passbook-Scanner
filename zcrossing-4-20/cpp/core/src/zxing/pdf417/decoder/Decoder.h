#ifndef __DECOCER_PDF_H__
#define __DECOCER_PDF_H__

/*
 *  Decoder.h
 *  zxing
 *
 *  Created by Hartmut Neubauer, 2012-05-25
 *  Copyright 2010,2012 ZXing authors All rights reserved.
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

#include <zxing/common/reedsolomon/ReedSolomonDecoder.h>
#include <zxing/common/reedsolomon/GenericGF.h>
#include <zxing/common/Counted.h>
#include <zxing/common/Array.h>
#include <zxing/common/DecoderResult.h>
#include <zxing/common/BitMatrix.h>


namespace zxing {
namespace pdf417 {

/**
 * <p>The main class which implements PDF417 Code decoding -- as
 * opposed to locating and extracting the PDF417 Code from an image.</p>
 *
 * <p> 2012-06-27 HFN Reed-Solomon error correction activated, see class PDF417RSDecoder. </p>
 */

class Decoder {
private:
  static const int MAX_ERRORS;
  static const int MAX_EC_CODEWORDS;

  int correctErrors(ArrayRef<int> codewords, 
                    ArrayRef<int> erasures, int numECCodewords);
  static void verifyCodewordCount(ArrayRef<int> codewords, int numECCodewords);

public:
  Decoder();

  Ref<DecoderResult> decode(Ref<BitMatrix> bits);
};

} /* namespace pdf417 */
} /* namespace zxing */

#endif // __DECOCER_PDF_H__
