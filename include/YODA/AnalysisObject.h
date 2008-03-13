// -*- C++ -*-
#ifndef YODA_AnalysisObject_h
#define YODA_AnalysisObject_h

#include <string>
#include <map>

namespace YODA {

  /// AnalysisObject is the base class for histograms and scatters
  class AnalysisObject {

  public:

    /// Constructor giving a path and title
    AnalysisObject(const std::string& path, const std::string& title);

    /// Default destructor
    virtual ~AnalysisObject ();

    /// Get the path
    std::string getPath();

    /// Set the path returning the object with 
    void setPath(const std::string& path);

    /// Get the title
    std::string getTitle();

    /// Set the title
    void setTitle(const std::string& title);

  public:

    ///@name Annotations
    //@{
    /// Add or set an annotation by name
    void annotation (const std::string& name, const std::string& value);
    
    /// Get the annotations
    std::map<std::string,std::string> annotations () const;

    /// Get an annotation by name
    std::string annotationValue (const std::string& name) const;
    //@}

  public:

    /// Reset this analysis object
    virtual void reset () = 0;

  private:

    /// The path
    std::string _path;

    /// The title
    std::string _title;

    /// The annotations indexed by name
    std::map<std::string,std::string> _annotations;

  };

  // AnalysisObject& mv(AnalysisObject& ao, const std::string newpath);
  // AnalysisObject& cp(AnalysisObject& ao, const std::string newpath);
  // AnalysisObject& rm(AnalysisObject& ao, const std::string newpath);

}

#endif // YODA_AnalysisObject_h
