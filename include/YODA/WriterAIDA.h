// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2013 The YODA collaboration (see AUTHORS for details)
//
#ifndef YODA_WRITERAIDA_H
#define YODA_WRITERAIDA_H

#include "YODA/AnalysisObject.h"
#include "YODA/Writer.h"

#include <vector>
#include <string>
#include <ostream>


namespace YODA {


  /// @brief Persistency writer for AIDA XML format.
  class WriterAIDA : public Writer {
  public:

    /// Singleton creation function
    static Writer& create() {
      static WriterAIDA _instance;
      _instance.setPrecision(6);
      return _instance;
    }


    // Include definitions of all write methods (all fulfilled by Writer::write(...))
    #include "YODA/WriterMethods.icc"


  protected:

    void writeHeader(std::ostream& stream);
    void writeFooter(std::ostream& stream);

    void writePlot(std::ostream& stream, const Plot& p);
    void writeHisto1D(std::ostream& stream, const Histo1D& h);
    void writeHisto2D(std::ostream& stream, const Histo2D& h);
    void writeProfile1D(std::ostream& stream, const Profile1D& p);
    void writeScatter2D(std::ostream& stream, const Scatter2D& s);
    //void writeScatter3D(std::ostream& stream, const Scatter3D& s);


  private:

    /// Private since it's a singleton.
    WriterAIDA() { }

  };


}

#endif
