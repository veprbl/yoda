// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2011 The YODA collaboration (see AUTHORS for details)
//
#ifndef YODA_Axis1D_h
#define YODA_Axis1D_h

#include "YODA/AnalysisObject.h"
#include "YODA/Exceptions.h"
#include "YODA/Bin.h"
#include "YODA/Utils/MathUtils.h"
#include <string>
#include <cassert>
#include <cmath>
#include <algorithm>

namespace YODA {


  /// @brief A 1D templated container of ordered bins
  ///
  /// This class is separately templated on the bin and distribution types.
  template <typename BIN1D, typename DBN>
  class Axis1D {
  public:


    typedef BIN1D Bin;
    typedef typename std::vector<BIN1D> Bins;


    // /// @name Helper functions to make bin edge vectors (see @file MathUtils.h)
    // //@{

    // static inline std::vector<double> mkBinEdgesLin(double start, double end, size_t nbins) {
    //   return linspace(start, end, nbins);
    // }

    // static inline std::vector<double> mkBinEdgesLog(double start, double end, size_t nbins) {
    //   return logspace(start, end, nbins);
    // }

    // //@}


  public:


    /// Null constructor.
    /// @todo Remove if we can.
    Axis1D() { }


    /// Constructor with a list of bin edges
    /// @todo Accept a general iterable and remove this silly special-casing for std::vector
    Axis1D(const std::vector<double>& binedges) {
      assert(binedges.size() > 1);
      _mkAxis(binedges);
    }


    /// Constructor with histogram limits, number of bins, and a bin distribution enum
    Axis1D(size_t nbins, double lower, double upper) {
      std::cout << lower << " " << upper << std::endl;
      _mkAxis(linspace(lower, upper, nbins));
    }


    /// @todo Accept a general iterable and remove this silly special-casing for std::vector
    Axis1D(const std::vector<BIN1D>& bins) {
      assert(!bins.empty());
      Bins sbins;
      for (typename std::vector<BIN1D>::const_iterator b = bins.begin(); b != bins.end(); ++b) {
        sbins.push_back(*b);
      }
      _mkAxis(sbins);
    }


    /// @brief State-setting constructor
    /// Principally intended for internal persistency use.
    Axis1D(const Bins& bins, const DBN& dbn_tot, const DBN& dbn_uflow, const DBN& dbn_oflow)
      : _dbn(dbn_tot), _underflow(dbn_uflow), _overflow(dbn_oflow)
    {
      assert(!bins.empty());
      _mkAxis(bins);
    }


    /////////////////////


  public:

    unsigned int numBins() const {
      return _bins.size();
    }


    // void addBin() {
    // }


    Bins& bins() {
      return _bins;
    }


    const Bins& bins() const {
      return _bins;
    }


    std::pair<double,double> binEdges(size_t binId) const {
      assert(binId < numBins());
      return std::make_pair(_cachedBinEdges[binId], _cachedBinEdges[binId+1]);
    }


    double lowEdge() const {
      return _bins.front().lowEdge();
    }
    double xMin() const { return lowEdge(); }


    double highEdge() const {
      return _bins.back().highEdge();
    }
    double xMax() const { return highEdge(); }


    BIN1D& bin(size_t index) {
      if (index >= numBins())
        throw RangeError("YODA::Histo: index out of range");
      return _bins[index];
    }


    const BIN1D& bin(size_t index) const {
      if (index >= numBins())
        throw RangeError("YODA::Histo: index out of range");
      return _bins[index];
    }


    BIN1D& binByCoord(double x) {
      return bin(findBinIndex(x));
    }

    const BIN1D& binByCoord(double x) const {
      return bin(findBinIndex(x));
    }


    DBN& totalDbn() {
      return _dbn;
    }

    const DBN& totalDbn() const {
      return _dbn;
    }


    DBN& underflow() {
      return _underflow;
    }

    const DBN& underflow() const {
      return _underflow;
    }


    DBN& overflow() {
      return _overflow;
    }

    const DBN& overflow() const {
      return _overflow;
    }


    size_t findBinIndex(double coord) const {
      /// @todo Improve!
      if (coord < _cachedBinEdges[0] || coord >= _cachedBinEdges[numBins()]) {
        throw RangeError("Coordinate is outside the valid range: you should request the underlow or overflow");
      }
      size_t i = _binHash.upper_bound(coord)->second;
      return i;
    }


    void reset() {
      _dbn.reset();
      _underflow.reset();
      _overflow.reset();
      for (typename Bins::iterator b = _bins.begin(); b != _bins.end(); ++b) {
        b->reset();
      }
    }


    /// @brief Merge bins so that bin widths are roughly increased by a factor @a factor.
    /// Note that rebinnings to this factor have to be approximate, since bins are discrete.
    void rebin(int factor) {
      assert(factor >= 1);
      /// @todo Implement! Requires ability to change bin edges from outside...
      throw std::runtime_error("Rebinning is not yet implemented! Pester me, please.");
    }


    /// Merge a bin range @a binindex1 to @a binindex2 into a single bin.
    void mergeBins(size_t binindex1, size_t binindex2) {
      assert(binindex1 >= binindex2);
      if (binindex1 < 0 || binindex1 >= numBins()) throw RangeError("binindex1 is out of range");
      if (binindex2 < 0 || binindex2 >= numBins()) throw RangeError("binindex2 is out of range");
      /// @todo Implement! Requires ability to change bin edges from outside...
      throw std::runtime_error("Rebinning is not yet implemented! Pester me, please.");
    }


    /// Scale the axis coordinates (i.e. bin edges).
    void scaleX(double scalefactor) {
       _dbn.scaleX(scalefactor);
       _underflow.scaleW(scalefactor);
       _overflow.scaleW(scalefactor);
       for(int i=0; i < _bins.size(); i++) _bins[i].scaleX(scalefactor);
       for(int i=0; i < _cachedBinEdges.size(); i++) _cachedBinEdges[i] *= scalefactor;
       _mkBinHash();
    }


    /// Scale the weights, as if all fills so far had used weights which differed by the given factor.
    void scaleW(double scalefactor) {
      _dbn.scaleW(scalefactor);
      _underflow.scaleW(scalefactor);
      _overflow.scaleW(scalefactor);
      for (typename Bins::iterator b = _bins.begin(); b != _bins.end(); ++b) {
        b->scaleW(scalefactor);
      }
    }


  public:

    bool operator == (const Axis1D& other) const {
      /// @todo Need/want to compare bin hash?
      return
        _cachedBinEdges == other._cachedBinEdges &&
        _binHash == other._binHash;
    }


    bool operator != (const Axis1D& other) const {
      return ! operator == (other);
    }


    Axis1D<BIN1D,DBN>& operator += (const Axis1D<BIN1D,DBN>& toAdd) {
      if (*this != toAdd) {
        throw LogicError("YODA::Histo1D: Cannot add axes with different binnings.");
      }
      for (size_t i = 0; i < bins().size(); ++i) {
        bins().at(i) += toAdd.bins().at(i);
      }
      _dbn += toAdd._dbn;
      _underflow += toAdd._underflow;
      _overflow  += toAdd._overflow;
      return *this;
    }


    Axis1D<BIN1D,DBN>& operator -= (const Axis1D<BIN1D,DBN>& toSubtract) {
      if (*this != toSubtract) {
        throw LogicError("YODA::Histo1D: Cannot subtract axes with different binnings.");
      }
      for (size_t i = 0; i < bins().size(); ++i) {
        bins().at(i) += toSubtract.bins().at(i);
      }
      _dbn -= toSubtract._dbn;
      _underflow -= toSubtract._underflow;
      _overflow  -= toSubtract._overflow;
      return *this;
    }


  private:

    /// @todo Remove
    void _mkBinHash() {
      for (size_t i = 0; i < numBins(); i++) {
        // Insert upper bound mapped to bin ID
        _binHash.insert(std::make_pair(_cachedBinEdges[i+1],i));
      }
    }


    void _mkAxis(const std::vector<double>& binedges) {
      const size_t nbins = binedges.size() - 1;
      for (size_t i = 0; i < nbins; ++i) {
        _bins.push_back( BIN1D(binedges.at(i), binedges.at(i+1)) );
      }
      std::sort(_bins.begin(), _bins.end());

      /// @todo Remove
      _cachedBinEdges = binedges;
      std::sort(_cachedBinEdges.begin(), _cachedBinEdges.end());
      _mkBinHash();
    }


    void _mkAxis(const Bins& bins) {
      _bins = bins;
      std::sort(_bins.begin(), _bins.end());

      /// @todo Remove
      for (size_t i = 0; i < bins.size(); ++i) {
        _cachedBinEdges.push_back(bins.at(i).lowEdge());
      }
      _cachedBinEdges.push_back(bins.back().highEdge());
      _mkBinHash();
    }


  private:


    /// @todo Store bins in a more flexible (and sorted) way
    /// @todo Check non-overlap of bins
    /// @todo Bin access by index
    /// @todo Overall y-dbn for profiles?


    /// @name Bin data
    //@{

    /// The bins contained in this histogram
    Bins _bins;

    /// A distribution counter for the whole histogram
    DBN _dbn;

    /// A distribution counter for overflow fills
    DBN _underflow;
    /// A distribution counter for underlow fills
    DBN _overflow;

    /// Bin edges: lower edges, except last entry,
    /// which is the high edge of the last bin
    std::vector<double> _cachedBinEdges;

    /// Map for fast bin lookup
    std::map<double,size_t> _binHash;
    //@}

  };



  template <typename BIN1D, typename DBN>
  Axis1D<BIN1D,DBN> operator + (const Axis1D<BIN1D,DBN>& first, const Axis1D<BIN1D,DBN>& second) {
    Axis1D<BIN1D,DBN> tmp = first;
    tmp += second;
    return tmp;
  }


  template <typename BIN1D, typename DBN>
  Axis1D<BIN1D,DBN> operator - (const Axis1D<BIN1D,DBN>& first, const Axis1D<BIN1D,DBN>& second) {
    Axis1D<BIN1D,DBN> tmp = first;
    tmp -= second;
    return tmp;
  }



}

#endif
