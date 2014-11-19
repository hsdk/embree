// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "range.h"

namespace embree
{
  template<typename ArrayArray, typename Func>
    __forceinline void sequential_for_for( ArrayArray& array2, const size_t minStepSize, const Func& f ) 
  {
    for (size_t i=0; i!=array2.size(); ++i) {
      const size_t N = array2[i].size();
      if (N) f(array2[i],range<size_t>(0,N));
    }
  }

  template<typename ArrayArray, typename Func>
    class ParallelForForTask
  {
  public:
    ParallelForForTask ( ArrayArray& array2, const size_t minStepSize, const Func& f)
      : array2(array2), minStepSize(minStepSize), f(f)
    {
      /* initialize arrays */
      const size_t M = array2.size();
      prefix_sum.resize(M);
      sizes.resize(M);

      /* compute prefix sum of number of elements of sub arrays */
      size_t sum=0;
      for (size_t i=0; i<M; i++) 
      {
        const size_t N = array2[i] ? array2[i]->size() : 0;
        prefix_sum[i] = sum;
        sizes[i] = N;
        sum += N;
      }
      K = sum;

      /* start parallel processing */
      LockStepTaskScheduler* scheduler = LockStepTaskScheduler::instance();
      const size_t threads = scheduler->getNumThreads();
      const size_t blocks  = (K+minStepSize-1)/minStepSize;
      scheduler->dispatchTaskSet(task_for_for,this,min(threads,blocks));
    }

    void for_for(const size_t threadIndex, const size_t threadCount, const size_t taskIndex, const size_t taskCount) 
    {
      /* calculate range */
      const size_t k0 = (taskIndex+0)*K/taskCount;
      const size_t k1 = (taskIndex+1)*K/taskCount;

      /* find start position */
      auto iter = std::upper_bound(prefix_sum.begin(), prefix_sum.end(), k0);
      size_t i0 = iter-prefix_sum.begin()-1;
      size_t j0 = k0-prefix_sum[i0];

      /* iterate over arrays */
      size_t k=k0;
      for (size_t i=i0; k<k1; i++) {
        const size_t N = sizes[i];
        const size_t r0 = j0;
        const size_t r1 = min(N,r0+k1-k);
        if (r1 > r0) f(array2[i],range<size_t>(r0,r1));
        k+=r1-r0; j0 = 0;
      }
    }
      
    static void task_for_for(void* data, const size_t threadIndex, const size_t threadCount, const size_t taskIndex, const size_t taskCount) {
      ((ParallelForForTask*)data)->for_for(threadIndex,threadCount,taskIndex,taskCount);
    }
    
    private:
      ArrayArray& array2;
      const size_t minStepSize;
      const Func& f;

    private:
      std::vector<size_t> prefix_sum;
      std::vector<size_t> sizes;
      size_t K;
  };

  template<typename ArrayArray, typename Func>
    __forceinline void parallel_for_for( ArrayArray& array2, const size_t minStepSize, const Func& f)
  {
    ParallelForForTask<ArrayArray,Func>(array2,minStepSize,f);
  }
}