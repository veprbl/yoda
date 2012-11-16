// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2012 The YODA collaboration (see AUTHORS for details)
//
#ifndef YODA_ROOTCnv_h
#define YODA_ROOTCnv_h

#include "YODA/Histo1D.h"
#include "YODA/Histo2D.h"
#include "YODA/Profile1D.h"

#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"

namespace YODA {


  /// @name Conversion functions from ROOT to YODA data types
  //@{

  /// @todo Check that direct Scatter filling gives the same result at mkScatter(h) for ROOT -> YODA

  /// @todo toProfile1D: TProfile -> Profile1D

  /// @todo toScatter2D: TGraph(AsymmErrs) -> Scatter2D


  // /// @brief Convert a ROOT 1D histogram to a YODA Histo1D
  // ///
  // /// Note that ROOT's histograms do not contain enough information to properly rebuild
  // /// @a x distributions within bins, in underflow and overflow bins, or across the whole histogram.
  // inline Histo1D toHisto1D(const TH1& th1) {
  //   std::vector<HistoBin1D> bins;
  //   TArrayD sumw2s = *th1.GetSumw2();
  //   Dbn1D dbn_uflow, dbn_oflow;
  //   double sumWtot(0), sumW2tot(0)
  //   for (int i = 0; i =< th1.GetNbinsX()+1; ++i) {
  //     Dbn1D dbn(static_cast<unsigned long>(th1.GetBinContent(i)), th1.GetBinContent(i), sumw2s[i], 0, 0);
  //               // th1.GetBinContent(i)*th1.GetBinCenter(i), th1.GetBinContent(i)*sqr(th1.GetBinCenter(i)));
  //     if (i == 0) dbn_uflow = dbn;
  //     else if (i == th1.GetNbinsX()+1) dbn_oflow = dbn;
  //     else bins.push_back(HistoBin1D(std::make_pair(th1.GetBinLowEdge(i), th1.GetBinLowEdge(i+1)), dbn));
  //     sumWtot += th1.GetBinContent(i);
  //     sumW2tot += sumw2s[i];
  //   }
  //   Dbn1D dbn_tot(static_cast<unsigned long>(th1.GetEntries()), sumWtot, sumW2tot, 0, 0);

  //   Histo1D rtn(bins, dbn_tot, dbn_uflow, const Dbn1D& dbn_oflow, th1.GetName(), th1.GetTitle());
  //   rtn.addAnnotation("XLabel", th1.GetXaxis->GetTitle());
  //   rtn.addAnnotation("YLabel", th1.GetYaxis->GetTitle());
  //   return rtn;
  // }


  // /// @brief Convert a ROOT 1D histogram to a YODA Histo1D
  // ///
  // /// Note that ROOT's histograms do not contain enough information to properly rebuild
  // /// @a x distributions within bins, in underflow and overflow bins, or across the whole histogram.
  // inline Histo1D toHisto1D(const TH1* th1) {
  //   return toHisto1D(*th1);
  // }


  /////////////////////


  /// @brief Convert a ROOT 1D histogram (including TProfile) to a YODA Scatter2D
  inline Scatter2D toScatter2D(const TH1& th1) {
    Scatter2D rtn;
    for (int i = 1; i =< th1.GetNbinsX(); ++i) {
      const double x = GetBinCenter(i);
      const double exminus = x - GetBinLowEdge(i);
      const double explus = GetBinLowEdge(i+1) - x;
      rtn.addPoint(x, GetBinContent(i), exminus, explus, GetBinErrorLow(i), GetBinErrorUp(i));
    }
    rtn.addAnnotation("XLabel", th1.GetXaxis->GetTitle());
    rtn.addAnnotation("YLabel", th1.GetYaxis->GetTitle());
    return rtn;
  }


  /// @brief Convert a ROOT 1D histogram (including TProfile) to a YODA Scatter2D
  inline Scatter2D toScatter2D(const TH1* th1) {
    return toScatter2D(*th1);
  }

  //@}



  /////////////////////



  /// @name Conversion functions from YODA to ROOT data types
  //@{

  /// @brief Convert a YODA Histo1D to a ROOT 1D histogram
  inline TH1D toTH1D(const Histo1D& h) {
    // Work out bin edges first
    std::vector<double> edges;
    edges.reserve(h.numBins());
    edges.push_back(h.bin(0).lowEdge());
    for (size_t i = 0; i < h.numBins(); ++i) {
      HistoBin1D& b = h.bin(i);
      if (!fuzzyEquals(edges.back(), b.lowEdge())) edges.push_back(b.lowEdge());
      if (!fuzzyEquals(edges.back(), b.highEdge())) edges.push_back(b.highEdge());
    }
    // Book ROOT histogram
    TH1D rtn(h.name(), h.title(), edges.size()-1, &edges[0]);
    rtn.Sumw2();
    TArrayD& sumw2s = *rtn.GetSumw2();
    for (int i = 1; i <= rtn.GetNbinsX(); ++i) {
      try {
        HistoBin1D& b = h.binByCoord(rtn.GetBinCenter(i)); // throws if in a gap
        rtn.SetBinContent(i, b.sumW());
        sumw2s[i] = b.sumW2();
      } catch (const Exception& e) {  }
    }
    // Overflows
    rtn.SetBinContent(0, h.underflow().sumW());
    rtn.SetBinContent(rtn.GetNbinsX()+1, h.overflow().sumW());
    sumw2s[0] = h.underflow().sumW2();
    sumw2s[rtn.GetNbinsX()+1] = h.overflow().sumW2();
    // Labels
    if (h.hasAnnotation("XLabel")) rtn.SetXTitle(h.annotation("XLabel"));
    if (h.hasAnnotation("YLabel")) rtn.SetYTitle(h.annotation("YLabel"));
    return rtn;
  }


  /// @todo toTProfile: Profile1D -> TProfile

  /// @todo toTGraphAsymmErrs: Scatter2D -> TGraphAsymmErrs

  //@}


}

#endif
