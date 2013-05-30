// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2013 The YODA collaboration (see AUTHORS for details)
//
#ifndef YODA_Dbn0D_h
#define YODA_Dbn0D_h

#include "YODA/Exceptions.h"
#include "YODA/Utils/MathUtils.h"
#include <cmath>

namespace YODA {


  /// @brief A 0D distribution
  ///
  /// This class is used internally by YODA to centralise the calculation of
  /// statistics of unbounded, unbinned sampled distributions. Each distribution
  /// fill contributes a weight, \f$ w \f$. Unlike e.g. Dbn1D there are no
  /// dimensionful value terms such as \f$ \sum wx \f$, and \f$ \sum wx^2 \f$.
  ///
  /// By storing the total number of fills (ignoring weights), \f$ \sum w \f$,
  /// and \f$ \sum w^2 \f$ the Dbn0D can calculate the mean and error on the
  /// aggregate of the supplied weights.  It is used to provide this information
  /// in the Counter class and in Dbn1D, Dbn2D, etc. (which themselves are used
  /// to implement histogram and profile bins).
  class Dbn0D {
  public:

    /// @name Constructors
    //@{

    /// Default constructor of a new distribution.
    Dbn0D() {
      reset();
    }


    /// @brief Constructor to set a distribution with a pre-filled state.
    ///
    /// Principally designed for internal persistency use.
    Dbn0D(unsigned long numEntries, double sumW, double sumW2) {
      _numFills = numEntries;
      _sumW = sumW;
      _sumW2 = sumW2;
    }


    /// Copy constructor
    ///
    /// Sets all the parameters using the ones provided from an existing Dbn0D.
    Dbn0D(const Dbn0D& toCopy) {
      _numFills = toCopy._numFills;
      _sumW = toCopy._sumW;
      _sumW2 = toCopy._sumW2;
    }


    /// Copy assignment
    ///
    /// Sets all the parameters using the ones provided from an existing Dbn0D.
    Dbn0D& operator=(const Dbn0D& toCopy) {
      _numFills = toCopy._numFills;
      _sumW = toCopy._sumW;
      _sumW2 = toCopy._sumW2;
      return *this;
    }

    //@}


    /// @name Modifiers
    //@{

    /// @brief Contribute a weight @a weight.
    ///
    /// @todo Be careful about negative weights.
    void fill(double weight=1.0);


    /// Reset the internal counters.
    void reset() {
      _numFills = 0;
      _sumW = 0;
      _sumW2 = 0;
    }


    /// Rescale as if all fill weights had been different by factor @a scalefactor.
    void scaleW(double scalefactor) {
      _sumW *= scalefactor;
      _sumW2 *= scalefactor*scalefactor;
    }

    //@}


    /// @name Raw distribution running sums
    //@{

    /// Number of entries (number of times @c fill was called, ignoring weights)
    unsigned long numEntries() const {
      return _numFills;
    }

    /// Effective number of entries \f$ = (\sum w)^2 / \sum w^2 \f$
    double effNumEntries() const {
      if (_sumW2 == 0) return 0;
      return _sumW*_sumW / _sumW2;
    }

    /// The sum of weights
    double sumW() const {
      return _sumW;
    }

    /// The sum of weights squared
    double sumW2() const {
      return _sumW2;
    }

    //@}


    /// @name Operators
    //@{

    /// Add two dbns
    Dbn0D& operator += (const Dbn0D& d) {
      return add(d);
    }

    /// Subtract one dbn from another
    Dbn0D& operator -= (const Dbn0D& d) {
      return subtract(d);
    }

    //@}


  protected:

    /// Add two dbns (internal, explicitly named version)
    Dbn0D& add(const Dbn0D&);

    /// Subtract one dbn from another (internal, explicitly named version)
    Dbn0D& subtract(const Dbn0D&);


  private:

    unsigned long _numFills;
    double _sumW;
    double _sumW2;

  };


  /// Add two dbns
  inline Dbn0D operator + (const Dbn0D& a, const Dbn0D& b) {
    Dbn0D rtn = a;
    rtn += b;
    return rtn;
  }

  /// Subtract one dbn from another
  inline Dbn0D operator - (const Dbn0D& a, const Dbn0D& b) {
    Dbn0D rtn = a;
    rtn -= b;
    return rtn;
  }


}

#endif
