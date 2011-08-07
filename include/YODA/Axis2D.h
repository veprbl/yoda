#ifndef YODA_Axis2D_h
#define YODA_Axis2D_h

#include "YODA/AnalysisObject.h"
#include "YODA/Exceptions.h"
#include "YODA/Bin.h"
#include "YODA/Utils/sortedvector.h"
#include "YODA/Utils/cachedvector.h"
#include "YODA/Utils/MathUtils.h"
#include "YODA/Dbn2D.h"
#include <string>
#include <cassert>
#include <cmath>
#include <algorithm>

using namespace std;

/// A big number for the low/high search:
/// @todo This isn't good enough! Why would this be guaranteed to be
/// larger than the axis scale? Use e.g. std::limits<double> and/or determine
/// the true extent when booking.
/// @todo Also, the code convention is that _foo is a private member variable/function:
/// for constants use all-caps, e.g. LARGENUM.
const double _largeNum = 1000000000000000000000.0;


namespace YODA {
    /// @brief 2D bin container and provider
    /** This class handles almost all boiler-plate operations
      * on 2D bins (like creating axis, adding, searching, testing).
      */
    template <typename BIN>
    class Axis2D {
    public:
        
        /// A collection of helpful typedefs
        typedef BIN Bin;
        typedef typename std::vector<BIN> Bins;
        
        /** When edge is added to the collection it must
          * obey the following format. size_t specifies the bin this
          * edge is a member of, a pair contains a beginning and end of the edge.
          */
        typedef typename std::pair<size_t, std::pair<double,double> > Edge;

        /** A type being a basic substructure of _binHashSparse. It contains a indicator
          * specifying the major coordinate and a collection of edges sharing the same major
          * coordinate.
          */
        typedef typename std::pair<double, std::vector<Edge> > EdgeCollection;

        /// A simple point in 2D @todo Should Point2D be used?
        typedef typename std::pair<double, double> Point;

        /// Segment, having a beginning and end.
        typedef typename std::pair<Point, Point> Segment;

    private:

        /// @brief Segment validator function
        /** This a 'dispatcher' function. It checks if the segment in question
          * is vertical or horizontal and launches the appropriate function 
          * searching for cuts in the prope direction. Since it operates on 
          * a vector of segments it is prepared to act on arbitrarly large sets of edges,
          * in practice usually being four sides of a rectangular bin.
          *
          * Notice that it will never be checked, in the current state, if there is a cut 
          * in edges in the edgeset. This imposes the requirement of provide the program
          * with non-degenerate bins, until checking in implemented. However, it doesn't seem 
          * to be needed, as edges are not generated by a user.
          * 
          * This is also a perfect place to paralellize the program, if required.
          */
        bool _validateEdge(vector<Segment>& edges)         
        {
            /// Setting the return variable. True means that no cuts were detected.
            bool ret = true;

            /// Looping over all the edges provided
            for(unsigned int i=0; i < edges.size(); i++) {
                /** If the X coordinate of the starting point is the same
                  * as X coordinate of the ending one, checks if there are cuts
                  * on this vertical segment.
                  */
                if(fuzzyEquals(edges[i].first.first, edges[i].second.first)) ret =  _findCutsY(edges[i]);
                
                /// Check if the segment is horizontal and is it cutting any bin that already exists
                else if(fuzzyEquals(edges[i].first.second, edges[i].second.second)) ret =  _findCutsX(edges[i]);
                
                /** This is a check that discards the bin if it is not a rectangle
                  * composed of vertical and horizontal segments. 
                  */
                else ret = false;
                
                /** If a cut was detected, say it. There is no point in checking other edges
                  * in the set.
                  */
                if(!ret) return false;
            }
            /// If no cuts were detected in any of the edges, tell the launching function about this
            return true;
        }
        
        /// @brief Inclusion checker
        /** Be aware that it works according to principle:
          * always fast, almost always right.
          */
        bool _checkInclusion(vector<Segment>& edges)
        {
            pair<Utils::cachedvector<EdgeCollection>, Utils::cachedvector<EdgeCollection> > binHash = _binHashSparse;

            double smallNum = 0.00001;

            if(edges.size() == 4) {
                _addEdge(edges, binHash, false);
                if (_findBinIndex(edges[1].second.first-smallNum, edges[1].second.second-smallNum, binHash) == -1) {
                    cout << " true1 ";
                    return true;
                }
            }

            for(unsigned int i=0; i < _bins.size(); i++) {
                int result = _findBinIndex(_bins[i].lowEdgeX(), _bins[i].highEdgeY()-smallNum, binHash);
                if (result == -1 || (unsigned int)result != i){
                    cout << " true2 " << i << " ";
                    return true;
                }
            }

            return false;
        }

        
        /// @brief A binary search function
        /** This is conceptually the same implementation as in STL
          * but because it returns the index of an element in a vector,
          * it is easier to use in our case than the STL implementation
          * that returns a pointer at the element.
          */
        size_t _binaryS(Utils::cachedvector<EdgeCollection>& toSearch, 
                        double value, size_t lower, size_t higher) 
        {
            /// Just a check if such degenerate situation happens
            if(lower == higher) return lower;

            /// Choose a midpoint that will be our pivot
            size_t where = (higher+lower)/2;

            /// Launch the same procedure on half of the range above the pivot
            if(value >= toSearch[where].first) {
                if(where == toSearch.size() - 1) return where;
                if(value <= toSearch[where+1].first) return where;
                return _binaryS(toSearch, value, where, higher);
            }

            /** This is not a redundant check, because
              * of the nature of int division.
              */
            if (where == 0) return where;

            /** Check if the value is somewhere inbetween
              * an element at the  position in question and 
              * an element at a lower position. If so, return
              * an index to the current positon.
              */
            if(value >= toSearch[where-1].first) return where;

            /** If none of the above occurs, the value must
              * be smaller that the element at the current 
              * position. In such case, launch the search on
              * half of the interval below the current position.
              */
            return _binaryS(toSearch, value, lower, where);
        }
        
        /// @brief Function that finds cuts of horizontal edges.
        /** A specialised function that tries to exploit the fact
          * that edges can cut one another only on right angles 
          * to the highest extent. The inner workings are explained
          * in the comments placed in the function body.
          */
        bool _findCutsX(Segment& edge) {
            /** Look up the limits of search in the _binHashSparse
              * structure. We are not interested in the vertical edges
              * that are before the beginning of our segment or after its end. 
              */
            size_t i = _binaryS(_binHashSparse.second, edge.first.first, 0, _binHashSparse.second.size());
            size_t end = _binaryS(_binHashSparse.second, edge.second.first, 0, _binHashSparse.second.size());

            for(; i < end; i++) {
                    
                    /** Scroll through all the vertical segments with a given X coordinate
                      * and see if any of those fulfills the cutting requirement. If it does,
                      * announce it.
                      */
                    for(unsigned int j = 0; j < _binHashSparse.second[i].second.size(); j++) {
                        /** Note that we are not taking into account the edges touching the 
                          * segment in question. That's because sides of a bin touch
                          */
                        if(_binHashSparse.second[i].second[j].second.first < edge.first.second &&
                           _binHashSparse.second[i].second[j].second.second > edge.first.second &&
                           !fuzzyEquals(_binHashSparse.second[i].second[j].second.first, edge.first.second) &&
                           !fuzzyEquals(_binHashSparse.second[i].second[j].second.second, edge.first.second)) {
                            return false;
                        }
                    }
                }
            /// If none of the existing edges is cutting this edge, announce it
            return true;
        }

        /// @brief Function that finds cuts of vertical edges
        /** For a detailed descpription, please look into
          * documentation for _findCutsX().
          */
        bool _findCutsY(Segment& edge) {
            size_t i = _binaryS(_binHashSparse.first, edge.first.second, 0, _binHashSparse.first.size());
            size_t end = _binaryS(_binHashSparse.first, edge.second.second, 0, _binHashSparse.first.size());
            for(; i < end; i++) {

                    for(unsigned int j = 0; j < _binHashSparse.first[i].second.size(); j++) {
                        if(_binHashSparse.first[i].second[j].second.first < edge.first.first &&
                           _binHashSparse.first[i].second[j].second.second > edge.first.first &&
                           !fuzzyEquals(_binHashSparse.first[i].second[j].second.first, edge.first.first) &&
                           !fuzzyEquals(_binHashSparse.first[i].second[j].second.second, edge.first.first)) {
                            return false;
                        }
                    }
                }
            return true;
        }

        /// @brief Function executed when a set of edges is dropped.
        /** It does not have any information about which edge in the set
          * had failed the check. If this is needed such additional information
          * can be readily implemented.
          */
        void _dropEdge(vector<Segment>& edges) {
            std::cerr << "A set of edges was dropped." << endl;
        }

        /// @brief Bin adder.
        /** It contains all the commands that need to executed
          * to properly add a bin. Specifially edges are added to 
          * the edge cache (_binHashSparse) and a bin is created from
          * those edges.
          */
        void _addEdge(vector<Segment>& edges, pair<Utils::cachedvector<EdgeCollection>, 
                      Utils::cachedvector<EdgeCollection> >& binHash, bool addBin = true) {
            /// Check if there was no mistake made when adding segments to a vector.
            if(edges.size() != 4) throw Exception("The segments supplied don't describe a full bin!");

            /** This is the part in charge of adding each of the segments
              * to the edge cache. Segments are assumed to be validated 
              * beforehand.
              */
            for(unsigned int j=0; j < edges.size(); j++) {
                /// Association made for convinience.
                Segment edge = edges[j];

                /** Do the following if the edge is vertical.
                  * Those two cases need to be distinguished 
                  * because of the way in which edge cache is structured.
                  */
                if(edge.first.first == edge.second.first) {
                    /** See if our edge has the same X coordinate as any other
                      * edge that is currently in the cache.
                      */

                    /// Keeps the status of the search
                    bool found = false;

                    /** There is only a certain set of X coordinates that we need to sweep
                      * to check if our segment has the same X coordinate. Find them.
                      */
                    size_t i = _binaryS(binHash.second, edge.first.first, 0, binHash.second.size())-1;
                    if(i < 0) i = 0;
                    size_t end = i+3;

                    /** For the coordinates in range, check if one of them is an X coordinate of
                      * the sement.
                      */
                    for(; i < binHash.second.size() && i < end ; i++) {
                        /// If this is the case, do what is needed to be done.
                        if(fuzzyEquals(binHash.second[i].first, edge.first.first)) {
                            binHash.second[i].second.push_back(make_pair(_bins.size(),make_pair(edge.first.second, edge.second.second)));
                            found = true;
                            break;
                        }
                    }

                    /** If no edge with the same X coordinate exist, create
                      * a new subhash at the X coordinate of a segment.
                      */
                    if(!found) {
                        vector<Edge> temp;
                        temp.push_back(make_pair(_bins.size(), make_pair(edge.first.second, edge.second.second)));
                        binHash.second.push_back(make_pair(edge.first.first,temp));
                        sort(binHash.second.begin(), binHash.second.end());
                    }
                }
                
                /// See the vertical case for description of a horizontal one
                else if(edge.first.second == edge.second.second) {
                    bool found = false;
                    size_t i = _binaryS(binHash.first, edge.first.second, 0, binHash.first.size())-1;
                    if(i < 0) i = 0;
                    size_t end = i+3;
                    for(; i < binHash.first.size() && i < end; i++) {
                        if(fuzzyEquals(binHash.first[i].first, edge.first.second)) {
                            binHash.first[i].second.push_back(make_pair(_bins.size(),make_pair(edge.first.first, edge.second.first)));
                            found = true;
                        }
                    }
                    if(!found) {
                        vector<Edge> temp;
                        temp.push_back(make_pair(_bins.size(), make_pair(edge.first.first, edge.second.first)));
                        binHash.first.push_back(make_pair(edge.second.second, temp));
                        sort(binHash.first.begin(), binHash.first.end());
                    }
                }
            }
            /// Now, create a bin with the edges provided
            if(addBin) _bins.push_back(BIN(edges));
        }

        /// @brief Orientation fixer
        /** Check if the orientation of an edge is proper
          * for the rest of the algorithm to work on, and if it is not
          * fix it.
          */
        void _fixOrientation(Segment& edge) {
            if(fuzzyEquals(edge.first.first, edge.second.first)) {
                if(edge.first.second > edge.second.second) {
                    double temp = edge.second.second;
                    edge.second.second = edge.first.second;
                    edge.first.second = temp;
                }
            }
            else if(edge.first.first > edge.second.first) {
                double temp = edge.first.first;
                edge.first.first = edge.second.first;
                edge.second.first = temp;
            }
        }
        
        /// @brief Axis creator
        /** The top-level function taking part in the process of
          * adding edges. Creating an axis is the same operation 
          * for it as adding new bins so it can be as well used to 
          * add some custom bins.
          *
          * It accepts two extremal points of a rectangle 
          * (top-right and bottom-left) as input.
          */
        void _mkAxis(const vector<Segment>& binLimits) {
            
            /// For each of the rectangles
            for(unsigned int i=0; i < binLimits.size(); i++) {
                /// Produce the segments that a rectangle is composed of
                Segment edge1 =
                    make_pair(binLimits[i].first,
                              make_pair(binLimits[i].first.first, binLimits[i].second.second));
                Segment edge2 =
                    make_pair(make_pair(binLimits[i].first.first, binLimits[i].second.second),
                              binLimits[i].second);
                Segment edge3 =
                    make_pair(make_pair(binLimits[i].second.first, binLimits[i].first.second),
                              binLimits[i].second);
                Segment edge4 =
                    make_pair(binLimits[i].first,
                              make_pair(binLimits[i].second.first, binLimits[i].first.second));

                /// Check if they are made properly
                _fixOrientation(edge1); _fixOrientation(edge2);
                _fixOrientation(edge3); _fixOrientation(edge4);

                /// Add all the segments to a vector
                vector<Segment> edges;
                edges.push_back(edge1); edges.push_back(edge2);
                edges.push_back(edge3); edges.push_back(edge4);

                /// And check if a bin is a proper one, if it is, add it.
                if(_validateEdge(edges))  _addEdge(edges, _binHashSparse);
                else _dropEdge(edges);

            }

            /// Setting all the caches
            _binHashSparse.first.regenCache();
            _binHashSparse.second.regenCache();
            _regenDelimiters();

        }

        /// @brief Plot extrema (re)generator.
        /** Since scrolling through every bin is an expensive 
          * operation to do every time we need the limits of 
          * the plot, there are caches set up. This function
          * regenerates them. It should be run after any change is made 
          * to bin layout.
          */
        void _regenDelimiters() {
            double highEdgeX = -_largeNum;
            double highEdgeY = -_largeNum;
            double lowEdgeX = _largeNum;
            double lowEdgeY = _largeNum;
            
            /// Scroll through the bins and set the delimiters.
            for(unsigned int i=0; i < _bins.size(); i++) {
                if(_bins[i].xMin() < lowEdgeX) lowEdgeX = _bins[i].xMin();
                if(_bins[i].xMax() > highEdgeX) highEdgeX = _bins[i].xMax();
                if(_bins[i].yMin() < lowEdgeY) lowEdgeY = _bins[i].yMin();
                if(_bins[i].yMax() > highEdgeY) highEdgeY = _bins[i].yMax();
            }

            _lowEdgeX = lowEdgeX;
            _highEdgeX = highEdgeX;
            _lowEdgeY = lowEdgeY;
            _highEdgeY = highEdgeY;
        }
        
        /// @brief Bin index finder
        /** This version of findBinIndex is searching for an edge on the left
         *  that is enclosing the point and then finds an edge on the bottom
         *  that does the same and if it finds two edges that are a part of
         *  the same square it returns that it had found a bin. If no bin is
         *  found, ie. (coordX, coordY) is a point in empty space -1 is returned.
         */
        int _findBinIndex(double coordX, double coordY, const pair<Utils::cachedvector<EdgeCollection>,
                         Utils::cachedvector<EdgeCollection> >& binHash) const {
            /** It is need to apply this trick not to have a coordinate 
              * pointing directly on the edge. Notice that this is lower that 
              * fuzzyEquals() tolerance.
              */
            coordX += 0.0000000001; coordY += 0.00000000001;
            //cout << "YO";

            size_t indexY = (*binHash.first._cache.lower_bound(approx(coordY))).second;

            if(indexY < binHash.first.size()) {
                for(unsigned int i = 0;  i < binHash.first[indexY].second.size(); i++){
                    if(binHash.first[indexY].second[i].second.first < coordX &&
                       binHash.first[indexY].second[i].second.second > coordX){
                        size_t indexX = (*binHash.second._cache.lower_bound(approx(coordX))).second;
                        if(indexX < binHash.second.size()){
                            for(unsigned int j=0; j < binHash.second[indexX].second.size(); j++) {
                                if(binHash.second[indexX].second[j].second.first < coordY &&
                                   (binHash.second[indexX].second[j].second.second > coordY) &&
                                   (binHash.second[indexX].second[j].first ==
                                   binHash.first[indexY].second[i].first))
                                    return binHash.second[indexX].second[j].first;
                            }
                        }
                    }
                }
            }
            return -1;
        }

    public:
        /// @name Constructors:
        //@{

        /// @brief Empty constructor
        /** Only added because it is required by SWIG. 
          * It doesn't make much sense to use it.
          */
        Axis2D() 
        {
            vector<Segment> edges;
            _mkAxis(edges);
        }
        
        /// Constructor provided with a vector of bin delimiters
        Axis2D(const vector<Segment>& binLimits) 
        {
            _mkAxis(binLimits);
        }

        ///Most standard constructor, should be self-explanatory
        Axis2D(size_t nbinsX, double lowerX, double upperX, size_t nbinsY, double lowerY, double upperY) 
        {
            vector<Segment> binLimits;
            double coeffX = (upperX - lowerX)/(double)nbinsX;
            double coeffY = (upperY - lowerX)/(double)nbinsY;

            for(double i=lowerX; i < upperX; i+=coeffX) {
                for(double j=lowerY; j < upperY; j+=coeffY) {
                    binLimits.push_back(make_pair(make_pair(i, j),
                                              make_pair((double)(i+coeffX), (double)(j+coeffY))));
                }
            }
            _mkAxis(binLimits);
        }
        //@}

        /// @name Addition operators:
        //@{
        
        /// @brief Bin addition operator
        /** This operator is provided a vector of limiting 
          * points in the format required by _mkAxis().
          * It should be noted that there is nothing special about 
          * the initiation stage of Axis2D, and the edges can be added 
          * online if they meet all the requirements of non-degeneracy. 
          * No merging is supported, and I don't think it should before the support
          * for merging for '+' operator (codewise it should be the same thing).
          */
        void addBin(const vector<Segment>& binLimits) 
        {
            _mkAxis(binLimits);
        }
        
        /// @brief Bin addition operator
        /** This operator is supplied with whe extreamal coordinates of just
          * one bin. It then launches the standard bin addition procedure.
          */
        void addBin(double lowX, double lowY, double highX, double highY) 
        {
            vector<Segment> coords;
            coords.push_back(make_pair(make_pair(lowX, lowY), make_pair(highX, highY)));

            addBin(coords);
        }
        //@}

        /// @name Some helper functions:
        //@{


        /// @brief Checks if our bins form a grid.
        /** This function uses a neat property of _binHashSparse.
          * If it is containing a set of edges forming a grid without
          * gaps in the middle it will have the same number of edges in the 
          * inner subcaches and half of this amount in the outer (grid boundary) 
          * subcaches. This makes isGriddy() a very, very fast function.
          */
        int isGriddy() 
        {
            
            /** Check if the number of edges parallel to X axis
              * is proper in every subcache.
              */
            unsigned int sizeX = _binHashSparse.first[0].second.size();
            for(unsigned int i=1; i < _binHashSparse.first.size(); i++) {
                if(i == _binHashSparse.first.size() - 1) {
                    if(_binHashSparse.first[i].second.size() != sizeX) {
                        return -1;
                    }
                }
                else if(_binHashSparse.first[i].second.size() != 2*sizeX) {
                    return -1;
                }
            }

            /// Do the same for edges parallel to Y axis.
            unsigned int sizeY = _binHashSparse.second[0].second.size();
            for(unsigned int i=1; i < _binHashSparse.second.size(); i++) {
                if(i!= _binHashSparse.second.size() - 1) {
                    if(2*sizeY != _binHashSparse.second[i].second.size()) {
                        return -1;
                    }
                }
                else if(_binHashSparse.second[i].second.size() != sizeY) return -1;
            }

            /// If everything is proper, announce it.
            return 0;
        }
        
        /// @brief Check if no bin is included in the other one
        /** For a bit more detailed description, plese see 
          * _checkInclusion().
          */
        bool checkInclusion()
        {
            vector<Segment> temp;
            return _checkInclusion(temp);
        }

        /// Return a total number of bins in a Histo
        unsigned int numBinsTotal() const 
        {
            return _bins.size();
        }

        /// Get inf(X) (non-const version)
        double lowEdgeX() 
        {
            return _lowEdgeX;
        }

        /// Get sup(X) (non-const version)
        double highEdgeX() 
        {
            return _highEdgeX;
        }

        /// Get inf(Y) (non-const version)
        double lowEdgeY() 
        {
            return _lowEdgeY;
        }

        /// Get sup(Y) (non-const version)
        double highEdgeY() 
        {
            return _highEdgeY;
        }

        /// Get inf(X) (const version)
        const double lowEdgeX() const 
        {
            return _lowEdgeX;
        }

        /// Get sup(X) (const version)
        const double highEdgeX() const 
        {
            return _highEdgeX;
        }

        /// Get inf(Y)
        const double lowEdgeY() const 
        {
            return _lowEdgeY;
        }

        ///Get sup(Y)
        const double highEdgeY() const 
        {
            return _highEdgeY;
        }

        /// Get the bins from an Axis (non-const version)
        Bins& bins() 
        {
            return _bins;
        }

        /// Get the bins from an Axis (const version)
        const Bins& bins() const 
        {
            return _bins;
        }

        /// Get a bin with a given index (non-const version)
        BIN& bin(size_t index) 
        {
            if(index >= _bins.size()) throw RangeError("YODA::Histo2D: index out of range");
            return _bins[index];
        }

        /// Get a bin with a given index (const version)
        const BIN& bin(size_t index) const
        {
            if(index >= _bins.size()) throw RangeError("YODA::Histo2D: index out of range");
            return _bins[index];
        }

        /// Get a bin at given coordinates (non-const version)
        BIN& binByCoord(double x, double y) 
        {
            int ret = _findBinIndex(x, y, _binHashSparse);
            if(ret != -1) return bin(ret);
            else throw RangeError("No bin found!!");
        }

        /// Get a bin at given coordinates (const version)
        const BIN& binByCoord(double x, double y) const 
        {
            int ret = _findBinIndex(x, y, _binHashSparse);
            if(ret != -1) return bin(ret);
            else throw RangeError("No bin found!!");
        }

        /// Get a bin at given coordinates (non-const version)
        BIN& binByCoord(pair<double, double>& coords) 
        {
            int ret = _findBinIndex(coords.first, coords.second, _binHashSparse);
            if(ret != -1) return bin(ret);
            else throw RangeError("No bin found!!");
        }

        /// Get a bin at given coordinates (const version)
        const BIN& binByCoord(pair<double, double>& coords) const 
        {
            int ret = _findBinIndex(coords.first, coords.second, _binHashSparse);
            if(ret != -1) return bin(ret);
            else throw RangeError("No bin found!!");
        }

        /// Get a total distribution (non-const version)
        Dbn2D& totalDbn() 
        {
            return _dbn;
        }

        /// Get a total distribution (const version)
        const Dbn2D& totalDbn() const
        {
            return _dbn;
        }

        /// Get the overflow distribution (non-const version)
        Dbn2D& overflow() 
        {
            return _overflow;
        }

        /// Get the overflow distribution (const version)
        const Dbn2D& overflow() const 
        {
            return _overflow;
        }

        /// Get the underflow distribution (non-const version)
        Dbn2D& underflow() 
        {
            return _underflow;
        }

        /// Get the underflow distribution (const version)
        const Dbn2D& underflow() const 
        {
            return _underflow;
        }

        /// Get the binHash(non-const version)
        std::pair<Utils::cachedvector<EdgeCollection>, 
                  Utils::cachedvector<EdgeCollection> > getHash() 
        {
            return _binHashSparse;
        }

        /// Get the binHash(const version)
        const std::pair<Utils::cachedvector<EdgeCollection>,
                  Utils::cachedvector<EdgeCollection > > getHash() const 
        {
            return _binHashSparse;
        }

        /// Get bin index from external classes (non-const version)
        int getBinIndex(double coordX, double coordY) 
        {
            return _findBinIndex(coordX, coordY, _binHashSparse);
        }
        
        /// Get bin index from external classes (const version)
        const int getBinIndex(double coordX, double coordY) const
        {
            return _findBinIndex(coordX, coordY, _binHashSparse);
        }

        /// Resetts the axis statistics ('fill history')
        void reset() 
        {
            _dbn.reset();
            _underflow.reset();
            _overflow.reset();
            for (size_t i=0; i<_bins.size(); i++) _bins[i].reset();
        }

        /// @brief Axis scaler
        /** Scales the axis with a given scale. If no scale is given, assumes 
          * identity transform.
          */
        void scale(double scaleX = 1.0, double scaleY = 1.0) 
        {
            // Two loops are put on purpose, just to protect
            // against improper _binHashSparse
            for(unsigned int i=0; i < _binHashSparse.first.size(); i++) {
                _binHashSparse.first[i].first *= scaleY;
                for(unsigned int j=0; j < _binHashSparse.first[i].second.size(); j++){
                    _binHashSparse.first[i].second[j].second.first *=scaleX;
                    _binHashSparse.first[i].second[j].second.second *=scaleX;
                }
            }
            for(unsigned int i=0; i < _binHashSparse.second.size(); i++) {
                _binHashSparse.second[i].first *= scaleX;
                for(unsigned int j=0; j < _binHashSparse.second[i].second.size(); j++){
                    _binHashSparse.second[i].second[j].second.first *=scaleY;
                    _binHashSparse.second[i].second[j].second.second *=scaleY;
                }
            }

            /// Regenerate the bin edges cache.
            _binHashSparse.first.regenCache();
            _binHashSparse.second.regenCache();

            /// Now, as we have the map rescaled, we need to update the bins
            for(unsigned int i=0; i < _bins.size(); i++) _bins[i].scale(scaleX, scaleY);
            _dbn.scale(scaleX, scaleY);
            _underflow.scale(scaleX, scaleY);
            _overflow.scale(scaleX, scaleY);

            //And making sure that we have correct boundaries set after rescaling
            _regenDelimiters();
        }

        /// Scales the heights of the bins
        void scaleW(double scalefactor) 
        {
            _dbn.scaleW(scalefactor);
            _underflow.scaleW(scalefactor);
            _overflow.scaleW(scalefactor);
            for (unsigned int i=0; i<_bins.size(); i++) _bins[i].scaleW(scalefactor);
        }
       //@}

       /// @name Operators:
       //@{

        /// Equality operator
        bool operator == (const Axis2D& other) const 
        {
            return _binHashSparse == other._binHashSparse;
        }

        /// Non-equality operator
        bool operator != (const Axis2D& other) const 
        {
            return ! operator == (other);
        }

        /// @brief Addition operator
        /** At this stage it is only possible to add two histograms with
          * the same binnings. Compatible but not equal binning to come soon.
          */
        Axis2D<BIN>& operator += (const Axis2D<BIN>& toAdd) 
        {
            if (*this != toAdd) {
                throw LogicError("YODA::Histo1D: Cannot add axes with different binnings.");
            }
            for (unsigned int i=0; i < bins().size(); i++) bins().at(i) += toAdd.bins().at(i);

            _dbn += toAdd._dbn;
            _underflow += toAdd._underflow;
            _overflow += toAdd._overflow;
            return *this;
        }

        /// Substraciton operator
        Axis2D<BIN>& operator -= (const Axis2D<BIN>& toSubstract) 
        {
            if (*this != toSubstract) {
                throw LogicError("YODA::Histo1D: Cannot add axes with different binnings.");
            }
            for (unsigned int i=0; i < bins().size(); i++) bins().at(i) -= toSubstract.bins().at(i);

            _dbn -= toSubstract._dbn;
            _underflow -= toSubstract._underflow;
            _overflow -= toSubstract._overflow;
            return *this;
        }
        //@}

    private:

        /// Bins contained in this histogram
        Bins _bins;

        /// Underflow distribution
        Dbn2D _underflow;

        /// Overflow distribution
        Dbn2D _overflow;

        /// The total distribution
        Dbn2D _dbn;

        /// @brief Bin hash structure
        /** First in pair is holding the horizontal edges indexed by first.first
          * which is an y coordinate. The last pair specifies x coordinates (begin, end) of
          * the horizontal edge.
          * Analogous for the second member of the pair.
          *
          * For the fullest description, see typedefs at the beginning 
          * of this file.
          */
        std::pair<Utils::cachedvector<EdgeCollection>,
                  Utils::cachedvector<EdgeCollection> >
                  _binHashSparse;

        /// Low/High edges:
        double _highEdgeX, _highEdgeY, _lowEdgeX, _lowEdgeY;

   };

    /// Additon operator
    template <typename BIN>
    Axis2D<BIN> operator + (const Axis2D<BIN>& first, const Axis2D<BIN>& second) 
    {
        Axis2D<BIN> tmp = first;
        tmp += second;
        return tmp;
    }

    /// Substraciton operator
    template <typename BIN>
    Axis2D<BIN> operator - (const Axis2D<BIN>& first, const Axis2D<BIN>& second) 
    {
        Axis2D<BIN> tmp = first;
        tmp -= second;
        return tmp;
    }


}

#endif
