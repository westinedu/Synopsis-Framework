/**
* (c) Daniel Lemire, 2008
*This C++ library implements fast nearest-neighbor retrieval under the dynamic time warping (DTW).
* This library includes the dynamic programming solution, a fast implementation of the LB_Keogh
*  lower bound, as well as a improved lower bound called LB_Improved. This library was used to show
*  that LB_Improved can be used to retrieve nearest neighbors three times on several data sets
* including random walks time series or shape time series.
*
* See the classes LB_Keogh and LB_Improved for usage.
*
* Time series are represented using STL vectors.
*/

#ifndef DTW
#define DTW

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include <cassert>
#include <sstream>

// was
//typedef double floattype;
typedef float floattype;
typedef unsigned int uint;
using namespace std;

class MathUtil {
  public:
    static inline int min (int x, int y ) { return x < y ? x : y;}
    static inline int max (int x, int y ) { return x > y ? x : y;}
};

/**
* You do not need this class, used by LB_Keogh and LB_Improved later.
* It computes the DTW using dynamic programmming.
*/
class dtw {
    public:
        
enum{ verbose = false,
   vverbose = false, INF = 100000000, fast=true};
static inline floattype max (floattype x, floattype y ) { return x > y ? x : y;}
static inline floattype min (floattype x, floattype y ) { return x < y ? x : y;}

    dtw(uint n, uint constraint): mGamma(n,vector<floattype>(n,INF)),mN(n),mConstraint(constraint) {
    }
    /*
    * hardcoded l1 norm (for speed)
    */
    inline floattype fastdynamic(const vector<floattype> &  v, const vector<floattype> & w) {
        if(! fast) return dynamic(v,w,mConstraint,1);
        assert(static_cast<int>(v.size()) == mN);
        assert(static_cast<int>(w.size()) == mN);
        assert(static_cast<int>(mGamma.size()) == mN);
        floattype Best(INF);
        for (int i = 0; i < mN;++i) {
            assert(static_cast<int>(mGamma[i].size()) == mN);
            for (int j = max(0,i-mConstraint); j < min(mN,i+mConstraint+1);++j) {
                Best = INF;
                if(i>0) Best = mGamma[i-1][j];
                if(j>0) Best = min(Best, mGamma[i][j-1]);
                if( (i > 0) && (j > 0) )
                 Best = min(Best, mGamma[i-1][j-1]);
                if((i==0) && (j==0))
                  mGamma[i][j] = fabs(v[i]-w[j]);
                else
                  mGamma[i][j] = Best + fabs(v[i]-w[j]);
            }
        }
         return mGamma[mN-1][mN-1];
    }

    vector<vector<floattype> > mGamma;

    int mN, mConstraint;

    static inline floattype dynamic(const vector<floattype> &  v, const vector<floattype> & w,
      int constraint=INF, int p=2) {
          assert(v.size() == w.size());
        int n ( (int)v.size() );
        vector<vector<floattype> > gamma(n, vector<floattype>(n,0.0));
        for (int i = 0; i < n;++i) {
            for (int j = 0; j < n;++j) {
                if(abs(i-j) > constraint) {
                  gamma[i][j] = INF; continue;
                }
                vector<floattype> previous(0);
                if(i>0) previous.push_back(gamma[i-1][j]);
                if(j>0) previous.push_back(gamma[i][j-1]);
                if( (i > 0) && (j > 0) )
                 previous.push_back(gamma[i-1][j-1]);
                floattype smallest = *min_element(previous.begin(),previous.end());
                if (p != INF)
                  gamma[i][j] = pow(fabs(v[i]-w[j]),p)+smallest;
                else
                  gamma[i][j] = max(fabs(v[i] - w[j]), smallest);
                   
            }
        }
        if(p != INF)
         return pow(gamma[n-1][n-1],1.0/p);
        else
         return gamma[n-1][n-1];
      }
};

/**
* You do not need this function, used by LB_Keogh and LB_Improved later.
*/
void computeEnvelope(const vector<floattype> & array, uint constraint, vector<floattype> & maxvalues, vector<floattype> & minvalues) {
        uint width = 1+  2 * constraint;
        deque<int> maxfifo, minfifo;
        maxfifo.push_back(0);
        minfifo.push_back(0);
        for(uint i = 1; i < array.size(); ++i) {
          if(i >=constraint+1) {
            maxvalues[i-constraint-1] = array[maxfifo.front()];
            minvalues[i-constraint-1] = array[minfifo.front()];
          }
          if(array[i] > array[i-1]) { //overshoot
            maxfifo.pop_back();
            while(maxfifo.size() > 0) {
              if(array[i] <= array[maxfifo.back()]) break;
              maxfifo.pop_back();
            }
          } else {
            minfifo.pop_back();
            while(minfifo.size() > 0) {
              if(array[i] >= array[minfifo.back()]) break;
              minfifo.pop_back();
            }
          }
          maxfifo.push_back(i);
          minfifo.push_back(i);
         if(i==  width+maxfifo.front()) maxfifo.pop_front();
         else if(i==  width+minfifo.front()) minfifo.pop_front();
        }
        for(uint i = (uint)array.size(); i <= array.size() + constraint; ++i) {
              maxvalues[i-constraint-1] = array[maxfifo.front()];
              minvalues[i-constraint-1] = array[minfifo.front()];
              if(i-maxfifo.front() >= width) maxfifo.pop_front();
              if(i-minfifo.front() >= width) minfifo.pop_front();
        }
}

class NearestNeighbor {
    public:
        NearestNeighbor(const vector<floattype> &  v, int constraint) : mDTW((uint)v.size(),constraint) {}
        virtual floattype test(const vector<floattype> & candidate) {return 0;}//= 0;
        virtual floattype getLowestCost() {return 0;}
        virtual ~NearestNeighbor() {};
        virtual int getNumberOfDTW(){return 0;}
        virtual int getNumberOfCandidates(){return 0;}
        dtw mDTW;
};

class NaiveNearestNeighbor :  public NearestNeighbor {
    public:
    NaiveNearestNeighbor(const vector<floattype> &  v, int constraint) : NearestNeighbor(v,constraint), lb_keogh(0),full_dtw(0),
     V(v),
    bestsofar(dtw::INF) {
    }
    
    floattype test(const vector<floattype> & candidate) {
        ++lb_keogh;++full_dtw;
        const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
        if(trueerror < bestsofar) bestsofar = trueerror;
        return bestsofar;
    }
    
    
    void resetStatistics() {
        lb_keogh = 0;
        full_dtw = 0;
    }
    floattype getLowestCost(){return bestsofar;}
    
    int getNumberOfDTW(){return full_dtw;}
    
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    
    private:
    int lb_keogh;
    int full_dtw;
    
    const vector<floattype>   V;
    floattype bestsofar;
    
};

/**
* Usage: create the object using the time series you want to match again the
* database and some DTW time constraint parameter. Then repeatedly apply
* the test function on the various candidates. The function returns the
* matching cost with the best candidate so far. By keeping track of when
* the cost was lowered, you can find the nearest neighbor in a database.
*
* Only the l1 norm is used.
*/
class LB_Keogh :  public NearestNeighbor{
    public:
    LB_Keogh(const vector<floattype> &  v, int constraint) : NearestNeighbor(v,constraint), lb_keogh(0), full_dtw(0), V(v), mConstraint(constraint),
    bestsofar(dtw::INF), U(V.size(),0), L(V.size(),0) {
        assert(mConstraint>=0);
        assert(mConstraint<static_cast<int>(V.size()));
        computeEnvelope(V,mConstraint,U,L);
    }
    
    floattype justlb(const vector<floattype> & candidate) {
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i])
              error += candidate[i] - U[i];
            else if(candidate[i] < L[i])
              error += L[i] - candidate[i];
        }
        return error;
    }
    floattype test(const vector<floattype> & candidate) {
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i])
              error += candidate[i] - U[i];
            else if(candidate[i] < L[i])
              error += L[i] - candidate[i];
        }
        //cout << "lb keogh = "<<error<<endl;
        if(error < bestsofar) {
            ++full_dtw;
            const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
            if(trueerror < bestsofar) bestsofar = trueerror;
        }
        return bestsofar;
    }
    
    
    int getNumberOfDTW(){return full_dtw;}
    
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    
    floattype getLowestCost(){return bestsofar;}
    
    void resetStatistics() {
        lb_keogh = 0;
        full_dtw = 0;
    }
    
    private:
    
    int lb_keogh;
    int full_dtw;
    const vector<floattype>   V;
    int mConstraint;
    floattype bestsofar;
    vector<floattype>   U;
    vector<floattype>   L;
    
};


class LB_KeoghEarly :  public NearestNeighbor{
    public:
    LB_KeoghEarly(const vector<floattype> &  v, int constraint) : NearestNeighbor(v,constraint), lb_keogh(0), full_dtw(0), V(v), mConstraint(constraint),
    bestsofar(dtw::INF), U(V.size(),0), L(V.size(),0) {
        assert(mConstraint>=0);
        assert(mConstraint<static_cast<int>(V.size()));
        computeEnvelope(V,mConstraint,U,L);
    }
    
    floattype test(const vector<floattype> & candidate) {
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i])
              error += candidate[i] - U[i];
            else if(candidate[i] < L[i])
              error += L[i] - candidate[i];
            if(error > bestsofar) return bestsofar;
        }
            ++full_dtw;
            const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
            if(trueerror < bestsofar) bestsofar = trueerror;
        return bestsofar;
    }
    
    
    int getNumberOfDTW(){return full_dtw;}
    
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    
    floattype getLowestCost(){return bestsofar;}
    
    void resetStatistics() {
        lb_keogh = 0;
        full_dtw = 0;
    }
    
    private:
    
    int lb_keogh;
    int full_dtw;
    const vector<floattype>   V;
    int mConstraint;
    floattype bestsofar;
    vector<floattype>   U;
    vector<floattype>   L;
    
};






/**
* Usage (same as LB_Keogh): create the object using the time series you want to match again the
* database and some DTW time constraint parameter. Then repeatedly apply
* the test function on the various candidates. The function returns the
* matching cost with the best candidate so far. By keeping track of when
* the cost was lowered, you can find the nearest neighbor in a database.
*
* Only the l1 norm is used.
*/
class LB_Improved : public  NearestNeighbor{
    public:
    LB_Improved(const vector<floattype> &  v, int constraint) : NearestNeighbor(v,constraint), lb_keogh(0), full_dtw(0), V(v), buffer(v), mConstraint(constraint),
    bestsofar(dtw::INF), U(V.size(),0), L(V.size(),0), U2(V.size(),0), L2(V.size(),0) {
        assert(mConstraint>=0);
        assert(mConstraint<static_cast<int>(V.size()));
        computeEnvelope(V,mConstraint,U,L);
    }
    
    void resetStatistics() {
        lb_keogh = 0;
        full_dtw = 0;
    }
    
    
    
    floattype justlb(const vector<floattype> & candidate) {
        floattype error (0.0);
        buffer = candidate;
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i])
              error += candidate[i] - U[i];
            else if(candidate[i] < L[i])
              error += L[i] - candidate[i];
        }
        computeEnvelope(buffer,mConstraint,U2,L2);
        for(uint i = 0; i < V.size();++i) {
                if(V[i] > U2[i]) {
                      error += V[i] - U2[i];
                } else if(V[i] < L2[i]) {
                      error += L2[i] - V[i];
                }
        }
        return error;
    }
    
    
    floattype test(const vector<floattype>  &candidate) {
        //memcpy(&buffer[0], &candidate[0],buffer.size()*sizeof(floattype));// = candidate;
        //buffer = candidate;
        //vector<floattype> & buffer(candidate);// no need for a copy
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            const floattype & cdi (candidate[i]);
            if(cdi > U[i]) {
              error += cdi - (buffer[i] = U[i]);
            } else if(cdi < L[i]) {
              error += (buffer[i] = L[i]) - cdi;
            } else buffer[i] = cdi;
        }
        if(error < bestsofar) {
            computeEnvelope(buffer,mConstraint,U2,L2);
            //env.compute(buffer,mConstraint,U2,L2);
            for(uint i = 0; i < V.size();++i) {
                if(V[i] > U2[i]) {
                      error += V[i] - U2[i];
                } else if(V[i] < L2[i]) {
                      error += L2[i] - V[i];
                }
            }
            if(error < bestsofar) {
                ++full_dtw;
                const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
                if(trueerror < bestsofar) bestsofar = trueerror;
            }
        }
        return bestsofar;
    }
    
    /**
    * for plotting purposes
    */
    string dumpTextDescriptor(const vector<floattype>  &candidate) {
        //memcpy(&buffer[0], &candidate[0],buffer.size()*sizeof(floattype));// = candidate;
        buffer = candidate;
        //vector<floattype> & buffer(candidate);// no need for a copy
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i]) {
              error += candidate[i] - U[i];
              buffer[i] = U[i];
            } else if(candidate[i] < L[i]) {
              error += L[i] - candidate[i];
              buffer[i] = L[i];
            }
            assert((buffer[i]== L[i]) or (buffer[i] == U[i]) or (buffer[i] == candidate[i]));
            
        }
        vector<floattype> lbimprovedarray;
        computeEnvelope(buffer,mConstraint,U2,L2);
        for(uint i = 0; i < V.size();++i) {
                if(V[i] > U2[i]) {
                      error += V[i] - U2[i];
                      lbimprovedarray.push_back(U2[i]);
                } else if(V[i] < L2[i]) {
                      error += L2[i] - V[i];
                      lbimprovedarray.push_back(L2[i]);
                } else
                    lbimprovedarray.push_back(V[i]);

        }
        stringstream ss;
        for(uint k = 0; k <V.size(); ++k) {
            assert((lbimprovedarray[k]== L2[k]) or (lbimprovedarray[k] == U2[k]) or (lbimprovedarray[k] == V[k]));
            assert((buffer[k]== L[k]) or (buffer[k] == U[k]) or (buffer[k] == candidate[k]));
            ss<<k<<" "<<V[k]<<" "<<candidate[k]<<" "<<buffer[k]<<" "<<lbimprovedarray[k]<<" "<<L[k]<<" "<<U[k]<<" "<<L2[k]<<" "<<U2[k]<<endl;
        }
        //string ans;
        //ss>>ans;
        ///cout<<ss.str()<<endl;
        return ss.str();
    }

    int getNumberOfDTW(){return full_dtw;}
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    floattype getLowestCost(){return bestsofar;}
    private:
    
    int lb_keogh;
    int full_dtw;
    const vector<floattype>   V;
    vector<floattype> buffer;
    int mConstraint;
    floattype bestsofar;
    vector<floattype>   U;
    vector<floattype>   L;
    vector<floattype>   U2;
    vector<floattype>   L2;

};

class LB_ImprovedEarly : public  NearestNeighbor{
    public:
    LB_ImprovedEarly(const vector<floattype> &  v, int constraint) : NearestNeighbor(v,constraint), lb_keogh(0), full_dtw(0), V(v), buffer(v), mConstraint(constraint),
    bestsofar(dtw::INF), U(V.size(),0), L(V.size(),0), U2(V.size(),0), L2(V.size(),0) {
        assert(mConstraint>=0);
        assert(mConstraint<static_cast<int>(V.size()));
        computeEnvelope(V,mConstraint,U,L);
    }
    
    void resetStatistics() {
        lb_keogh = 0;
        full_dtw = 0;
    }

    floattype test(const vector<floattype>  &candidate) {
        //memcpy(&buffer[0], &candidate[0],buffer.size()*sizeof(floattype));// = candidate;
        //buffer = candidate;
        //vector<floattype> & buffer(candidate);// no need for a copy
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            const floattype & cdi (candidate[i]);
            if(cdi > U[i]) {
              error += cdi - (buffer[i] = U[i]);
            } else if(cdi < L[i]) {
              error += (buffer[i] = L[i]) - cdi;
            } else buffer[i] = cdi;
            if(error>bestsofar) return bestsofar;
        }
        if(error < bestsofar) {
            computeEnvelope(buffer,mConstraint,U2,L2);
            //env.compute(buffer,mConstraint,U2,L2);
            for(uint i = 0; i < V.size();++i) {
                if(V[i] > U2[i]) {
                      error += V[i] - U2[i];
                } else if(V[i] < L2[i]) {
                      error += L2[i] - V[i];
                }
                if(error>bestsofar) return bestsofar;

            }
            if(error < bestsofar) {
                ++full_dtw;
                const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
                if(trueerror < bestsofar) bestsofar = trueerror;
            }
        }
        return bestsofar;
    }

    
    int getNumberOfDTW(){return full_dtw;}
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    floattype getLowestCost(){return bestsofar;}
    private:
    
    int lb_keogh;
    int full_dtw;
    const vector<floattype>   V;
    vector<floattype> buffer;
    int mConstraint;
    floattype bestsofar;
    vector<floattype>   U;
    vector<floattype>   L;
    vector<floattype>   U2;
    vector<floattype>   L2;

};



void piecewiseSumReduction(const vector<floattype> & array, vector<floattype> & out) {
    // the length of out gives out the desired output length
    assert(out.size()>0);
    const uint sizeofpieces = (uint)array.size()/out.size();
    assert(sizeofpieces>0);
    //sum_up<floattype> s;
    for(uint k = 0; k<out.size()-1;++k) {
        //s.reset();
        out[k] = 0;
        for(uint j = k*sizeofpieces; j < (k+1)*sizeofpieces; ++j)
          out[k] += array[j];
    }
    uint k=(uint)out.size()-1;
    out[k] = 0;
    for(uint j = k*sizeofpieces; j < array.size(); ++j)
          out[k] += array[j];
}




/**
* for debugging purposes
*/
class DimReducedLB_Keogh :  public NearestNeighbor{
    public:
    DimReducedLB_Keogh(const vector<floattype> &  v, int constraint, int reduced) : NearestNeighbor(v,constraint), lb_keogh(0), full_dtw(0), V(v), mConstraint(constraint),
    bestsofar(dtw::INF), U(V.size(),0), L(V.size(),0),Ured(reduced,0),Lred(reduced) {
        assert(mConstraint>=0);
        assert(mConstraint<static_cast<int>(V.size()));
        computeEnvelope(V,mConstraint,U,L);
        piecewiseSumReduction(U, Ured);
        piecewiseSumReduction(L, Lred);
    }
    
    floattype test(const vector<floattype> & candidate) {
        vector<floattype> reducedCandidate(Ured.size());
        piecewiseSumReduction(candidate, reducedCandidate);
        floattype smallerror (0.0);
        for(uint i = 0; i < Ured.size();++i) {
            if(reducedCandidate[i] > Ured[i])
              smallerror += reducedCandidate[i] - Ured[i];
            else if(reducedCandidate[i] < Lred[i])
              smallerror += Lred[i] - reducedCandidate[i];
        }
        if(smallerror > bestsofar) return bestsofar;
        ++lb_keogh;
        floattype error (0.0);
        for(uint i = 0; i < V.size();++i) {
            if(candidate[i] > U[i])
              error += candidate[i] - U[i];
            else if(candidate[i] < L[i])
              error += L[i] - candidate[i];
        }
        if(error < bestsofar) {
            ++full_dtw;
            const floattype trueerror = mDTW.fastdynamic(V,candidate);//,mConstraint,1);
            if(trueerror < bestsofar) bestsofar = trueerror;
        }
        return bestsofar;
    }
    
    
    int getNumberOfDTW(){return full_dtw;}
    
    
    int getNumberOfCandidates(){return lb_keogh;}
    
    
    floattype getLowestCost(){return bestsofar;}
    
    
    private:
    
    int lb_keogh;
    int full_dtw;
    const vector<floattype>   V;
    int mConstraint;
    floattype bestsofar;
    vector<floattype>   U,L;
    vector<floattype>   Ured,Lred;
    
};





/**
* this class is not used?
*/
class Envelope {
    public:
    Envelope() : maxfifo(), minfifo(){}
    virtual ~Envelope() {}
    void compute(const vector<floattype> & array, uint constraint, vector<floattype> & maxvalues, vector<floattype> & minvalues) {
        const uint width = 1 +  2 * constraint;
        maxfifo.clear();minfifo.clear();
        for(uint i = 1; i < array.size(); ++i) {
          if(i >=constraint+1) {
            maxvalues[i-constraint-1] = array[maxfifo.size()>0 ? maxfifo.front():i-1];
            minvalues[i-constraint-1] = array[minfifo.size()>0 ? minfifo.front(): i-1];
          }
          if(array[i] > array[i-1]) { //overshoot
            minfifo.push_back(i-1);
            if(i ==  width+minfifo.front()) minfifo.pop_front();
            while(maxfifo.size() > 0) {
              if(array[i] <= array[maxfifo.back()]) {
                if (i==  width+maxfifo.front()) maxfifo.pop_front();
                break;
              }
              maxfifo.pop_back();
            }
          } else {
            maxfifo.push_back(i-1);
            if (i==  width+maxfifo.front()) maxfifo.pop_front();
            while(minfifo.size() > 0) {
              if(array[i] >= array[minfifo.back()]) {
                if(i==  width+minfifo.front()) minfifo.pop_front();
                break;
              }
              minfifo.pop_back();
            }
          }
        }
        for(uint i = (uint)array.size(); i <= array.size() + constraint; ++i) {
              if(maxfifo.size()>0) {
                  maxvalues[i-constraint-1] = array[maxfifo.front()];
                if(i-maxfifo.front() >= width) {
                    maxfifo.pop_front();}
              } else {
                  maxvalues[i-constraint-1] = array[array.size()-1];
              }
              if(minfifo.size() > 0) {
                  minvalues[i-constraint-1] = array[minfifo.front()];
                  if(i-minfifo.front() >= width) {
                      minfifo.pop_front();}
              } else {
                  minvalues[i-constraint-1] = array[array.size()-1];
              }
        }
 }
        deque<int> maxfifo, minfifo;


};

// only used for unit testing
floattype l1diff(const vector<floattype> &  v, const vector<floattype> & w) {
    floattype ans(0);
    for(uint k = 0; k<v.size();++k)
      ans+=abs(v[k]-w[k]);
    return ans;
}
#endif
