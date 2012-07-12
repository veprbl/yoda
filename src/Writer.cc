// -*- C++ -*-
//
// This file is part of YODA -- Yet more Objects for Data Analysis
// Copyright (C) 2008-2012 The YODA collaboration (see AUTHORS for details)
//

#include "YODA/Writer.h"
#include <iostream>
#include <typeinfo>
#include <sstream>

using namespace std;

namespace YODA {


  void Writer::write(std::ostream& stream, const AnalysisObject& ao) {
    writeHeader(stream);
    writeBody(stream, ao);
    writeFooter(stream);
  }


  void Writer::write(const std::string& filename, const AnalysisObject& ao) {
    ofstream outstream;
    outstream.open(filename.c_str());
    write(outstream, ao);
    outstream.close();
  }


  void Writer::writeBody(std::ostream& stream, const AnalysisObject& ao) {
    const string aotype = ao.type();
    if (aotype == "Plot") {
      writePlot(stream, dynamic_cast<const Plot&>(ao));
    } else if (aotype == "Histo1D") {
      writeHisto1D(stream, dynamic_cast<const Histo1D&>(ao));
    } else if (aotype == "Histo2D") {
      writeHisto2D(stream, dynamic_cast<const Histo2D&>(ao));
    } else if (aotype == "Profile1D") {
      writeProfile1D(stream, dynamic_cast<const Profile1D&>(ao));
    // } else if (aotype == "Profile2D") {
    //   writeProfile2D(stream, dynamic_cast<const Profile2D&>(ao));
    } else if (aotype == "Scatter2D") {
      writeScatter2D(stream, dynamic_cast<const Scatter2D&>(ao));
    // } else if (aotype == "Scatter3D") {
    //   writeScatter3D(stream, dynamic_cast<const Scatter3D&>(ao));
    } else {
      ostringstream oss;
      oss << "Unrecognised analysis object type " << aotype << " in Writer::write";
      throw Exception(oss.str());
    }
  }


}
