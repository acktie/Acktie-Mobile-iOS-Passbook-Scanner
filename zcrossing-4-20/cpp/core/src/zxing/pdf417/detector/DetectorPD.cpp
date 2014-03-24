/*
 *  Detector.cpp
 *  zxing
 *
 *  Created by Hartmut Neubauer 2012-05-25
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

#include <zxing/ResultPoint.h>
#include <zxing/common/GridSampler.h>
#include <zxing/pdf417/detector/Detector.h>
#include <zxing/common/detector/MathUtils.h>
#include <sstream>
#include <cstdlib>

/**
 * <p>Encapsulates logic that can detect a PDF417 Code in an image, even if the
 * PDF417 Code is rotated or skewed, or partially obscured.</p>
 *
 * @author SITA Lab (kevin.osullivan@sita.aero)
 * @author dswitkin@google.com (Daniel Switkin)
 */

namespace zxing {
namespace pdf417 {

const int Detector::MAX_AVG_VARIANCE = (int) ((1 << 8) * 0.42f);
const int Detector::MAX_INDIVIDUAL_VARIANCE = (int) ((1 << 8) * 0.8f);
const int Detector::SKEW_THRESHOLD = 2;

  // B S B S B S B S Bar/Space pattern
  // 11111111 0 1 0 1 0 1 000
const int Detector::START_PATTERN_[] = {8, 1, 1, 1, 1, 1, 1, 3};

  // 11111111 0 1 0 1 0 1 000
const int Detector::START_PATTERN_REVERSE_[] = {3, 1, 1, 1, 1, 1, 1, 8};

  // 1111111 0 1 000 1 0 1 00 1
const int Detector::STOP_PATTERN_[] = {7, 1, 1, 3, 1, 1, 1, 2, 1};

  // B S B S B S B S B Bar/Space pattern
  // 1111111 0 1 000 1 0 1 00 1
const int Detector::STOP_PATTERN_REVERSE_[] = {1, 2, 1, 1, 1, 3, 1, 1, 7};

const int Detector::SIZEOF_START_PATTERN_ = sizeof(START_PATTERN_) / sizeof(int);
const int Detector::SIZEOF_START_PATTERN_REVERSE_ = sizeof(START_PATTERN_REVERSE_) / sizeof(int);
const int Detector::SIZEOF_STOP_PATTERN_ = sizeof(STOP_PATTERN_) / sizeof(int);
const int Detector::SIZEOF_STOP_PATTERN_REVERSE_ = sizeof(STOP_PATTERN_REVERSE_) / sizeof(int);
Ref<PerspectiveTransform> Detector::transform_;
//* todo: right so or this element and some methods nonstatic?


Detector::Detector(Ref<BinaryBitmap> image) : image_(image)
{
}

Ref<BinaryBitmap> Detector::getImage() {
   return image_;
}
    
Ref<DetectorResult> Detector::detect()
{
  DecodeHints hi;
  return detect(hi);
}

// #pragma warning( push )
// #pragma warning( disable : 4101 )
  /**
   * <p>Detects a PDF417 Code in an image. Only checks 0 and 180 degree rotations.</p>
   *
   * @param hints optional hints to detector (2012-05-25 hfn: obviously ignored ... ;-/ )
   * @return {@link DetectorResult} encapsulating results of detecting a PDF417 Code
   * @throws NotFoundException if no PDF417 Code can be found
   */
Ref<DetectorResult> Detector::detect(DecodeHints const& hints)
{
    // Fetch the 1 bit matrix once up front.
    Ref<BitMatrix> matrix = image_->getBlackMatrix();
	std::vector<Ref<ResultPoint> > vertices;

    // Try to find the vertices assuming the image is upright.
	// 2012-05-25 hfn because there seem difficulties in c++ to convert
	// these types into NULL pointer, work with exceptions at this point
	// (including the find... methods below
	try {
      vertices = findVertices(matrix);
	  if (vertices.size() < 8) {
		vertices = findVertices180(matrix);
		if (vertices.size() >= 8) {
		  correctCodeWordVertices(vertices, true);
		}
	  }
	  else {
		correctCodeWordVertices(vertices, false);
	  }
	}
	catch(NotFoundException const &re) {
	  throw(re);
	}

	if(vertices.size() < 8) {
	  throw NotFoundException("No vertices found!");
	}
	  

	    
	/*
	 * Note that the following "try-catch" version does produce errors in eMbedded visual c++.
	 * 2012-06-04 hfn
	 *
	try {
      vertices = findVertices(matrix);
	  correctCodeWordVertices(vertices, FALSE);
	}
	catch(NotFoundException const &re) {
	  try {
		vertices = findVertices180(matrix);
		correctCodeWordVertices(vertices, TRUE);
	  }
	  catch(NotFoundException const &re) {
		throw(re);
	  }
	}
	*/



    float moduleWidth = computeModuleWidth(vertices);
    if (moduleWidth < 1.0f) {
      throw NotFoundException("PDF:Detector: Bad Module width!");
    }

	ArrayRef< Ref<ResultPoint> > points(4);

	Ref<ResultPoint>rp(new ResultPoint(float(0.0),float(1.0)));
	points[0].reset(new ResultPoint(vertices[5]->getX(),vertices[5]->getY()));
	points[1].reset(new ResultPoint(vertices[4]->getX(),vertices[4]->getY()));
	points[2].reset(new ResultPoint(vertices[6]->getX(),vertices[6]->getY()));
	points[3].reset(new ResultPoint(vertices[7]->getX(),vertices[7]->getY()));
	/* following seems to be difficult: points[0].reset(Ref<ResultPoint>(vertices[5])); etc. */

    int dimension = computeDimension(points[1], points[2],
        points[0], points[3], moduleWidth);
    if (dimension < 1) {
      throw NotFoundException("PDF:Detector: Bad dimension!");
    }

    // Deskew and sample image.
    Ref<BitMatrix> bits(sampleGrid(matrix, points[1], points[0],
        points[2], points[3], dimension));
    
	Ref<DetectorResult> dr(new DetectorResult(bits, points));
    return dr;
}

  /**
   * Locate the vertices and the codewords area of a black blob using the Start
   * and Stop patterns as locators.
   * TODO: Scanning every row is very expensive. We should only do this for TRY_HARDER.
   *
   * @param matrix the scanned barcode image.
   * @return an array containing the vertices:
   *           vertices[0] x, y top left barcode
   *           vertices[1] x, y bottom left barcode
   *           vertices[2] x, y top right barcode
   *           vertices[3] x, y bottom right barcode
   *           vertices[4] x, y top left codeword area
   *           vertices[5] x, y bottom left codeword area
   *           vertices[6] x, y top right codeword area
   *           vertices[7] x, y bottom right codeword area
   */
std::vector<Ref<ResultPoint> > Detector::findVertices(Ref<BitMatrix> matrix)
{
    int height = matrix->getHeight();
    int width = matrix->getWidth();

    std::vector<Ref<ResultPoint> > result (8);
    bool found = false;

    ArrayRef<int> counters(new Array<int>(SIZEOF_START_PATTERN_));

    // Top Left
    for (int i = 0; i < height; i++) {
	  try {
		ArrayRef<int> loc = findGuardPattern(matrix, 0, i, width, false, START_PATTERN_,
			SIZEOF_START_PATTERN_,counters);
		if (loc->size() >= 2) {
			result[0].reset(new ResultPoint(loc[0], i));
			result[4].reset (new ResultPoint(loc[1], i));
			found = true;
			break;
		}
      }
	  catch(NotFoundException &re) {
		/* next i */
	  }
    }
    // Bottom left
    if (found) { // Found the Top Left vertex
      found = false;
      for (int i = height - 1; i > 0; i--) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, 0, i, width, false, START_PATTERN_,
			  SIZEOF_START_PATTERN_,counters);
		  if (loc->size() >= 2) {
			  result[1].reset (new ResultPoint(loc[0], i));
			  result[5].reset (new ResultPoint(loc[1], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }

    counters = new Array<int>(SIZEOF_STOP_PATTERN_);

    // Top right
    if (found) { // Found the Bottom Left vertex
      found = false;
      for (int i = 0; i < height; i++) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, 0, i, width, false, STOP_PATTERN_,
			  SIZEOF_STOP_PATTERN_,counters);
		  if (loc->size() >= 2) {
			  result[2].reset (new ResultPoint(loc[1], i));
			  result[6].reset (new ResultPoint(loc[0], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }
    // Bottom right
    if (found) { // Found the Top right vertex
      found = false;
      for (int i = height - 1; i > 0; i--) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, 0, i, width, false, STOP_PATTERN_,
			  SIZEOF_STOP_PATTERN_,counters);
		  if (loc->size() >= 2) {
			  result[3].reset (new ResultPoint(loc[1], i));
			  result[7].reset (new ResultPoint(loc[0], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }
	if (!found) {
//	  throw NotFoundException("PDF:Detector:findVertices: nothing found!");
       std::vector<Ref<ResultPoint> > dummy (1);
	   return dummy;

	}
    return result;
}

  /**
   * Locate the vertices and the codewords area of a black blob using the Start
   * and Stop patterns as locators. This assumes that the image is rotated 180
   * degrees and if it locates the start and stop patterns at it will re-map
   * the vertices for a 0 degree rotation.
   * TODO: Change assumption about barcode location.
   * TODO: Scanning every row is very expensive. We should only do this for TRY_HARDER.
   *
   * @param matrix the scanned barcode image.
   * @return an array containing the vertices:
   *           vertices[0] x, y top left barcode
   *           vertices[1] x, y bottom left barcode
   *           vertices[2] x, y top right barcode
   *           vertices[3] x, y bottom right barcode
   *           vertices[4] x, y top left codeword area
   *           vertices[5] x, y bottom left codeword area
   *           vertices[6] x, y top right codeword area
   *           vertices[7] x, y bottom right codeword area
   */
std::vector<Ref<ResultPoint> > Detector::findVertices180(Ref<BitMatrix> matrix)
{
    int height = matrix->getHeight();
    int width = matrix->getWidth();
    int halfWidth = width >> 1;

    std::vector<Ref<ResultPoint> > result (8);
    bool found = false;

    ArrayRef<int> counters = new Array<int>(SIZEOF_START_PATTERN_REVERSE_);
    
    // Top Left
    for (int i = height - 1; i > 0; i--) {
	  try {
		ArrayRef<int> loc = findGuardPattern(matrix, halfWidth, i, halfWidth, true,
			START_PATTERN_REVERSE_, SIZEOF_START_PATTERN_REVERSE_,counters);
		if (loc->size() >= 2) {
			result[0].reset (new ResultPoint(loc[1], i));
			result[4].reset (new ResultPoint(loc[0], i));
			found = true;
			break;
		}
      }
	  catch(NotFoundException &re) {
		/* next i */
	  }
    }
    // Bottom Left
    if (found) { // Found the Top Left vertex
      found = false;
      for (int i = 0; i < height; i++) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, halfWidth, i, halfWidth, true,
			  START_PATTERN_REVERSE_, SIZEOF_START_PATTERN_REVERSE_,counters);
		  if (loc->size() >= 2) {
			  result[1].reset (new ResultPoint(loc[1], i));
			  result[5].reset (new ResultPoint(loc[0], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }
    
    counters = new Array<int>(SIZEOF_STOP_PATTERN_REVERSE_);

    // Top Right
    if (found) { // Found the Bottom Left vertex
      found = false;
      for (int i = height - 1; i > 0; i--) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, 0, i, halfWidth, false,
			  STOP_PATTERN_REVERSE_, SIZEOF_STOP_PATTERN_REVERSE_, counters);
		  if(loc->size()>=2) {
			  result[2].reset (new ResultPoint(loc[0], i));
			  result[6].reset (new ResultPoint(loc[1], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }
    // Bottom Right
    if (found) { // Found the Top Right vertex
      found = false;
      for (int i = 0; i < height; i++) {
		try {
          ArrayRef<int> loc = findGuardPattern(matrix, 0, i, halfWidth, false,
			  STOP_PATTERN_REVERSE_, SIZEOF_STOP_PATTERN_REVERSE_, counters);
		  if (loc->size() >= 2) {
			  result[3].reset (new ResultPoint(loc[0], i));
			  result[7].reset (new ResultPoint(loc[1], i));
			  found = true;
			  break;
		  }
        }
		catch(NotFoundException &re) {
		  /* next i */
		}
      }
    }
	if (!found) {
//	  throw NotFoundException("PDF:Detector:findVertices180: nothing found!");
       std::vector<Ref<ResultPoint> > dummy (1);
	   return dummy;
	}
    return result;
}

// #pragma warning(pop)

  /**
   * Because we scan horizontally to detect the start and stop patterns, the vertical component of
   * the codeword coordinates will be slightly wrong if there is any skew or rotation in the image.
   * This method moves those points back onto the edges of the theoretically perfect bounding
   * quadrilateral if needed.
   *
   * @param vertices The eight vertices located by findVertices().
   */
void Detector::correctCodeWordVertices(std::vector<Ref<ResultPoint> > &vertices,
			bool upsideDown)
{
    float skew = vertices[4]->getY() - vertices[6]->getY();
    if (upsideDown) {
      skew = -skew;
    }
    if (skew > SKEW_THRESHOLD) {
      // Fix v4
      float length = vertices[4]->getX() - vertices[0]->getX();
      float deltax = vertices[6]->getX() - vertices[0]->getX();
      float deltay = vertices[6]->getY() - vertices[0]->getY();
      float correction = length * deltay / deltax;
      vertices[4].reset(new ResultPoint(vertices[4]->getX(), vertices[4]->getY() + correction));
    } else if (-skew > SKEW_THRESHOLD) {
      // Fix v6
      float length = vertices[2]->getX() - vertices[6]->getX();
      float deltax = vertices[2]->getX() - vertices[4]->getX();
      float deltay = vertices[2]->getY() - vertices[4]->getY();
      float correction = length * deltay / deltax;
      vertices[6].reset(new ResultPoint(vertices[6]->getX(), vertices[6]->getY() - correction));
    }

    skew = vertices[7]->getY() - vertices[5]->getY();
    if (upsideDown) {
      skew = -skew;
    }
    if (skew > SKEW_THRESHOLD) {
      // Fix v5
      float length = vertices[5]->getX() - vertices[1]->getX();
      float deltax = vertices[7]->getX() - vertices[1]->getX();
      float deltay = vertices[7]->getY() - vertices[1]->getY();
      float correction = length * deltay / deltax;
      vertices[5].reset(new ResultPoint(vertices[5]->getX(), vertices[5]->getY() + correction));
    } else if (-skew > SKEW_THRESHOLD) {
      // Fix v7
      float length = vertices[3]->getX() - vertices[7]->getX();
      float deltax = vertices[3]->getX() - vertices[5]->getX();
      float deltay = vertices[3]->getY() - vertices[5]->getY();
      float correction = length * deltay / deltax;
      vertices[7].reset(new ResultPoint(vertices[7]->getX(), vertices[7]->getY() - correction));
    }
}
			
  /**
   * <p>Estimates module size (pixels in a module) based on the Start and End
   * finder patterns.</p>
   *
   * @param vertices an array of vertices:
   *           vertices[0] x, y top left barcode
   *           vertices[1] x, y bottom left barcode
   *           vertices[2] x, y top right barcode
   *           vertices[3] x, y bottom right barcode
   *           vertices[4] x, y top left codeword area
   *           vertices[5] x, y bottom left codeword area
   *           vertices[6] x, y top right codeword area
   *           vertices[7] x, y bottom right codeword area
   * @return the module size.
   */
float Detector::computeModuleWidth(std::vector<Ref<ResultPoint> > &vertices)
{
	/* 2012-05-31 hfn: every attempt to use "Ref<ResultPoint>(vertices[i])" as arguments for the two-argument */
	/* distance method led to an exception in eVc compiler, therefore: ... */
	float pixels1 = ResultPoint::distance(vertices[0]->getX(),vertices[4]->getX(),vertices[0]->getY(),vertices[4]->getY());
    float pixels2 = ResultPoint::distance(vertices[1]->getX(),vertices[5]->getX(),vertices[1]->getY(),vertices[5]->getY());
    float moduleWidth1 = (pixels1 + pixels2) / (17 * 2.0f);
    float pixels3 = ResultPoint::distance(vertices[6]->getX(),vertices[2]->getX(),vertices[6]->getY(),vertices[2]->getY());
    float pixels4 = ResultPoint::distance(vertices[7]->getX(),vertices[3]->getX(),vertices[7]->getY(),vertices[3]->getY());
    float moduleWidth2 = (pixels3 + pixels4) / (18 * 2.0f);
    return (moduleWidth1 + moduleWidth2) / 2.0f;
}

  /**
   * Computes the dimension (number of modules in a row) of the PDF417 Code
   * based on vertices of the codeword area and estimated module size.
   *
   * @param topLeft     of codeword area
   * @param topRight    of codeword area
   * @param bottomLeft  of codeword area
   * @param bottomRight of codeword are
   * @param moduleWidth estimated module size
   * @return the number of modules in a row.
   */
int Detector::computeDimension(Ref<ResultPoint> topLeft,
			  Ref<ResultPoint> topRight,
			  Ref<ResultPoint> bottomLeft,
			  Ref<ResultPoint> bottomRight,
			  float moduleWidth)
{
    int topRowDimension = round(ResultPoint::distance(topLeft, topRight) / moduleWidth);
    int bottomRowDimension = round(ResultPoint::distance(bottomLeft, bottomRight) / moduleWidth);
    return ((((topRowDimension + bottomRowDimension) >> 1) + 8) / 17) * 17;
}

Ref<PerspectiveTransform> Detector::createTransform(Ref<ResultPoint> topLeft,
    Ref<ResultPoint> topRight, Ref<ResultPoint> bottomLeft, Ref<ResultPoint> bottomRight,
    int dimensionX, int dimensionY) {

    // Note that unlike the QR Code sampler, we didn't find the center of modules, but the
    // very corners. So there is no 0.5f here; 0.0f is right.
  Ref<PerspectiveTransform> transform(
      PerspectiveTransform::quadrilateralToQuadrilateral(
	  /*
        0.0f, // p1ToX
        0.0f, // p1ToY
        dimensionX, // p2ToX
        0.0f, // p2ToY
        dimensionX, // p3ToX
        dimensionY, // p3ToY
        0.0f, // p4ToX
        dimensionY, // p4ToY
	   */
#define __DIFF 0.0f// 0.25f 
	    __DIFF,0.0f,dimensionX+__DIFF,0.0f,dimensionX+__DIFF,dimensionY,__DIFF,dimensionY,
        topLeft->getX(), // p1FromX
        topLeft->getY(), // p1FromY
        topRight->getX(), // p2FromX
        topRight->getY(), // p2FromY
        bottomRight->getX(), // p3FromX
        bottomRight->getY(), // p3FromY
        bottomLeft->getX(), // p4FromX
        bottomLeft->getY())); // p4FromY
  return transform;
}
			  
Ref<BitMatrix> Detector::sampleGrid(Ref<BitMatrix> matrix,
			  Ref<ResultPoint> topLeft,
			  Ref<ResultPoint> bottomLeft,
			  Ref<ResultPoint> topRight,
			  Ref<ResultPoint> bottomRight,
			  int dimension)
{
    // Note that unlike the QR Code sampler, we didn't find the center of modules, but the
    // very corners. So there is no 0.5f here; 0.0f is right.
	// 2012-05-31 hfn It seems so that, at this point, the vertical dimension is not yet known,
	// so the horizontal dimension is assumed. At a later stage during decoding, the rows are parsed
	// and the real number of rows will be calculated / estimated.
	int dimensionY = dimension;
    GridSampler sampler = GridSampler::getInstance();
	transform_ = createTransform(topLeft,topRight,bottomLeft,bottomRight,dimension,dimensionY);

#if (0)
	{
		WCHAR *pwc;
		const char *pc=matrix->description();
		size_t len = strlen(pc);
		pwc=new WCHAR[len+1];
		mbstowcs(pwc,pc,len+1);
		OutputDebugString(L"sampleGrid:\n");
		OutputDebugString(pwc);
		delete[] pwc;
	}
#endif

    return sampler.sampleGrid(
        matrix, 
        dimension, dimensionY, transform_);
}
			  
  /**
   * Ends up being a bit faster than Math.round(). This merely rounds its
   * argument to the nearest int, where x.5 rounds up.
   */
int Detector::round(float d)
{
   return (int) (d + 0.5f);
}

  /**
   * @param matrix row of black/white values to search
   * @param column x position to start search
   * @param row y position to start search
   * @param width the number of pixels to search on this row
   * @param pattern pattern of counts of number of black and white pixels that are
   *                 being searched for as a pattern
   * @param counters array of counters, as long as pattern, to re-use 
   * @return start/end horizontal offset of guard pattern, as an array of two ints.
   */
ArrayRef<int> Detector::findGuardPattern(Ref<BitMatrix> matrix,
				int column,
				int row,
				int width,
				bool whiteFirst,
				const int pattern[],
				unsigned int pattern_size,
				ArrayRef<int> counters)
{
	counters->values().assign(counters->size(),0);
    // Arrays.fill(counters, 0, counters.length, 0);
    int patternLength = pattern_size;
    bool isWhite = whiteFirst;
	int i;
    int counterPosition = 0;
    int patternStart = column;
    for (int x = column; x < column + width; x++) {
      bool pixel = matrix->get(x, row);
      if (pixel ^ isWhite) {
        counters[counterPosition]++;
      } else {
        if (counterPosition == patternLength - 1) {
          if (patternMatchVariance(counters, pattern, MAX_INDIVIDUAL_VARIANCE) < MAX_AVG_VARIANCE) {
			ArrayRef<int> newArr = new Array<int>(2);
			newArr[0]=patternStart;
			newArr[1]=x;
			return newArr;
            // in java: return new int[]{patternStart, x};
          }
          patternStart += counters[0] + counters[1];
		  for(i=0;i<patternLength-2;++i)
		    counters[i] = counters[i+2];
          counters[patternLength - 2] = 0;
          counters[patternLength - 1] = 0;
          counterPosition--;
        } else {
          counterPosition++;
        }
        counters[counterPosition] = 1;
        isWhite = !isWhite;
      }
    }
#if (!defined __NEVER____ /* DEBUG */ )
	ArrayRef<int> newArr = new Array<int>(1);
	newArr[0]=0;
	return newArr;
#else
	throw NotFoundException("");
#endif
}
				
  /**
   * Determines how closely a set of observed counts of runs of black/white
   * values matches a given target pattern. This is reported as the ratio of
   * the total variance from the expected pattern proportions across all
   * pattern elements, to the length of the pattern.
   *
   * @param counters observed counters
   * @param pattern expected pattern
   * @param maxIndividualVariance The most any counter can differ before we give up
   * @return ratio of total variance between counters and pattern compared to
   *         total pattern size, where the ratio has been multiplied by 256.
   *         So, 0 means no variance (perfect match); 256 means the total
   *         variance between counters and patterns equals the pattern length,
   *         higher values mean even more variance
   */
int Detector::patternMatchVariance(ArrayRef<int> counters, const int pattern[],
			int maxIndividualVariance)
{
    int numCounters = counters->size();
    int total = 0;
    int patternLength = 0;
    for (int i = 0; i < numCounters; i++) {
      total += counters[i];
      patternLength += pattern[i];
    }
    if (total < patternLength) {
      // If we don't even have one pixel per unit of bar width, assume this
      // is too small to reliably match, so fail:
      return std::numeric_limits<int>::max();
    }
    // We're going to fake floating-point math in integers. We just need to use more bits.
    // Scale up patternLength so that intermediate values below like scaledCounter will have
    // more "significant digits".
    int unitBarWidth = (total << 8) / patternLength;
    maxIndividualVariance = (maxIndividualVariance * unitBarWidth) >> 8;

    int totalVariance = 0;
    for (int x = 0; x < numCounters; x++) {
      int counter = counters[x] << 8;
      int scaledPattern = pattern[x] * unitBarWidth;
      int variance = counter > scaledPattern ? counter - scaledPattern : scaledPattern - counter;
      if (variance > maxIndividualVariance) {
        return std::numeric_limits<int>::max();
      }
      totalVariance += variance;
    }
    return totalVariance / total;
}			

} /* namespace pdf417 */
} /* namespace zxing */