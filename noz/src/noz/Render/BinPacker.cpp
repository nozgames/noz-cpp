///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BinPacker.h"

using namespace noz;

BinPacker::BinPacker(void) {
}

BinPacker::BinPacker(noz_int32 width, noz_int32 height) {
	Resize(width, height);
}

void BinPacker::Resize(noz_int32 width, noz_int32 height) {
  size_.w = width;
  size_.h = height;

	used_.clear();
	free_.clear();
	free_.push_back(BinRect(1,1,width-2,height-2));
}

BinPacker::BinRect BinPacker::Insert(BinSize size, Method method) {
	BinRect rect;
	noz_int32 score1;
	noz_int32 score2;
	
  switch(method) {
		case Method::BestShortSideFit:  rect = FindPositionForNewNodeBestShortSideFit(size.w, size.h, score1, score2); break;
		case Method::BottomLeftRule:    rect = FindPositionForNewNodeBottomLeft(size.w, size.h, score1, score2); break;
		case Method::ContactPointRule:  rect = FindPositionForNewNodeContactPoint(size.w, size.h, score1); break;
		case Method::BestLongSideFit:   rect = FindPositionForNewNodeBestLongSideFit(size.w, size.h, score2, score1); break;
		case Method::BestAreaFit:       rect = FindPositionForNewNodeBestAreaFit(size.w, size.h, score1, score2); break;
	}
		
	if (rect.h == 0) return rect;

  PlaceRect(rect);

	return rect;
}

/*
void BinPacker::Insert(std::vector<RectSize> &rects, std::vector<Rect> &dst, Method method)
{
	dst.clear();

	while(rects.size() > 0)
	{
		noz_int32 bestScore1 = std::numeric_limits<noz_int32>::max();
		noz_int32 bestScore2 = std::numeric_limits<noz_int32>::max();
		noz_int32 bestRectIndex = -1;
		Rect rect;

		for(size_t i = 0; i < rects.size(); ++i)
		{
			noz_int32 score1;
			noz_int32 score2;
			Rect newNode = ScoreRect(rects[i].w, rects[i].h, method, score1, score2);

			if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
			{
				bestScore1 = score1;
				bestScore2 = score2;
				rect = newNode;
				bestRectIndex = i;
			}
		}

		if (bestRectIndex == -1)
			return;

		PlaceRect(rect);
		rects.erase(rects.begin() + bestRectIndex);
	}
}
*/

void BinPacker::PlaceRect(const BinRect &rect) {
	size_t freeCount = free_.size();
	for(size_t i = 0; i < freeCount; ++i)	{
		if (SplitFreeNode(free_[i], rect)) {
			free_.erase(free_.begin() + i);
			--i;
			--freeCount;
		}
	}

	PruneFreeList();

	used_.push_back(rect);
}

BinPacker::BinRect BinPacker::ScoreRect(BinSize size, Method method, noz_int32 &score1, noz_int32 &score2) const {
	BinRect rect;
	score1 = std::numeric_limits<noz_int32>::max();
	score2 = std::numeric_limits<noz_int32>::max();
	
  switch(method) 	{
	  case Method::BestShortSideFit: rect = FindPositionForNewNodeBestShortSideFit(size.w, size.h, score1, score2); break;
	  case Method::BottomLeftRule:   rect = FindPositionForNewNodeBottomLeft(size.w, size.h, score1, score2); break;
	  case Method::ContactPointRule: 
      rect = FindPositionForNewNodeContactPoint(size.w, size.h, score1); 
		  score1 = -score1; // Reverse since we are minimizing, but for contact point score bigger is better.
		  break;
	  case Method::BestLongSideFit: rect = FindPositionForNewNodeBestLongSideFit(size.w, size.h, score2, score1); break;
	  case Method::BestAreaFit: rect = FindPositionForNewNodeBestAreaFit(size.w, size.h, score1, score2); break;
	}

	// Cannot fit the current rectangle.
	if (rect.h == 0) {
		score1 = std::numeric_limits<noz_int32>::max();
		score2 = std::numeric_limits<noz_int32>::max();
	}

	return rect;
}

/// Computes the ratio of used surface area.
noz_float BinPacker::GetOccupancy(void) const {
	unsigned long area = 0;
	for(size_t i = 0; i < used_.size(); ++i) {
		area += used_[i].w * used_[i].h;
  }

	return (noz_float)area / (size_.w * size_.h);
}

BinPacker::BinRect BinPacker::FindPositionForNewNodeBottomLeft(noz_int32 width, noz_int32 height, noz_int32 &bestY, noz_int32 &bestX) const {
	BinRect rect;

	bestY = std::numeric_limits<noz_int32>::max();

	for(size_t i = 0; i < free_.size(); ++i) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (free_[i].w >= width && free_[i].h >= height)
		{
			noz_int32 topSideY = free_[i].y + height;
			if (topSideY < bestY || (topSideY == bestY && free_[i].x < bestX))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestY = topSideY;
				bestX = free_[i].x;
			}
		}
		if (free_[i].w >= height && free_[i].h >= width)
		{
			noz_int32 topSideY = free_[i].y + width;
			if (topSideY < bestY || (topSideY == bestY && free_[i].x < bestX))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestY = topSideY;
				bestX = free_[i].x;
			}
		}
	}
	return rect;
}

BinPacker::BinRect BinPacker::FindPositionForNewNodeBestShortSideFit(noz_int32 width, noz_int32 height, noz_int32 &bestShortSideFit, noz_int32 &bestLongSideFit) const {
	BinRect rect;
	memset(&rect, 0, sizeof(Rect));

	bestShortSideFit = std::numeric_limits<noz_int32>::max();

	for(size_t i = 0; i < free_.size(); ++i)
	{
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (free_[i].w >= width && free_[i].h >= height)
		{
			noz_int32 leftoverHoriz = abs(free_[i].w - width);
			noz_int32 leftoverVert = abs(free_[i].h - height);
			noz_int32 shortSideFit = Math::Min(leftoverHoriz, leftoverVert);
			noz_int32 longSideFit = Math::Max(leftoverHoriz, leftoverVert);

			if (shortSideFit < bestShortSideFit || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}

		if (free_[i].w >= height && free_[i].h >= width)
		{
			noz_int32 flippedLeftoverHoriz = abs(free_[i].w - height);
			noz_int32 flippedLeftoverVert = abs(free_[i].h - width);
			noz_int32 flippedShortSideFit = Math::Min(flippedLeftoverHoriz, flippedLeftoverVert);
			noz_int32 flippedLongSideFit = Math::Max(flippedLeftoverHoriz, flippedLeftoverVert);

			if (flippedShortSideFit < bestShortSideFit || (flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = height;
				rect.h = width;
				bestShortSideFit = flippedShortSideFit;
				bestLongSideFit = flippedLongSideFit;
			}
		}
	}
	return rect;
}

BinPacker::BinRect BinPacker::FindPositionForNewNodeBestLongSideFit(noz_int32 width, noz_int32 height, 
	noz_int32 &bestShortSideFit, noz_int32 &bestLongSideFit) const
{
	BinRect rect;
	memset(&rect, 0, sizeof(rect));

	bestLongSideFit = std::numeric_limits<noz_int32>::max();

	for(size_t i = 0; i < free_.size(); ++i)
	{
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (free_[i].w >= width && free_[i].h >= height)
		{
			noz_int32 leftoverHoriz = abs(free_[i].w - width);
			noz_int32 leftoverVert = abs(free_[i].h - height);
			noz_int32 shortSideFit = Math::Min(leftoverHoriz, leftoverVert);
			noz_int32 longSideFit = Math::Max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}
    /*
		if (free_[i].w >= height && free_[i].h >= width)
		{
			noz_int32 leftoverHoriz = abs(free_[i].w - height);
			noz_int32 leftoverVert = abs(free_[i].h - width);
			noz_int32 shortSideFit = Math::Min(leftoverHoriz, leftoverVert);
			noz_int32 longSideFit = Math::Max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = height;
				rect.h = width;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}
*/

	}
	return rect;
}

BinPacker::BinRect BinPacker::FindPositionForNewNodeBestAreaFit(noz_int32 width, noz_int32 height, 
	noz_int32 &bestAreaFit, noz_int32 &bestShortSideFit) const
{
	BinRect rect;
	memset(&rect, 0, sizeof(Rect));

	bestAreaFit = std::numeric_limits<noz_int32>::max();

	for(size_t i = 0; i < free_.size(); ++i) {
		noz_int32 areaFit = free_[i].w * free_[i].h - width * height;

		// Try to place the rectangle in upright (non-flipped) orientation.
		if (free_[i].w >= width && free_[i].h >= height) {
			noz_int32 leftoverHoriz = abs(free_[i].w - width);
			noz_int32 leftoverVert = abs(free_[i].h - height);
			noz_int32 shortSideFit = Math::Min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit)) {
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}

		if (free_[i].w >= height && free_[i].h >= width) {
			noz_int32 leftoverHoriz = abs(free_[i].w - height);
			noz_int32 leftoverVert = abs(free_[i].h - width);
			noz_int32 shortSideFit = Math::Min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit)) {
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = height;
				rect.h = width;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}
	}
	return rect;
}

/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
noz_int32 CommonIntervalLength(noz_int32 i1start, noz_int32 i1end, noz_int32 i2start, noz_int32 i2end) {
	if (i1end < i2start || i2end < i1start) return 0;
	return Math::Min(i1end, i2end) - Math::Max(i1start, i2start);
}

noz_int32 BinPacker::ContactPointScoreNode(noz_int32 x, noz_int32 y, noz_int32 width, noz_int32 height) const {
	noz_int32 score = 0;

	if (x == 0 || x + width == size_.w) score += height;
	if (y == 0 || y + height == size_.h) score += width;

	for(size_t i = 0; i < used_.size(); ++i) {
		if (used_[i].x == x + width || used_[i].x + used_[i].w == x)
			score += CommonIntervalLength(used_[i].y, used_[i].y + used_[i].h, y, y + height);
		if (used_[i].y == y + height || used_[i].y + used_[i].h == y)
			score += CommonIntervalLength(used_[i].x, used_[i].x + used_[i].w, x, x + width);
	}

	return score;
}

BinPacker::BinRect BinPacker::FindPositionForNewNodeContactPoint(noz_int32 width, noz_int32 height, noz_int32 &bestContactScore) const {
	BinRect rect;
	memset(&rect, 0, sizeof(Rect));

	bestContactScore = -1;

	for(size_t i = 0; i < free_.size(); ++i) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (free_[i].w >= width && free_[i].h >= height) {
			noz_int32 score = ContactPointScoreNode(free_[i].x, free_[i].y, width, height);
			if (score > bestContactScore) {
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = width;
				rect.h = height;
				bestContactScore = score;
			}
		}
		if (free_[i].w >= height && free_[i].h >= width) {
			noz_int32 score = ContactPointScoreNode(free_[i].x, free_[i].y, width, height);
			if (score > bestContactScore) {
				rect.x = free_[i].x;
				rect.y = free_[i].y;
				rect.w = height;
				rect.h = width;
				bestContactScore = score;
			}
		}
	}
	return rect;
}

bool BinPacker::SplitFreeNode(BinRect freeNode, const BinRect &usedNode) {
	// Test with SAT if the rectangles even intersect.
	if (usedNode.x >= freeNode.x + freeNode.w || usedNode.x + usedNode.w <= freeNode.x ||
		usedNode.y >= freeNode.y + freeNode.h || usedNode.y + usedNode.h <= freeNode.y)
		return false;

	if (usedNode.x < freeNode.x + freeNode.w && usedNode.x + usedNode.w > freeNode.x) {
		// New node at the top side of the used node.
		if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.h) {
			BinRect newNode = freeNode;
			newNode.h = usedNode.y - newNode.y;
			free_.push_back(newNode);
		}

		// New node at the bottom side of the used node.
		if (usedNode.y + usedNode.h < freeNode.y + freeNode.h) {
			BinRect newNode = freeNode;
			newNode.y = usedNode.y + usedNode.h;
			newNode.h = freeNode.y + freeNode.h - (usedNode.y + usedNode.h);
			free_.push_back(newNode);
		}
	}

	if (usedNode.y < freeNode.y + freeNode.h && usedNode.y + usedNode.h > freeNode.y) {
		// New node at the left side of the used node.
		if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.w) {
			BinRect newNode = freeNode;
			newNode.w = usedNode.x - newNode.x;
			free_.push_back(newNode);
		}

		// New node at the right side of the used node.
		if (usedNode.x + usedNode.w < freeNode.x + freeNode.w) {
			BinRect newNode = freeNode;
			newNode.x = usedNode.x + usedNode.w;
			newNode.w = freeNode.x + freeNode.w - (usedNode.x + usedNode.w);
			free_.push_back(newNode);
		}
	}

	return true;
}

void BinPacker::PruneFreeList(void) {
	// Remoe redundance rectangles
	for(size_t i = 0; i < free_.size(); ++i) {
		for(size_t j = i+1; j < free_.size(); ++j) {
			if (IsContainedIn(free_[i], free_[j])) {
				free_.erase(free_.begin()+i);
				--i;
				break;
			}
			if (IsContainedIn(free_[j], free_[i])) {
				free_.erase(free_.begin()+j);
				--j;
			}
		}
  }
}
