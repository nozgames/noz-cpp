///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Graphics_BinPacker_h__
#define __noz_Graphics_BinPacker_h__

namespace noz {

  class BinPacker {
	  /// Methods for seleting a free rectangle
	  public: enum class Method {
		  BestShortSideFit,   ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
		  BestLongSideFit,    ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
		  BestAreaFit,        ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
		  BottomLeftRule,     ///< -BL: Does the Tetris placement.
		  ContactPointRule    ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
	  };

    public: struct BinSize {
      BinSize(void) {w = 0; h = 0;}
      BinSize(noz_int32 _w, noz_int32 _h) {w=_w; h=_h; }
	    noz_int32 w;
	    noz_int32 h;
    };

    public: struct BinRect {
      BinRect(void) {x=y=w=h=0;}
      BinRect(noz_int32 _x, noz_int32 _y, noz_int32 _w, noz_int32 _h) {x=_x;y=_y;w=_w;h=_h;}

	    noz_int32 x;
	    noz_int32 y;
	    noz_int32 w;
	    noz_int32 h;
    };

    /// Size of the bin packer
    private: BinSize size_;

    /// Vector of used rectangles
  	private: std::vector<BinRect> used_;

    /// Vector of free rectangles
	  private: std::vector<BinRect> free_;


    /// Create an empty bin with a size of 0,0
    public: BinPacker(void);

	  /// Create a bin packer with the given size
	  public: BinPacker (noz_int32 width, noz_int32 height);

  	/// Resize the bin to a new size and empty its contents.
	  public: void Resize (noz_int32 width, noz_int32 height);

	  /// Inserts the given list of rectangles in an offline/batch mode, possibly rotated.
	  /// @param rects The list of rectangles to insert. This vector will be destroyed in the process.
	  /// @param dst [out] This list will contain the packed rectangles. The indices will not correspond to that of rects.
	  /// @param method The rectangle placement rule to use when packing.
	  //public: void Insert(std::vector<BinSize> &rects, std::vector<BinRect> &dst, Method method);

	  /// Inserts a single rectangle into the bin, possibly rotated.
	  public: BinRect Insert(BinSize size, Method method);
	  public: BinRect Insert(noz_int32 width, noz_int32 height, Method method) {return Insert(BinSize(width,height),method);}

	  /// Computes the ratio of used surface area to the total bin area.
	  public: noz_float GetOccupancy(void) const;

    public: const BinSize& GetSize(void) const {return size_;}

	  /// Computes the placement score for placing the given rectangle with the given method.
	  /// @param score1 [out] The primary placement score will be outputted here.
	  /// @param score2 [out] The secondary placement score will be outputted here. This isu sed to break ties.
	  /// @return This struct identifies where the rectangle would be placed if it were placed.
	  private: BinRect ScoreRect(BinSize size, Method method, noz_int32& score1, noz_int32& score2) const;

	  /// Places the given rectangle into the bin.
	  private: void PlaceRect(const BinRect &node);

  	/// Computes the placement score for the -CP variant.
  	private: noz_int32 ContactPointScoreNode(int x, int y, int width, int height) const;

	  private: BinRect FindPositionForNewNodeBottomLeft(int width, int height, int &bestY, int &bestX) const;
	  private: BinRect FindPositionForNewNodeBestShortSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit) const;
	  private: BinRect FindPositionForNewNodeBestLongSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit) const;
	  private: BinRect FindPositionForNewNodeBestAreaFit(int width, int height, int &bestAreaFit, int &bestShortSideFit) const;
	  private: BinRect FindPositionForNewNodeContactPoint(int width, int height, int &contactScore) const;

	  /// @return True if the free node was split.
	  private: bool SplitFreeNode(BinRect freeRect, const BinRect &usedRect);

	  /// Goes through the free rectangle list and removes any redundant entries.
	  private: void PruneFreeList(void);

    private: bool IsContainedIn(const BinRect &a, const BinRect &b) const {
      return a.x >= b.x && a.y >= b.y && a.x+a.w <= b.x+b.w && a.y+a.h <= b.y+b.h;
    }
  };

} // namespace noz

#endif // __noz_Graphics_BinPacker_h__

