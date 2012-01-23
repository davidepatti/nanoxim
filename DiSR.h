/*****************************************************************************

  DiSR.h -- DiSR Model 

 *****************************************************************************/
#ifndef __DISR_H__
#define __DISR_H__

//---------------------------------------------------------------------------

#include <systemc.h>
#include "TRouter.h"
#include "nanoxim.h"

enum DiSR_status { READY_SEARCHING, ACTIVE_SEARCHING, CANDIDATE, ASSIGNED, FREE };
//
//---------------------------------------------------------------------------
// Distribuited SR data

class TSegmentId
{
    public:
	int node;
	int link;

	TSegmentId(int node,int link);
	TSegmentId();


	inline bool operator == (const TSegmentId& segid) const
	{
	    return (segid.node==node && segid.link==link);
	}

	inline bool isNull()
	{
	    return ( (node == NOT_VALID) || (link == NOT_VALID));

	}

}


class DiSR
{

  // Distribuited SR related functions and data
    public:
  void reset();
  void update_status();
  int process(const TPacket& p);


    private:
  int next_free_link();
  bool sanity_check();
  void search_starting_segment();

  // Local environment data (LED)

  TSegmentId visited;
  TSegmentId tvisited;
    // intented as bidirectional!
  TSegmentId link_visited[4];
  TSegmentId link_tvisited[4];

  bool starting;
  bool terminal;
  int subnet;
  int current_link;

  DiSR_status status;
  
}

//---------------------------------------------------------------------------

#endif
