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

TSegmentId::TSegmentId()
{
    this->node = NOT_VALID;
    this->link = NOT_VALID;
}

void DiSR::set_router(TRouter * r)
{
    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    if (r->local_id == 0) 
	status = BOOTSTRAP;
    else
	status = FREE;
    this->router = r;
}

int DiSR::process(const TPacket& p)
{
    if ( (p.type == STARTING_SEGMENT_REQUEST) && (p.src_id!=router->local_id) )
    {
	cout << "[DiSR on node "<<router->local_id<<"]: received STARTING SEG REQUEST with ID " << p.src_id << endl;
	return DIRECTION_ALL;
    }

    return DIRECTION_SOUTH;
}

int DiSR::next_free_link()
{
    while (current_link<DIRECTION_LOCAL)
    {

	if ( (link_visited[current_link].isNull()) && (link_tvisited[current_link].isNull()))
	{
	    cout << "[DiSR on node "<<router->local_id<<"]: found free link " << current_link+1 <<  endl;
	    return current_link++;
	}
	current_link++;
    }

    return NOT_VALID;
}

void DiSR::search_starting_segment()
{
	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    status = ACTIVE_SEARCHING;
	    cout << "[DiSR on "<<router->local_id<<"]:Injecting STARTING_SEGMENT_REQUEST on link " << candidate_link << endl;
	    // prepare the packet
	    TPacket packet;
	    packet.src_id = router->local_id;
	    packet.type = STARTING_SEGMENT_REQUEST;
	    packet.dir_in = DIRECTION_LOCAL;
	    packet.dir_out = candidate_link;
	    packet.hop_no = 0;
	    router->inject_to_network(packet);
	}
	else
	{
	    cout << "[DiSR on "<<router->local_id<<"]:cant Inject STARTING_SEGMENT_REQUEST (no suitable links)" << endl;
	}

}

void DiSR::update_status()
{

    if (router->local_id==0)
    {
	if (status == BOOTSTRAP)

	{ 
	    cout << "[DiSR on "<<router->local_id<<"]: BOOSTRAP" << endl;
	    cout << "[DiSR on "<<router->local_id<<"]: STARTING node setup, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	    //must search for starting segment
	    search_starting_segment();
	}
    }
}

void DiSR::reset()
{
    const TSegmentId segnull = TSegmentId(NOT_VALID,NOT_VALID);
    visited= segnull;
    tvisited= segnull;
    segment = segnull;

    for (int i =0;i<DIRECTIONS;i++)
    {
	link_visited[i] = segnull;
	link_tvisited[i] = segnull;
    }

    starting = false;
    terminal = false;
    subnet = NOT_VALID;
    current_link = DIRECTION_NORTH;
    status = FREE;
}

bool DiSR::sanity_check()
{
    return true;
}
