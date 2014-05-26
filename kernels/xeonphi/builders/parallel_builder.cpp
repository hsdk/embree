#include "parallel_builder.h"

namespace embree
{

  static double dt = 0.0f;

  void ParallelBuilderInterface::fillLocalWorkQueues(const size_t threadID, const size_t numThreads)
  {
    __aligned(64) BuildRecord br;
    const size_t numCores = (numThreads+3)/4;
    const size_t coreID   = threadID/4;

    if (threadID % 4 == 0)
      {
	unsigned int ID = coreID; // LockStepTaskScheduler::taskCounter.inc();
	
	while (true) 
	  {
	    /* get build record from global queue */
	    if (ID >= global_workStack.size()) break;
	    br = global_workStack.get(ID);
	    bool success = local_workStack[coreID].push(br);	  
	    if (!success) FATAL("can't fill local work queues");
	    ID += numCores;
	  }    

      }
  }

  void ParallelBuilderInterface::buildSubTrees(const size_t threadID, const size_t numThreads)
  {
    NodeAllocator alloc(atomicID,numAllocatedNodes);
    __aligned(64) BuildRecord br;
    const size_t numCores = (numThreads+3)/4;
    const size_t globalCoreID   = threadID/4;

    if (enablePerCoreWorkQueueFill && numThreads > 1)
      {
	const size_t globalThreadID = threadID;
	const size_t localThreadID  = threadID % 4;
    
	if (localThreadID != 0)
	  {
	    localTaskScheduler[globalCoreID].dispatchTaskMainLoop(localThreadID,globalThreadID);
	  }
	else
	  {
	    local_workStack[globalCoreID].mutex.inc();
	    while (local_workStack[globalCoreID].size() < 8 && 
		   local_workStack[globalCoreID].size()+ 4 <= SIZE_LOCAL_WORK_STACK) 
	      {
		BuildRecord br;
		if (!local_workStack[globalCoreID].pop_largest(br)) break;
		buildSubTree(br,alloc,FILL_LOCAL_QUEUES,globalThreadID,4);
	      }

	    localTaskScheduler[globalCoreID].releaseThreads(localThreadID,globalThreadID);	
	    local_workStack[globalCoreID].mutex.dec();
	  }
      }

    while(true)
      {
      /* process local work queue */
	while (1)
	  {
	    if (!local_workStack[globalCoreID].pop_largest(br)) 
	      {
		if (local_workStack[globalCoreID].mutex.val() > 0)
		  {
		    __pause(1024);
		    continue;
		  }
		else
		  break;
	      }
	    local_workStack[globalCoreID].mutex.inc();
	    buildSubTree(br,alloc,RECURSE,threadID,numThreads);
	    local_workStack[globalCoreID].mutex.dec();
	  }

	/* try task stealing */
        bool success = false;
	if (enableTaskStealing && numThreads > 4)
	  {
	    for (size_t i=0; i<numThreads; i++)
	      {
		unsigned int next_threadID = (threadID+i);
		if (next_threadID >= numThreads) next_threadID -= numThreads;
		const unsigned int next_globalCoreID   = next_threadID/4;

		assert(next_globalCoreID < numCores);
		if (local_workStack[next_globalCoreID].pop_smallest(br)) { 
		  success = true;
		  break;
		}
	      }
	  }
        if (!success) break; 
	
	local_workStack[globalCoreID].mutex.inc();
	buildSubTree(br,alloc,RECURSE,threadID,numThreads);
	local_workStack[globalCoreID].mutex.dec();

      }

  }
};