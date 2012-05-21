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
	setStatus(BOOTSTRAP);
    else
	setStatus(FREE);
}


DiSR_status DiSR::getStatus() const
{
    // sanity check of some assumed environmnet values
    switch (this->status) {
	case BOOTSTRAP:
	    assert(tvisited==false);
	    assert(visited==false);
	    break;
	case READY_SEARCHING:
	    assert(visited==true);
	    assert(tvisited==false);
	    break;
	case ACTIVE_SEARCHING:
	    assert(visited==true);
	    assert(tvisited==false);
	    break;
	case CANDIDATE:
	    assert(tvisited==true);
	    assert(visited==false);
	    break;
	case ASSIGNED:
	    assert(visited==true);
	    assert(tvisited==false);
	    break;
	case FREE:
	    assert(tvisited==false);
	    assert(visited==false);
	    break;
	default:
	    cout << "[DiSR on "<<router->local_id<<"] status:  NOTVALID!!" << endl;
	    assert(false);
	    exit(-1);
    }

    return (this->status);

}

void DiSR::setStatus(const DiSR_status& new_status)
{
    DiSR_status current_status = getStatus();

    assert(new_status!=current_status);

    switch (current_status) {
	case BOOTSTRAP:
	    assert(new_status==ACTIVE_SEARCHING);
	    break;
	case READY_SEARCHING:
	    break;
	case ACTIVE_SEARCHING:
	    break;
	case CANDIDATE:
	    break;
	case ASSIGNED:
	    break;
	case FREE:
	    break;
	default:
	    cout << "[DiSR on "<<router->local_id<<"] status:  NOTVALID!!" << endl;
	    assert(false);
	    exit(-1);
    }
    this->status = new_status;
}


void DiSR::setLinks(int type, const vector<int>& directions,const TSegmentId& id)
{

    for (int i = 0;i<directions.size();i++)
    {
	if (type==TVISITED)
	    link_tvisited[directions[i]] = id;

	if (type==VISITED)
	    link_visited[directions[i]] = id;
    }
}


    ////////////////////////////////////////////////////////////////
    // This function is called whenever a packet is received
    // In most of the case the result is:
    // - to update the current dynamic behaviour status (DBS)
    // - to change some local environmnet data (LED) variables
    //
    // The retun value can be:
    // - a specific direction
    // - an action
    // and it is used by the router class to determine how
    // the current packet should be handled
    //
    // IMPORTANT: The semantic is that DBS changes must happen only after all LED
    // varibables have been properly updated
int DiSR::process(TPacket& p)
{
    const TSegmentId freeid = TSegmentId(NOT_RESERVED,NOT_RESERVED);

    ////////////////////////////////////////////////////////
    if (p.type == STARTING_SEGMENT_REQUEST )
    ////////////////////////////////////////////////////////
    {
	TSegmentId pippo = p.id;
	cout << "[DiSR on "<<router->local_id << "]: processing STARTING SEG REQUEST with ID " << pippo << endl;

	 // locally generated 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	     // just inject to the link found by next_free_link() during bootstrap
	    return p.dir_out;
	}
	// foreign starting segment request
	else if (  p.src_id!=router->local_id ) 
	{
	    if (this->getStatus()==FREE)
	    {
		cout << "[DiSR on "<<router->local_id << "]: enable flooding..." << endl;

		this->segID = p.id;
		tvisited = true;
		link_tvisited[p.dir_in] = p.id;


		setStatus(CANDIDATE);
		// TODO: after flooding links has been selecte by the
		// router, the LED variable should be update accordingly
		return FLOOD_MODE;
	    }
		// ignore flooding if packet already received....
	    else
	    {
		// TODO: determine more accurately what happened....
		cout << "[DiSR on "<< router->local_id <<  "]: IGNORE flooding, already done" << endl;
		return IGNORE;
	    }
	    
	}
	// the packet returned to its orginal source, must confirm segment!
	// Note: this is a starting segment, the node is already
	// visited, there's no need to move from tvisited to visited
	else if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    this->segID = p.id;
	    cout << "[DiSR on node "<<router->local_id<<"]: CONFIRM STARTING SEG REQUEST " << this->segID << endl;
	    setStatus(ASSIGNED);
	    confirm_starting_segment(p);
	    return CONFIRM; // TODO: perform confirmation stuff 
	}
    }

    /////////////////////////////////////////////////////////
    if (p.type == SEGMENT_CONFIRM)
    ////////////////////////////////////////////////////////
    {
	TSegmentId segment_id = p.id;
	cout << "[DiSR on "<<router->local_id << "]: processing SEGMENT_CONFIRM with ID " << segment_id << endl;

	 // locally generated starting segment packet confirmation, 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	     // inject the packet to the appropriate link found by
	     // confirm_starting_segment()
	    return p.dir_out;
	}
	// foreign confirm segment request
	else
	if (  p.src_id!=router->local_id ) 
	{

	    if  ( (this->getStatus()==CANDIDATE) && (this->segID == p.id) ) 
	    {
		cout << "[DiSR on "<< router->local_id <<  "]: CANDIDATE to segment id: " << segment_id << " has been ASSIGNED!" << endl;
		this->tvisited = false;
		this->visited = true;
		this->link_visited[p.dir_in] = p.id;
		this->link_tvisited[p.dir_in] = freeid;

		this->setStatus(ASSIGNED);
		cout << "[DiSR on "<< router->local_id <<  "]: FORWARDING CONFIRM " << segment_id << " back to " << flooding_path << endl;
		return flooding_path;
		
		// return FORWARD_CONFIRM;
	    }
	    else 
	    {
		assert(false);
		cout << "[DiSR on "<<router->local_id << "]: CRITICAL, receving SEGMENT_CONFIRM with inconsistent environment" << endl;
		return IGNORE;
	    }
	    
	    
	}
	else // the confirmation packet returned to its orginal source, alla done!
	if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    cout << "[DiSR on node "<<router->local_id<<"]: confirmation process of segment id " << this->segID <<  " ENDED !" << endl;
	    setStatus(ASSIGNED);
	    return END_CONFIRM; 
	}
    }

    return IGNORE;
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
    // TODO: put here some timer-like stuff ?
    if (this->status==BOOTSTRAP)
	bootstrap_node();
}

void DiSR::bootstrap_node()
{

	// go directly to ACTIVE status 
	cout << "[DiSR on "<<router->local_id<<"] starting node BOOTSTRAP, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	
	/////////////////////////////////////////////////////////////////////////////////
	//must search for starting segment

	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    TSegmentId segment_id(router->local_id,candidate_link);
	    // mark the link and the node with id of segment request
	    link_tvisited[candidate_link] = segment_id;
	    visited = true;
	    this->setStatus(ACTIVE_SEARCHING);


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

	}
	else
	{
	    cout << "[DiSR on "<<router->local_id<<"]: CRITICAL! cant inject STARTING_SEGMENT_REQUEST (no suitable links)" << endl;
	}
	///////////////////// end injecting starting segment //////

}

// prepares the packet required to confirm the the request and inject
// it in the network.
// As a consequence, the rx process of the router will find the packet
// on the buffer in the local direction
void DiSR::confirm_starting_segment(TPacket& p)
{
    TSegmentId segment_id = p.id;
    cout << "[DiSR on "<<router->local_id<<"] confirming STARTING_SEGMENT_REQUEST " << segment_id << " from direction " << p.dir_in << endl;

    // change only the data required when sending back the packet

    p.type = SEGMENT_CONFIRM;
    p.dir_out = p.dir_in;  // must return from where it came!
    p.dir_in = DIRECTION_LOCAL;

    cout << "[DiSR on "<<router->local_id<<"] injecting SEGMENT_CONFIRM " << segment_id << " towards direction " << p.dir_out << endl;
    router->inject_to_network(p);

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
