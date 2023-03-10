// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  FinderPatternFinder.cpp
 *  zxing
 *
 *  Created by Christian Brunschen on 13/05/2008.
 *  Copyright 2008 ZXing authors All rights reserved.
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

#include <algorithm>
#include <zxing/qrcode/detector/FinderPatternFinder.h>
#include <zxing/ReaderException.h>
#include <zxing/DecodeHints.h>
#include <cstring>
#include <iostream>

//#include <limits>
//#include <math.h>

using std::abs;
using std::max;
using std::sort;
using std::vector;

using zxing::qrcode::FinderPattern;
using zxing::qrcode::FinderPatternFinder;
using zxing::qrcode::FinderPatternInfo;

// VC++

using zxing::BitMatrix;
using zxing::DecodeHints;
using zxing::ResultPoint;
using zxing::ResultPointCallback;

namespace
{

  class EstimatedModuleComparator
  {
  public:
    bool operator()(QSharedPointer<FinderPattern> a, QSharedPointer<FinderPattern> b)
    {
      return a->getEstimatedModuleSize() < b->getEstimatedModuleSize();
    }
  };

}

int FinderPatternFinder::CENTER_QUORUM = 2;
int FinderPatternFinder::MIN_SKIP = 3;
int FinderPatternFinder::MAX_MODULES = 57;

float FinderPatternFinder::centerFromEnd(int *stateCount, int end)
{
  return (float)(end - stateCount[4] - stateCount[3]) - stateCount[2] / 2.0f;
}

bool FinderPatternFinder::foundPatternCross(int *stateCount)
{
  int totalModuleSize = 0;
  for (int i = 0; i < 5; i++)
  {
    int count = stateCount[i];
    if (count == 0)
    {
      return false;
    }
    totalModuleSize += count;
  }
  if (totalModuleSize < 7)
  {
    return false;
  }
  int moduleSize = (totalModuleSize << 8) / 7;
  int maxVariance = moduleSize / 2;
  // Allow less than 50% variance from 1-1-3-1-1 proportions
  if (   ::abs(moduleSize - (stateCount[0] << 8)) < maxVariance &&
         ::abs(moduleSize - (stateCount[1] << 8)) < maxVariance &&
         ::abs(3.0f * moduleSize - (stateCount[2] << 8)) < 3 * maxVariance &&
         ::abs(moduleSize - (stateCount[3] << 8)) < maxVariance &&
         ::abs(moduleSize - (stateCount[4] << 8)) < maxVariance
     ) {
      return true;
  }

  // Proportions are not matched by the pattern. However, a bad print or a blurred frame
  // may lead to a false negative. Try to check whether there is a valid center and the
  // two extremities are of similar size, with blacks and whites having matching widths.

  return ::abs(3.0f * moduleSize - (stateCount[2] << 8))                   < 3 * maxVariance &&  // center is 3/7 +- variance
         ::abs(2.0f * moduleSize - ((stateCount[0] + stateCount[1]) << 8)) < 2 * maxVariance &&  // left/top is 2/7 +- variance
         ::abs(2.0f * moduleSize - ((stateCount[3] + stateCount[4]) << 8)) < 2 * maxVariance &&  // right/bottom is 2/7 +- variance
         (::abs(stateCount[0] - stateCount[4]) << 8) < maxVariance &&  // difference between first and last black is < variance
         (::abs(stateCount[1] - stateCount[3]) << 8) < maxVariance;    // difference between whites is < variance
}

float FinderPatternFinder::crossCheckVertical(size_t startI, size_t centerJ, int maxCount, int originalStateCountTotal)
{

  int maxI = image_->getHeight();
  int *stateCount = getCrossCheckStateCount();

  // Start counting up from center
  int i = int(startI);
  while (i >= 0 && image_->get(int(centerJ), i))
  {
    stateCount[2]++;
    i--;
  }
  if (i < 0)
  {
    return nan();
  }
  while (i >= 0 && !image_->get(int(centerJ), i) && stateCount[1] <= maxCount)
  {
    stateCount[1]++;
    i--;
  }
  // If already too many modules in this state or ran off the edge:
  if (i < 0 || stateCount[1] > maxCount)
  {
    return nan();
  }
  while (i >= 0 && image_->get(int(centerJ), i) && stateCount[0] <= maxCount)
  {
    stateCount[0]++;
    i--;
  }
  if (stateCount[0] > maxCount)
  {
    return nan();
  }

  // Now also count down from center
  i = int(startI) + 1;
  while (i < maxI && image_->get(int(centerJ), i))
  {
    stateCount[2]++;
    i++;
  }
  if (i == maxI)
  {
    return nan();
  }
  while (i < maxI && !image_->get(int(centerJ), i) && stateCount[3] < maxCount)
  {
    stateCount[3]++;
    i++;
  }
  if (i == maxI || stateCount[3] >= maxCount)
  {
    return nan();
  }
  while (i < maxI && image_->get(int(centerJ), i) && stateCount[4] < maxCount)
  {
    stateCount[4]++;
    i++;
  }
  if (stateCount[4] >= maxCount)
  {
    return nan();
  }

  // If we found a finder-pattern-like section, but its size is more than 40% different than
  // the original, assume it's a false positive
  int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
  if (5 * abs(stateCountTotal - originalStateCountTotal) >= 2 * originalStateCountTotal)
  {
    return nan();
  }

  return foundPatternCross(stateCount) ? centerFromEnd(stateCount, i) : nan();
}

float FinderPatternFinder::crossCheckHorizontal(size_t startJ, size_t centerI, int maxCount,
                                                int originalStateCountTotal)
{

  int maxJ = image_->getWidth();
  int *stateCount = getCrossCheckStateCount();

  int j = int(startJ);
  while (j >= 0 && image_->get(j, int(centerI)))
  {
    stateCount[2]++;
    j--;
  }
  if (j < 0)
  {
    return nan();
  }
  while (j >= 0 && !image_->get(j, int(centerI)) && stateCount[1] <= maxCount)
  {
    stateCount[1]++;
    j--;
  }
  if (j < 0 || stateCount[1] > maxCount)
  {
    return nan();
  }
  while (j >= 0 && image_->get(j, int(centerI)) && stateCount[0] <= maxCount)
  {
    stateCount[0]++;
    j--;
  }
  if (stateCount[0] > maxCount)
  {
    return nan();
  }

  j = int(startJ) + 1;
  while (j < maxJ && image_->get(j, int(centerI)))
  {
    stateCount[2]++;
    j++;
  }
  if (j == maxJ)
  {
    return nan();
  }
  while (j < maxJ && !image_->get(j, int(centerI)) && stateCount[3] < maxCount)
  {
    stateCount[3]++;
    j++;
  }
  if (j == maxJ || stateCount[3] >= maxCount)
  {
    return nan();
  }
  while (j < maxJ && image_->get(j, int(centerI)) && stateCount[4] < maxCount)
  {
    stateCount[4]++;
    j++;
  }
  if (stateCount[4] >= maxCount)
  {
    return nan();
  }

  // If we found a finder-pattern-like section, but its size is significantly different than
  // the original, assume it's a false positive
  int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
  if (5 * abs(stateCountTotal - originalStateCountTotal) >= originalStateCountTotal)
  {
    return nan();
  }

  return foundPatternCross(stateCount) ? centerFromEnd(stateCount, j) : nan();
}

bool FinderPatternFinder::handlePossibleCenter(int *stateCount, size_t i, size_t j)
{
  int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
  float centerJ = centerFromEnd(stateCount, int(j));
  float centerI = crossCheckVertical(i, (size_t)centerJ, stateCount[2], stateCountTotal);
  if (!isnan_z(centerI))
  {
    // Re-cross check
    centerJ = crossCheckHorizontal((size_t)centerJ, (size_t)centerI, stateCount[2], stateCountTotal);
    if (!isnan_z(centerJ) && crossCheckDiagonal((int)centerI, (int)centerJ, stateCount[2], stateCountTotal))
    {
      float estimatedModuleSize = (float)stateCountTotal / 7.0f;
      bool found = false;
      size_t max = possibleCenters_.size();
      for (size_t index = 0; index < max; index++)
      {
        QSharedPointer<FinderPattern> center = possibleCenters_[index];
        // Look for about the same center and module size:
        if (center->aboutEquals(estimatedModuleSize, centerI, centerJ))
        {
          possibleCenters_[index] = center->combineEstimate(centerI, centerJ, estimatedModuleSize);
          found = true;
          break;
        }
      }
      if (!found)
      {
        QSharedPointer<FinderPattern> newPattern(new FinderPattern(centerJ, centerI, estimatedModuleSize));
        possibleCenters_.push_back(newPattern);
        if (callback_ != 0)
        {
          callback_->foundPossibleResultPoint(*newPattern);
        }
      }
      return true;
    }
  }
  return false;
}

int FinderPatternFinder::findRowSkip()
{
  size_t max = possibleCenters_.size();
  if (max <= 1)
  {
    return 0;
  }
  QSharedPointer<FinderPattern> firstConfirmedCenter;
  for (size_t i = 0; i < max; i++)
  {
    QSharedPointer<FinderPattern> center = possibleCenters_[i];
    if (center->getCount() >= CENTER_QUORUM)
    {
      if (firstConfirmedCenter == 0)
      {
        firstConfirmedCenter = center;
      }
      else
      {
        // We have two confirmed centers
        // How far down can we skip before resuming looking for the next
        // pattern? In the worst case, only the difference between the
        // difference in the x / y coordinates of the two centers.
        // This is the case where you find top left first. Draw it out.
        hasSkipped_ = true;
        return (int)(abs(firstConfirmedCenter->getX() - center->getX()) - abs(firstConfirmedCenter->getY() - center->getY())) / 2;
      }
    }
  }
  return 0;
}

bool FinderPatternFinder::haveMultiplyConfirmedCenters()
{
  int confirmedCount = 0;
  float totalModuleSize = 0.0f;
  size_t max = possibleCenters_.size();
  for (size_t i = 0; i < max; i++)
  {
    QSharedPointer<FinderPattern> pattern = possibleCenters_[i];
    if (pattern->getCount() >= CENTER_QUORUM)
    {
      confirmedCount++;
      totalModuleSize += pattern->getEstimatedModuleSize();
    }
  }
  if (confirmedCount < 3)
  {
    return false;
  }
  // OK, we have at least 3 confirmed centers, but, it's possible that one is a "false positive"
  // and that we need to keep looking. We detect this by asking if the estimated module sizes
  // vary too much. We arbitrarily say that when the total deviation from average exceeds
  // 5% of the total module size estimates, it's too much.
  float average = totalModuleSize / (float)max;
  float totalDeviation = 0.0f;
  for (size_t i = 0; i < max; i++)
  {
    QSharedPointer<FinderPattern> pattern = possibleCenters_[i];
    totalDeviation += abs(pattern->getEstimatedModuleSize() - average);
  }
  return totalDeviation <= 0.05f * totalModuleSize;
}

vector<QSharedPointer<FinderPattern>> FinderPatternFinder::selectBestPatterns()
{
  std::vector<QSharedPointer<FinderPattern>> bestPatterns;
  int startSize = static_cast<int>(possibleCenters_.size());
  if (startSize < 3)
  {
    // Couldn't find enough finder patterns
    return bestPatterns;
  }

  std::sort(possibleCenters_.begin(), possibleCenters_.end(), EstimatedModuleComparator());

  double distortion = std::numeric_limits<double>::max();
  vector<double> squares(3, 0);

  for (int i = 0; i < startSize - 2; i++)
  {
    QSharedPointer<FinderPattern> fpi = possibleCenters_[i];
    float minModuleSize = fpi->getEstimatedModuleSize();

    for (int j = i + 1; j < startSize - 1; j++)
    {
      QSharedPointer<FinderPattern> fpj = possibleCenters_[j];
      double squares0 = squaredDistance(fpi, fpj);

      for (int k = j + 1; k < startSize; k++)
      {
        QSharedPointer<FinderPattern> fpk = possibleCenters_[k];
        float maxModuleSize = fpk->getEstimatedModuleSize();
        if (maxModuleSize > minModuleSize * 1.4f)
        {
          // module size is not similar
          continue;
        }

        squares[0] = squares0;
        squares[1] = squaredDistance(fpj, fpk);
        squares[2] = squaredDistance(fpi, fpk);
        std::sort(squares.begin(), squares.end());

        // a^2 + b^2 = c^2 (Pythagorean theorem), and a = b (isosceles triangle).
        // Since any right triangle satisfies the formula c^2 - b^2 - a^2 = 0,
        // we need to check both two equal sides separately.
        // The value of |c^2 - 2 * b^2| + |c^2 - 2 * a^2| increases as dissimilarity
        // from isosceles right triangle.
        double d = std::abs(squares[2] - 2 * squares[1]) + std::abs(squares[2] - 2 * squares[0]);
        if (d < distortion)
        {
          distortion = d;
          bestPatterns.resize(3);
          bestPatterns[0] = fpi;
          bestPatterns[1] = fpj;
          bestPatterns[2] = fpk;
        }
      }
    }
  }

  return bestPatterns;
}

vector<QSharedPointer<FinderPattern>> FinderPatternFinder::orderBestPatterns(vector<QSharedPointer<FinderPattern>> patterns)
{
  // Find distances between pattern centers
  float abDistance = distance(patterns[0], patterns[1]);
  float bcDistance = distance(patterns[1], patterns[2]);
  float acDistance = distance(patterns[0], patterns[2]);

  QSharedPointer<FinderPattern> topLeft;
  QSharedPointer<FinderPattern> topRight;
  QSharedPointer<FinderPattern> bottomLeft;
  // Assume one closest to other two is top left;
  // topRight and bottomLeft will just be guesses below at first
  if (bcDistance >= abDistance && bcDistance >= acDistance)
  {
    topLeft = patterns[0];
    topRight = patterns[1];
    bottomLeft = patterns[2];
  }
  else if (acDistance >= bcDistance && acDistance >= abDistance)
  {
    topLeft = patterns[1];
    topRight = patterns[0];
    bottomLeft = patterns[2];
  }
  else
  {
    topLeft = patterns[2];
    topRight = patterns[0];
    bottomLeft = patterns[1];
  }

  // Use cross product to figure out which of other1/2 is the bottom left
  // pattern. The vector "top-left -> bottom-left" x "top-left -> top-right"
  // should yield a vector with positive z component
  if ((bottomLeft->getY() - topLeft->getY()) * (topRight->getX() - topLeft->getX()) < (bottomLeft->getX() - topLeft->getX()) * (topRight->getY() - topLeft->getY()))
  {
    QSharedPointer<FinderPattern> temp = topRight;
    topRight = bottomLeft;
    bottomLeft = temp;
  }

  vector<QSharedPointer<FinderPattern>> results(3);
  results[0] = bottomLeft;
  results[1] = topLeft;
  results[2] = topRight;
  return results;
}

float FinderPatternFinder::distance(QSharedPointer<ResultPoint> p1, QSharedPointer<ResultPoint> p2)
{
  float dx = p1->getX() - p2->getX();
  float dy = p1->getY() - p2->getY();
  return (float)sqrt(dx * dx + dy * dy);
}

FinderPatternFinder::FinderPatternFinder(QSharedPointer<BitMatrix> image,
                                         QSharedPointer<ResultPointCallback> const &callback) : image_(image), possibleCenters_(), hasSkipped_(false), callback_(callback)
{
}

QSharedPointer<FinderPatternInfo> FinderPatternFinder::find(DecodeHints const &hints)
{
  bool tryHarder = hints.getTryHarder();

  size_t maxI = image_->getHeight();
  size_t maxJ = image_->getWidth();

  // We are looking for black/white/black/white/black modules in
  // 1:1:3:1:1 ratio; this tracks the number of such modules seen so far

  // As this is used often, we use an integer array instead of vector
  int stateCount[5];
  bool done = false;

  // Let's assume that the maximum version QR Code we support takes up 1/4
  // the height of the image, and then account for the center being 3
  // modules in size. This gives the smallest number of pixels the center
  // could be, so skip this often. When trying harder, look for all
  // QR versions regardless of how dense they are.
  int iSkip = (3 * int(maxI)) / (4 * MAX_MODULES);
  if (iSkip < MIN_SKIP || tryHarder)
  {
    iSkip = MIN_SKIP;
  }

  // This is slightly faster than using the Ref. Efficiency is important here
  BitMatrix &matrix = *image_;

  for (size_t i = iSkip - 1; i < maxI && !done; i += iSkip)
  {
    // Get a row of black/white values

    memset(stateCount, 0, sizeof(stateCount));
    int currentState = 0;
    for (size_t j = 0; j < maxJ; j++)
    {
      if (matrix.get(int(j), int(i)))
      {
        // Black pixel
        if ((currentState & 1) == 1)
        { // Counting white pixels
          currentState++;
        }
        stateCount[currentState]++;
      }
      else
      { // White pixel
        if ((currentState & 1) == 0)
        { // Counting black pixels
          if (currentState == 4)
          { // A winner?
            if (foundPatternCross(stateCount))
            { // Yes
              bool confirmed = handlePossibleCenter(stateCount, i, j);
              if (confirmed)
              {
                // Start examining every other line. Checking each line turned out to be too
                // expensive and didn't improve performance.
                iSkip = 2;
                if (hasSkipped_)
                {
                  done = haveMultiplyConfirmedCenters();
                }
                else
                {
                  int rowSkip = findRowSkip();
                  if (rowSkip > stateCount[2])
                  {
                    // Skip rows between row of lower confirmed center
                    // and top of presumed third confirmed center
                    // but back up a bit to get a full chance of detecting
                    // it, entire width of center of finder pattern

                    // Skip by rowSkip, but back off by stateCount[2] (size
                    // of last center of pattern we saw) to be conservative,
                    // and also back off by iSkip which is about to be
                    // re-added
                    i += rowSkip - stateCount[2] - iSkip;
                    j = maxJ - 1;
                  }
                }
              }
              else
              {
                stateCount[0] = stateCount[2];
                stateCount[1] = stateCount[3];
                stateCount[2] = stateCount[4];
                stateCount[3] = 1;
                stateCount[4] = 0;
                currentState = 3;
                continue;
              }
              // Clear state to start looking again
              currentState = 0;
              memset(stateCount, 0, sizeof(stateCount));
            }
            else
            { // No, shift counts back by two
              stateCount[0] = stateCount[2];
              stateCount[1] = stateCount[3];
              stateCount[2] = stateCount[4];
              stateCount[3] = 1;
              stateCount[4] = 0;
              currentState = 3;
            }
          }
          else
          {
            stateCount[++currentState]++;
          }
        }
        else
        { // Counting white pixels
          stateCount[currentState]++;
        }
      }
    }
    if (foundPatternCross(stateCount))
    {
      bool confirmed = handlePossibleCenter(stateCount, i, maxJ);
      if (confirmed)
      {
        iSkip = stateCount[0];
        if (hasSkipped_)
        {
          // Found a third one
          done = haveMultiplyConfirmedCenters();
        }
      }
    }
  }

  if (possibleCenters_.size() < 3)
  {
    throw zxing::ReaderException("no possible centers found");
  }

  vector<QSharedPointer<FinderPattern>> patternInfo = selectBestPatterns();

  if (patternInfo.size() != 3)
  {
    throw zxing::ReaderException("no pattern info found");
  }

  vector<QSharedPointer<ResultPoint>> patternInfoResPoints;

  for (size_t i = 0; i < patternInfo.size(); i++)
    patternInfoResPoints.push_back(QSharedPointer<ResultPoint>(patternInfo[i]));

  ResultPoint::orderBestPatterns(patternInfoResPoints);

  patternInfo.clear();
  for (size_t i = 0; i < patternInfoResPoints.size(); i++)
      patternInfo.push_back(qSharedPointerCast<FinderPattern>(patternInfoResPoints[i]));

  QSharedPointer<FinderPatternInfo> result(new FinderPatternInfo(patternInfo));
  return result;
}

QSharedPointer<BitMatrix> FinderPatternFinder::getImage()
{
  return image_;
}

vector<QSharedPointer<FinderPattern>> &FinderPatternFinder::getPossibleCenters()
{
  return possibleCenters_;
}

bool FinderPatternFinder::crossCheckDiagonal(int startI, int centerJ, int maxCount, int originalStateCountTotal) const
{
  int *stateCount = getCrossCheckStateCount();

  // Start counting up, left from center finding black center mass
  int i = 0;
  while (startI >= i && centerJ >= i && image_->get(centerJ - i, startI - i))
  {
    stateCount[2]++;
    i++;
  }

  if (startI < i || centerJ < i)
  {
    return false;
  }

  // Continue up, left finding white space
  while (startI >= i && centerJ >= i && !image_->get(centerJ - i, startI - i) &&
         stateCount[1] <= maxCount)
  {
    stateCount[1]++;
    i++;
  }

  // If already too many modules in this state or ran off the edge:
  if (startI < i || centerJ < i || stateCount[1] > maxCount)
  {
    return false;
  }

  // Continue up, left finding black border
  while (startI >= i && centerJ >= i && image_->get(centerJ - i, startI - i) &&
         stateCount[0] <= maxCount)
  {
    stateCount[0]++;
    i++;
  }
  if (stateCount[0] > maxCount)
  {
    return false;
  }

  int maxI = image_->getHeight();
  int maxJ = image_->getWidth();

  // Now also count down, right from center
  i = 1;
  while (startI + i < maxI && centerJ + i < maxJ && image_->get(centerJ + i, startI + i))
  {
    stateCount[2]++;
    i++;
  }

  // Ran off the edge?
  if (startI + i >= maxI || centerJ + i >= maxJ)
  {
    return false;
  }

  while (startI + i < maxI && centerJ + i < maxJ && !image_->get(centerJ + i, startI + i) &&
         stateCount[3] < maxCount)
  {
    stateCount[3]++;
    i++;
  }

  if (startI + i >= maxI || centerJ + i >= maxJ || stateCount[3] >= maxCount)
  {
    return false;
  }

  while (startI + i < maxI && centerJ + i < maxJ && image_->get(centerJ + i, startI + i) &&
         stateCount[4] < maxCount)
  {
    stateCount[4]++;
    i++;
  }

  if (stateCount[4] >= maxCount)
  {
    return false;
  }

  // If we found a finder-pattern-like section, but its size is more than 100% different than
  // the original, assume it's a false positive
  int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
  return abs(stateCountTotal - originalStateCountTotal) < 2 * originalStateCountTotal &&
         foundPatternCross(stateCount);
}

int *FinderPatternFinder::getCrossCheckStateCount() const
{
  memset(crossCheckStateCount, 0, sizeof(crossCheckStateCount));
  return crossCheckStateCount;
}

double FinderPatternFinder::squaredDistance(QSharedPointer<zxing::qrcode::FinderPattern> a, QSharedPointer<zxing::qrcode::FinderPattern> b)
{
  double x = a->getX() - b->getX();
  double y = a->getY() - b->getY();
  return x * x + y * y;
}
