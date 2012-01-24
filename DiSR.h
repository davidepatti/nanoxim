/*****************************************************************************

  DiSR.h -- DiSR Model 

 *****************************************************************************/
#ifndef __DISR_H__
#define __DISR_H__

//---------------------------------------------------------------------------

#include <systemc.h>
#include "TRouter.h"
#include "nanoxim.h"

enum DiSR_status { BOOTSTRAP,READY_SEARCHING, ACTIVE_SEARCHING, CANDIDATE, ASSIGNED, FREE };
//
//---------------------------------------------------------------------------
// Distribuited SR data

class TSegmentId
{
    public:
	int node;
	int link;

	TSegmentId(int,int);
	TSegmentId();


	inline bool operator == (const TSegmentId& segid) const
	{
	    return ( (segid.node==node) && (segid.link==link));
	}

	inline bool isNull()
	{
	    //assert(!((node == NOT_VALID) ^^ (link == NOT_VALID)));
	    return ( (node == NOT_VALID) || (link == NOT_VALID));

	}

};


class DiSR
{

  // Distribuited SR related functions and data
    public:
  void reset();
  void update_status();
  int process(const TPacket& p);
  void set_router(TRouter *);


    private:
  int next_free_link();
  bool sanity_check();
  void search_starting_segment();

  // Local environment data (LED)

  TSegmentId segment;
  TSegmentId visited;
  TSegmentId tvisited;
    // intented as bidirectional!
  TSegmentId link_visited[4];
  TSegmentId link_tvisited[4];

  bool starting;
  bool terminal;
  int subnet;
  int current_link;
  TRouter * router;

  DiSR_status status;
  
};

//---------------------------------------------------------------------------

#endif
