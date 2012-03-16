/*****************************************************************************

  DiSR.cpp -- DiSR algorithm model implementation

*****************************************************************************/
#include "nanoxim.h"
#include "TRouter.h"

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
    this->router = r;
    cout << "[DiSR on node "<<router->local_id<<"]: setting router id to " << r->local_id << endl;
    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    // Whener the router pointer is updated, status must be resetted
    if (r->local_id == 0) 
	status = BOOTSTRAP;
    else
	status = FREE;
}

int DiSR::process(const TPacket& p)
{
    cout << "[DiSR on node "<<router->local_id<<"]: CALL process" << endl;
    if ( (p.type == STARTING_SEGMENT_REQUEST) && (p.src_id!=router->local_id) )
    {
	cout << "[DiSR on node "<<router->local_id<<"]: received STARTING SEG REQUEST with ID " << p.src_id << endl;
	return DIRECTION_ALL;
    }

    return DIRECTION_SOUTH;
}

int DiSR::next_free_link()
{
    cout << "[DiSR on node "<<router->local_id<<"]: CALL next_free_link() " << endl;
    while (current_link<DIRECTION_LOCAL)
    {

	if ( (link_visited[current_link].isNull()) && (link_tvisited[current_link].isNull()))
	{
	    cout << "[DiSR on node "<<router->local_id<<"]: found free link " << current_link+1 <<  endl;
	    return current_link++;
	}
	current_link++;
    }
    cout << "[DiSR on node "<<router->local_id<<"]: MSG no link found " << endl;

    return NOT_VALID;
}

void DiSR::search_starting_segment()
{
	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    status = ACTIVE_SEARCHING;
	    cout << "[DiSR on "<<router->local_id<<"] injecting STARTING_SEGMENT_REQUEST on link " << candidate_link << endl;
	    TSegmentId segment_id(router->local_id,candidate_link);

	    // mark the link and the node with id of segment request
	    link_tvisited[candidate_link] = segment_id;
	    visited = segment_id;

	    // prepare the packet
	    TPacket packet;
	    packet.id = segment_id;
	    packet.src_id = router->local_id;
	    packet.type = STARTING_SEGMENT_REQUEST;
	    packet.dir_in = DIRECTION_LOCAL;
	    packet.dir_out = candidate_link;
	    packet.hop_no = 0;

	    router->inject_to_network(packet);
	}
	else
	{
	    cout << "[DiSR on "<<router->local_id<<"]:cant inject STARTING_SEGMENT_REQUEST (no suitable links)" << endl;
	}

}

void DiSR::print_status() const
{
    if (this->router!=NULL)
    {
	switch (this->status) {
	    case BOOTSTRAP:
		cout << "[DiSR on "<<router->local_id<<"] current status:  BOOTSTRAP" << endl;
		break;
	    case READY_SEARCHING:
		cout << "[DiSR on "<<router->local_id<<"] current status:  READY_SEARCHING" << endl;
		break;
	    case ACTIVE_SEARCHING:
		cout << "[DiSR on "<<router->local_id<<"] current status:  ACTIVE_SEARCHING" << endl;
		break;
	    case CANDIDATE:
		cout << "[DiSR on "<<router->local_id<<"] current status:  CANDIDATE" << endl;
		break;
	    case ASSIGNED:
		cout << "[DiSR on "<<router->local_id<<"] current status:  ASSIGNED" << endl;
		break;
	    case FREE:
		cout << "[DiSR on "<<router->local_id<<"] current status:  FREE" << endl;
		break;
	    default:
		cout << "[DiSR on "<<router->local_id<<"] current status:  NOTVALID!!" << endl;
		exit(0);
	}
    }
    else
    {
		cout << "[DiSR on "<<router->local_id<<"] print_status:  ERROR, router not set" << endl;
    }

	    

}

void DiSR::update_status()
{
    cout << "[DiSR on "<<router->local_id<<"] updating DiSR status..." << endl;
    print_status();

    if (status == BOOTSTRAP)
    { 
	status = READY_SEARCHING;
	cout << "[DiSR on "<<router->local_id<<"] starting node setup, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	//must search for starting segment
	search_starting_segment();
    }

    if (status == ACTIVE_SEARCHING)
    {

    }
    print_status();
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
    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    // Whener the router pointer is updated, status must be resetted
    if ((router!=NULL) && (router->local_id == 0)) 
	status = BOOTSTRAP;
    else
	status = FREE;
}

bool DiSR::sanity_check()
{
    return true;
}
