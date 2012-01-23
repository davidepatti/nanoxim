/*****************************************************************************

  DiSR.cpp -- DiSR algorithm model implementation

*****************************************************************************/
#include "DiSR.h"

//---------------------------------------------------------------------------

TSegmentId::TSegmentId(int node,int link)
{
    this->node = node;
    this->link = link;
}

TSegment::TSegmentId()
{
    this->node = NOT_VALID;
    this->link = NOT_VALID;
}

int DiSR::process(const TPacket& p)
{
    if ( (p.type == FIRST_SEG_REQUEST) && (p.src_id!=local_id) )
    {
	cout << "["<<local_id<<"]: received FIRST SEG REQUEST with ID " << p.src_id << endl;
	return DIRECTION_ALL;
    }

    return DIRECTION_SOUTH;
}

int DiSR::next_free_link()
{
    while (this.current_link<DIRECTION_LOCAL)
    {

	if ( (!link_visited[current_link]) && (!link_tvisited[current_link]))
	    return current_link++;
	current_link++;
    }

    return NOT_VALID;
}

void DiSR::search_starting_segment()
{
	this.status = SEARCH_FIRST_SEG;
	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    cout << "["<<local_id<<"]:Injecting FIRST_SEG_REQUEST on link " << candidate_link << endl;
	    // prepare the packet
	    TPacket packet;
	    packet.src_id = local_id;
	    packet.type = FIRST_SEG_REQUEST;
	    packet.dir_in = DIRECTION_LOCAL;
	    packet.dir_out = candidate_link;
	    packet.hop_no = 0;
	    inject_to_network(packet);
	}
	else
	{
	    cout << "["<<local_id<<"]:cant Inject FIRST_SEG_REQUEST (no suitable links)" << endl;
	    status = END_SEARCH;
	}

}

void DiSR::update_status()
{


    if (local_id==0)
    {
	if (DiSR_data.status == INITIAL)
	{ 
	    cout << "["<<local_id<<"]: STARTING node setup, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	    //must search for first segment
	    DiSR_search_first_segment();
	}
    }
}

void DiSR::reset()
{
    const TSegmentId segnull = new TSegmentId(NOT_VALID,NOT_VALID);
    visited= segnull;
    tvisited= segnull;

    for (int i =0;i<DIRECTIONS;i++)
    {
	link_visited[i] = segnull;
	link_tvisited[i] = segnull;
    }

    starting = false;
    terminal = false;
    segment = segnull;
    subnet = NOT_VALID;
    current_link = DIRECTION_NORTH;
    status = FREE;
}

bool TRouter::DiSR_sanity_check()
{
    return true;
}
