//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include <vector>

namespace noz {

    class RectPacker {
    public: 
        
        enum class method {
            BestShortSideFit,   ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
            BestLongSideFit,    ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
            BestAreaFit,        ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
            BottomLeftRule,     ///< -BL: Does the Tetris placement.
            ContactPointRule    ///< -CP: Chooses the placement where the rectangle touches other rects as much as possible.
        };

        struct Size {
            Size() { w = 0; h = 0; }
            Size(int32_t _w, int32_t _h) { w = _w; h = _h; }

            int32_t w;
            int32_t h;
        };

        struct Rect {
            Rect() { x = y = w = h = 0; }
            Rect(int32_t _x, int32_t _y, int32_t _w, int32_t _h) { x = _x; y = _y; w = _w; h = _h; }

            int32_t x;
            int32_t y;
            int32_t w;
            int32_t h;
        };

        RectPacker();
        RectPacker(int32_t width, int32_t height);

        void Resize(int32_t width, int32_t height);

        int Insert(const Vec2Int& size, method method, Rect& result);
        int Insert(int32_t width, int32_t height, method method, Rect& result) { return Insert(Vec2Int(width, height), method, result); }

        // Mark a rect as used (for restoring saved atlas state)
        void MarkUsed(const Rect& rect);

        float GetOccupancy() const;

        const Size& size() const { return size_; }

        bool empty() const { return used_.empty(); }

        bool validate() const;

    private:

        Rect ScoreRect(Size size, method method, int32_t& score1, int32_t& score2) const;

        void PlaceRect(const Rect& node);

        int32_t ContactPointScoreNode(int x, int y, int width, int height) const;

        Rect FindPositionForNewNodeBottomLeft(int width, int height, int& bestY, int& bestX) const;
        Rect FindPositionForNewNodeBestShortSideFit(int width, int height, int& bestShortSideFit, int& bestLongSideFit) const;
        Rect FindPositionForNewNodeBestLongSideFit(int width, int height, int& bestShortSideFit, int& bestLongSideFit) const;
        Rect FindPositionForNewNodeBestAreaFit(int width, int height, int& bestAreaFit, int& bestShortSideFit) const;
        Rect FindPositionForNewNodeContactPoint(int width, int height, int& contactScore) const;

        bool SplitFreeNode(Rect freeRect, const Rect& usedRect);

        void PruneFreeList();

        static bool IsContainedIn(const Rect& a, const Rect& b) {
            return a.x >= b.x && a.y >= b.y && a.x + a.w <= b.x + b.w && a.y + a.h <= b.y + b.h;
        }

        Size size_;
        std::vector<Rect> used_;
        std::vector<Rect> free_;
    };
}
