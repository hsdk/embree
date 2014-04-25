// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "geometry/bezier1.h"
#include "builders/primrefalloc.h"
#include "heuristic_fallback.h"

namespace embree
{
  /*! Performs standard object binning */
  struct ObjectPartition
  {
    /*! number of bins */
    static const size_t BINS = 16;

    typedef PrimRefBlockT<Bezier1> BezierRefBlock;
    typedef atomic_set<BezierRefBlock> BezierRefList;

    /*! Compute the number of blocks occupied for each dimension. */
    //__forceinline static ssei blocks(const ssei& a) { return (a+ssei(3)) >> 2; }
    __forceinline static ssei blocks(const ssei& a) { return a; }
	
    /*! Compute the number of blocks occupied in one dimension. */
    //__forceinline static size_t  blocks(size_t a) { return (a+3) >> 2; }
    __forceinline static size_t  blocks(size_t a) { return a; }

    /*! mapping into bins */
    struct Mapping
    {
    public:
      __forceinline Mapping() {}
      __forceinline Mapping(const BBox3fa& centBounds, const LinearSpace3fa& space);
      __forceinline ssei bin(const Vec3fa& p) const;
      __forceinline ssei bin_unsafe(const Vec3fa& p) const;
      __forceinline bool invalid(const int dim) const;
    public:
      ssef ofs,scale;
      LinearSpace3fa space;
    };

     struct Split
    {
      __forceinline Split()
	 : dim(-1), pos(0), cost(inf) {}

       /*! calculates standard surface area heuristic for the split */
    __forceinline float splitSAH(float intCost) const {
      return intCost*cost;
    }

      /*! splits hairs into two sets */
    void split(size_t threadIndex, PrimRefBlockAlloc<Bezier1>& alloc, BezierRefList& curves, BezierRefList& lprims_o, PrimInfo& linfo_o, BezierRefList& rprims_o, PrimInfo& rinfo_o) const;

      //LinearSpace3fa space;
      int dim;
      int pos;
      float cost;
      //ssef ofs,scale;
      Mapping mapping;
    };

    struct BinInfo
    {
      BinInfo();
      void  bin (BezierRefList& prims, const Mapping& mapping);
      Split best(BezierRefList& prims, const Mapping& mapping);

      BBox3fa bounds[BINS][4];
      ssei    counts[BINS];
    };

  public:
    
   
    
    /*! performs object binning to the the best partitioning */
    static ObjectPartition::Split find(size_t threadIndex, BezierRefList& curves, const LinearSpace3fa& space);
    
    
  public:
   
  };
}