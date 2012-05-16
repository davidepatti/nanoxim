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
    this->node = NOT_RESERVED;
    this->link = NOT_RESERVED;
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
    // TODO: check all cases

    if (p.type == STARTING_SEGMENT_REQUEST )
    {
	TSegmentId pippo = p.id;
	cout << "[DiSR on "<<router->local_id << "]: processing STARTING SEG REQUEST with ID " << pippo << endl;

	// foreign starting segment request, flooding ...
	if ( p.src_id!=router->local_id )
	{
	    cout << "[DiSR on "<<router->local_id << "]: enable flooding..." << endl;
	    // TODO: ignore flooding if packet already received....
	    this->status = CANDIDATE;
	    return FLOOD_MODE;
	    
	}
	else // processing a locally generated starting segment packet, 
	     // inject to the link found by next_free_link()
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	    return p.dir_out;
	}
	else // the packet returned to its orginal source, confirm segment!
	if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    this->status =  ASSIGNED;
	    //cout << "[DiSR on node "<<router->local_id<<"]: confirming  STARTING SEG REQUEST " << p.id << endl;
	    return DIRECTION_LOCAL; // TODO: check how packets are sinked
	}
    }

    return DIRECTION_SOUTH;
}

int DiSR::next_free_link()
{
    cout << "[DiSR on node "<<router->local_id<<"]: CALL next_free_link() " << endl;
    while (current_link<DIRECTION_LOCAL)
    {

	if ( (link_visited[current_link].isFree()) && (link_tvisited[current_link].isFree()))
	{
	    cout << "[DiSR on node "<<router->local_id<<"]: found free link " << current_link <<  endl;
	    return current_link++;
	}
	current_link++;
    }
    cout << "[DiSR on node "<<router->local_id<<"]: MSG no link found " << endl;

    return NOT_VALID;
}


void DiSR::print_status() const
{
    if (this->router!=NULL)
    {
	switch (this->status) {
	    case BOOTSTRAP:
		cout << "[DiSR on "<<router->local_id<<"] status:  BOOTSTRAP" << endl;
		break;
	    case READY_SEARCHING:
		cout << "[DiSR on "<<router->local_id<<"] status:  READY_SEARCHING" << endl;
		break;
	    case ACTIVE_SEARCHING:
		cout << "[DiSR on "<<router->local_id<<"] status:  ACTIVE_SEARCHING" << endl;
		break;
	    case CANDIDATE:
		cout << "[DiSR on "<<router->local_id<<"] status:  CANDIDATE" << endl;
		break;
	    case ASSIGNED:
		cout << "[DiSR on "<<router->local_id<<"] status:  ASSIGNED" << endl;
		break;
	    case FREE:
		// assuming mute as free
		// cout << "[DiSR on "<<router->local_id<<"] status:  FREE" << endl;
		break;
	    default:
		cout << "[DiSR on "<<router->local_id<<"] status:  NOTVALID!!" << endl;
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

    if (status == BOOTSTRAP)
    { 
	// go directly to ACTIVE status 
	cout << "[DiSR on "<<router->local_id<<"] starting node BOOTSTRAP, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	
	/////////////////////////////////////////////////////////////////////////////////
	//must search for starting segment

	visited = true;
	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    cout << "[DiSR on "<<router->local_id<<"] free link " << candidate_link << endl;
	    TSegmentId segment_id(router->local_id,candidate_link);
	    // mark the link and the node with id of segment request
	    link_tvisited[candidate_link] = segment_id;


	    // prepare the packet
	    TPacket packet;
	    packet.id = segment_id;
	    packet.src_id = router->local_id;
	    packet.type = STARTING_SEGMENT_REQUEST;
	    packet.dir_in = DIRECTION_LOCAL;
	    packet.dir_out = candidate_link;
	    packet.hop_no = 0;

	    cout << "[DiSR on "<<router->local_id<<"] injecting STARTING_SEGMENT_REQUEST " << segment_id << " towards direction " << candidate_link << endl;
	    router->inject_to_network(packet);
	    status = ACTIVE_SEARCHING;
	}
	else
	{
	    cout << "[DiSR on "<<router->local_id<<"]: CRITICAL! cant inject STARTING_SEGMENT_REQUEST (no suitable links)" << endl;
	}
	///////////////////// end injecting starting segment //////
    }

    if (status == ACTIVE_SEARCHING)
    {

    }
    print_status();
}

void DiSR::reset()
{
    const TSegmentId freeid = TSegmentId(NOT_RESERVED,NOT_RESERVED);
    visited= false;
    tvisited= false;
    segID = freeid;
    starting = false;
    terminal = false;
    subnet = NOT_RESERVED;
    current_link = DIRECTION_NORTH;

    for (int i =0;i<DIRECTIONS;i++)
    {
	// note: not valid links shouldnt' be set as free
	if (link_visited[i].isValid())
	{
	    link_visited[i] = freeid;
	    link_tvisited[i] = freeid;
	}
    }

    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    // Whener the router pointer is updated, status must be resetted
    if ((router!=NULL) && (router->local_id == 0)) 
	status = BOOTSTRAP;
    else
	status = FREE;
}

void DiSR::invalidate(int d)
{
    link_visited[d] = TSegmentId(NOT_VALID,NOT_VALID);
    link_tvisited[d] = TSegmentId(NOT_VALID,NOT_VALID);

}

bool DiSR::sanity_check()
{
    return true;
}
