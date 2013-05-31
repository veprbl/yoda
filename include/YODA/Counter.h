// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2013 The YODA collaboration (see AUTHORS for details)
//
#ifndef YODA_Counter_h
#define YODA_Counter_h

#include "YODA/AnalysisObject.h"
#include "YODA/Dbn0D.h"
//#include "YODA/Scatter1D.h"
#include "YODA/Exceptions.h"
#include <vector>
#include <string>
#include <map>

namespace YODA {


  /// A weighted counter.
  class Counter : public AnalysisObject {
  public:

    /// @name Constructors
    //@{

    /// Default constructor
    Counter(const std::string& path="", const std::string& title="")
      : AnalysisObject("Counter", path, title),
        _dbn()
    { }


    /// @brief Constructor accepting an explicit Dbn0D.
    ///
    /// Intended both for internal persistency and user use.
    Counter(const Dbn0D& dbn,
            const std::string& path="", const std::string& title="")
            : AnalysisObject("Counter", path, title),
            _dbn(dbn)
    { }


    /// Copy constructor with optional new path
    /// @todo Don't copy the path?
    Counter(const Counter& c, const std::string& path="");


    /// Assignment operator
    Counter& operator = (const Counter& c) {
      setPath(c.path());
      setTitle(c.title());
      _dbn = c._dbn;
      return *this;
    }

    //@}


  public:

    /// @name Modifiers
    //@{

    /// Fill histo by value and weight
    void fill(double weight=1.0) {
      _dbn.fill(weight);
    }


    /// @brief Reset the histogram.
    ///
    /// Keep the binning but set all bin contents and related quantities to zero
    virtual void reset() {
      _dbn.reset();
    }


    /// Rescale as if all fill weights had been different by factor @a scalefactor.
    void scaleW(double scalefactor) {
      setAnnotation("ScaledBy", annotation<double>("ScaledBy", 1.0) * scalefactor);
      _dbn.scaleW(scalefactor);
    }

    //@}


    /// @name Data access
    //@{

    /// Get the number of fills
    double numEntries() const { return _dbn.numEntries(); }

    /// Get the effective number of fills
    double effNumEntries() const { return _dbn.effNumEntries(); }

    /// Get the sum of weights
    double sumW() const { return _dbn.sumW(); }

    /// Get the sum of squared weights
    double sumW2() const { return _dbn.sumW2(); }

    /// Get the error
    /// @todo Implement on Dbn0D and feed through to this and Dbn1D, 2D, etc.
    // double err() const { return _dbn.err(); }

    //@}


  public:

    /// @name Adding and subtracting counters
    //@{

    /// Add another counter to this
    Counter& operator += (const Counter& toAdd) {
      _dbn += toAdd._dbn;
      return *this;
    }

    /// Subtract another counter from this
    Counter& operator -= (const Counter& toSubtract) {
      _dbn -= toSubtract._dbn;
      return *this;
    }

    //@}


  private:

    /// @name Data
    //@{

    /// Contained 0D distribution
    Dbn0D _dbn;

    //@}

  };


  /// @name Combining counters: global operators
  //@{

  /// Add two counters
  inline Counter add(const Counter& first, const Counter& second) {
    Counter tmp = first;
    tmp += second;
    return tmp;
  }


  /// Add two counters
  inline Counter operator + (const Counter& first, const Counter& second) {
    return add(first, second);
  }


  /// Subtract two counters
  inline Counter subtract(const Counter& first, const Counter& second) {
    Counter tmp = first;
    tmp -= second;
    return tmp;
  }


  /// Subtract two counters
  inline Counter operator - (const Counter& first, const Counter& second) {
    return subtract(first, second);
  }


  // /// Divide two counters, with an uncorrelated error treatment
  // /// @todo Or just return a Point1D?
  // Scatter1D divide(const Counter& numer, const Counter& denom);


  // /// Divide two counters, with an uncorrelated error treatment
  // /// @todo Or just return a Point1D?
  // inline Scatter1D operator / (const Counter& numer, const Counter& denom) {
  //   return divide(numer, denom);
  // }


  // /// @todo Add divide functions/operators on pointers


  // /// @brief Calculate an efficiency ratio of two counters
  // ///
  // /// Note that an efficiency is not the same thing as a standard division of two
  // /// histograms: the errors must be treated as correlated
  // ///
  // /// @todo Or just return a Point1D?
  // Scatter1D efficiency(const Counter& accepted, const Counter& total);

  //@}


}

#endif