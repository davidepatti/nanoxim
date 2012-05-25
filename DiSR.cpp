/*****************************************************************************

  DiSR.cpp -- DiSR algorithm model implementation

  Davide Patti, davide.patti@dieei.unict.it

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
    cout << "[node " << router->local_id << "] DiSR::set_router() setting router id to " << r->local_id << endl;
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
	case CANDIDATE_STARTING:
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
	    cout << "[node " <<router->local_id<< "] DiSR::getStatus()  WARNING not valid status " << this->status << endl;
    }

    return (this->status);

}

void DiSR::setStatus(const DiSR_status& new_status)
{
    DiSR_status current_status = this->status;

    //assert(new_status!=current_status);
    
    if (new_status==current_status)
	    cout << "[node "<<router->local_id<< "] DiSR::setStatus()  WARNING: setting already present status "<< current_status << endl;


    switch (new_status) {
	case BOOTSTRAP:
	    //assert(new_status==ACTIVE_SEARCHING);
	    break;
	case READY_SEARCHING:
	    break;
	case ACTIVE_SEARCHING:
	    break;
	case CANDIDATE_STARTING:
	    break;
	case CANDIDATE:
	    break;
	case ASSIGNED:
	    break;
	case FREE:
	    break;
	default:
	    cout << "[node " <<router->local_id<<"] DiSR::setStatus() CRITICAL: setting not valid status " << new_status << endl;
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
    const TSegmentId packet_segment_id = p.id;
    const TSegmentId local_segment_id = this->segID;

    ////////////////////////////////////////////////////////
    if (p.type == STARTING_SEGMENT_REQUEST )
    ////////////////////////////////////////////////////////
    {
	// if the node is already visited, the only possibility is
	// that it was the initiator of the request

	cout << "[node "<<router->local_id << "] DiSR::process() found STARTING SEG REQUEST with ID " << packet_segment_id << endl;

	 // locally generated // //////////////////////////////////////////////////
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	     // just inject to the link found by next_free_link() during bootstrap
	    return p.dir_out;
	}
	// the packet returned to its orginal source, must confirm segment!
	// Note: this is a starting segment, the node is already
	// visited, there's no need to move from tvisited to visited
	else if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    this->segID = packet_segment_id;
	    link_tvisited[p.dir_in] = freeid;
	    link_visited[p.dir_in] = packet_segment_id;

	    // since link LED has been updated:
	    current_link = 0;

	    cout << "[node "<<router->local_id<<"] DiSR::process() confirming STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;
	    setStatus(ASSIGNED);

	    generate_starting_segment_confirm(p);
	    return CONFIRM; 
	}
	// foreign starting segment request ////////////////////////////////////
	else if (  p.src_id!=router->local_id ) 
	{
	    if (this->getStatus()==FREE)
	    {
		cout << "[node "<<router->local_id << "] DiSR::process() enable FLOOD_MODE" << endl;

		this->segID = packet_segment_id;
		tvisited = true;
		link_tvisited[p.dir_in] = packet_segment_id;


		setStatus(CANDIDATE_STARTING);
		// TODO: after flooding links has been selecte by the
		// router, the LED variable should be update accordingly
		return FLOOD_MODE;
	    }
		// ignore flooding if packet already received....
	    else
	    {
		// TODO: determine more accurately what happened....
		cout << "[node "<< router->local_id <<  "] DiSR::process() ignore flooding, already done" << endl;
		return IGNORE;
	    }
	    
	}
    }
    /////////////////////////////////////////////////////////
    else if (p.type == SEGMENT_REQUEST)
    ////////////////////////////////////////////////////////
    {
	cout << "[node "<<router->local_id << "] DiSR::process() found SEGMENT_REQUEST with ID " << packet_segment_id << endl;

	 // locally generated 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	     // just inject to the link found by next_free_link() 
	    return p.dir_out;
	}
	// foreign segment request
	else if (  p.src_id!=router->local_id ) 
	{
	    if (this->getStatus()==FREE)
	    {
		/*
		cout << "[node "<<router->local_id << "] DiSR::process() enable FLOOD_MODE" << endl;

		this->segID = packet_segment_id;
		tvisited = true;
		link_tvisited[p.dir_in] = packet_segment_id;


		setStatus(CANDIDATE);
		// TODO: after flooding links has been selecte by the
		// router, the LED variable should be update accordingly
		// */
		return FORWARD_REQUEST;
	    }
	    // A normal SEGMENT_REQUEST packet reached a node
	    // candidate for starting segment. This means the starting
	    // segment has already been estabilished and the node must cancel a previous LED settings
	    else if (this->getStatus()==CANDIDATE_STARTING)
	    {
		this->segID = packet_segment_id;
		this->tvisited = true;
		this->visited = false;

		for (int i=0;i<DIRECTIONS;i++)
		{
		    link_visited[i] = freeid;
		    link_tvisited[i] = freeid;

		}
		// the incoming direction becomes tvisited 
		link_visited[p.dir_in] = freeid;
		link_tvisited[p.dir_in] = packet_segment_id;

		// since link status has been updated, the pointer of
		// the current link to be investigated must be resetted
		current_link = 0;

		cout << "[node "<< router->local_id <<  "] DiSR::process() cancelling previous CANDIDATE_STARTING and setting CANDIDATE with id " << this->segID << endl;
		this->setStatus(CANDIDATE);
	    }
	    else
	    {
		// TODO: determine more accurately what happened....
		cout << "[node "<< router->local_id <<  "] DiSR::process() node  cancelling request:" << endl;
		print_status();
		return CANCEL_REQUEST;
	    }
	    
	}
	// the packet returned to its orginal source
	else if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    /*
	    this->segID = packet_segment_id;
	    cout << "[node "<<router->local_id<<"] DiSR::process() confirming STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;
	    setStatus(ASSIGNED);
	    generate_confirm_starting_segment(p);
	    */
	    return IGNORE; 
	}
    }

    /////////////////////////////////////////////////////////
    else if (p.type == STARTING_SEGMENT_CONFIRM)
    ////////////////////////////////////////////////////////
    {

	cout << "[node "<<router->local_id << "] DiSR::process() found STARTING_SEGMENT_CONFIRM with ID " << packet_segment_id << endl;

	 // locally generated starting segment packet confirmation, 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id << "] DiSR::process() sending locally generated STARTING_SEGMENT_CONFIRM with id " << packet_segment_id << " towards " << p.dir_out << endl;
	     // inject the packet to the appropriate link previously found by generate_starting_segment_confirm()
	    return p.dir_out;
	}
	// foreign confirm segment request
	else
	if (  p.src_id!=router->local_id ) 
	{

	    if  ( (this->getStatus()==CANDIDATE_STARTING) && (local_segment_id == packet_segment_id) )
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process()  CANDIDATE_STARTING to segment id: " << local_segment_id << " has been ASSIGNED!" << endl;
		// node status changes from tvisited to visited
		this->tvisited = false;
		this->visited = true;

		// the incoming link and flooding path change from tvsited to visited with the segment id
		this->link_visited[p.dir_in] = packet_segment_id;
		this->link_tvisited[p.dir_in] = freeid;

		this->link_visited[flooding_path] = packet_segment_id;
		this->link_tvisited[flooding_path] = freeid;

		// ...while all other links can return free....
		for (int i=0;i<DIRECTIONS;i++) {
		    if ( (i!=flooding_path) && (i!=p.dir_in) )
		    {
			link_visited[i] = freeid;
			link_tvisited[i] = freeid;
		    }
		}

		// since link status has been updated, the pointer of
		// the current link to be investigated must be resetted
		current_link = 0;

		// all other directions tvisitred during flooding
		// should be released
		// TODO: deprecated ? this->release_flooding_paths();

		this->setStatus(ASSIGNED);
		cout << "[node "<< router->local_id <<  "] DiSR::process()  FORWARDING CONFIRM " << packet_segment_id << " back to " << flooding_path << endl;
		return flooding_path;
		
		// return FORWARD_CONFIRM;
	    }
	    else 
	    {
		cout << "[node "<<router->local_id << "] DiSR::process()  CRITICAL, receving STARTING_SEGMENT_CONFIRM with inconsistent environment:" << endl;
		cout << "[node "<<router->local_id << "] DiSR::process()  local segid = " << local_segment_id << " , packet id = " << packet_segment_id << ", ";
		print_status();
		assert(false);
		return IGNORE;
	    }
	    
	    
	}
	else // the confirmation packet returned to its orginal source, all done!
	if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id<<"] DiSR::process()  confirmation process of segment id " << packet_segment_id <<  " ENDED !" << endl;
	    setStatus(ASSIGNED);
	    return END_CONFIRM; 
	}
    }

    return IGNORE;
}


/* TODO: check if deprecated with the priority-based cancel
 *
// updates the LED data and send release packets along the previuously
// flooded directions
void DiSR::release_flooding_paths()
{
	cout << "[node "<<router->local_id<<"] DiSR::release_flooding_paths() ready to inject SEGMENT_CANCEL" << endl;
	
	const TSegmentId freeid = TSegmentId(NOT_RESERVED,NOT_RESERVED);
	/////////////////////////////////////////////////////////////////////////////////
	//must search for starting segment

	for (int i = 0;i<DIRECTIONS;i++)
	{
	    if (link_tvisited[i]==this->segID)
	    {

		link_tvisited[i] = freeid;

		// prepare the packet
		TPacket packet;
		packet.id = segment_id;
		packet.src_id = router->local_id;
		packet.type = SEGMENT_CANCEL;
		packet.dir_in = DIRECTION_LOCAL;
		packet.dir_out = i;
		packet.hop_no = 0;

		cout << "[node "<<router->local_id<<"] DiSR::release_flooding_paths() injecting SEGMENT_CANCEL " << segment_id << " towards direction " << i << endl;
		router->inject_to_network(packet);
	    }
	}
}
*/

int DiSR::next_free_link()
{
    cout << "[DiSR::next_free_link() on  "<<router->local_id<<"]" << endl;
    while (current_link<DIRECTION_LOCAL)
    {

	if ( (link_visited[current_link].isFree()) && (link_tvisited[current_link].isFree()))
	{
	    cout << "[node "<<router->local_id<<"] DiSR::next_free_link() found free link " << current_link <<  endl;
	    return current_link++;
	}
	current_link++;
    }
    cout << "[node "<<router->local_id<<"] DiSR::next_free_link() no link found! " << endl;

    return NOT_VALID;
}


// TODO: what happen if link status changes between two has/next free
// link function calls
int DiSR::has_free_link() const
{
    cout << "[DiSR::has_free_link() on  "<<router->local_id<<"]" << endl;
    int tmp_link = current_link;

    while (tmp_link<DIRECTION_LOCAL)
    {

	if ( (link_visited[tmp_link].isFree()) && (link_tvisited[tmp_link].isFree()))
	{
	    cout << "[node "<<router->local_id<<"] DiSR::has_free_link() found free link " << tmp_link <<  endl;
	    return tmp_link;
	}
	tmp_link++;
    }
    cout << "[node "<<router->local_id<<"] DiSR::has_free_link() no link found! " << endl;

    return NOT_VALID;
}

void DiSR::print_status() const
{
    if (this->router!=NULL)
    {
	switch (this->status) {
	    case BOOTSTRAP:
		cout << "[node "<<router->local_id<<"] DBS status:  BOOTSTRAP" << endl;
		break;
	    case READY_SEARCHING:
		cout << "[node "<<router->local_id<<"] DBS status:  READY_SEARCHING" << endl;
		break;
	    case ACTIVE_SEARCHING:
		cout << "[node "<<router->local_id<<"] DBS status:  ACTIVE_SEARCHING" << endl;
		break;
	    case CANDIDATE:
		cout << "[node "<<router->local_id<<"] DBS status:  CANDIDATE" << endl;
		break;
	    case CANDIDATE_STARTING:
		cout << "[node "<<router->local_id<<"] DBS status:  CANDIDATE_STARTING" << endl;
		break;
	    case ASSIGNED:
		cout << "[node "<<router->local_id<<"] DBS status:  ASSIGNED" << endl;
		break;
	    case FREE:
		// assuming mute as free
		// cout << "[DiSR on "<<router->local_id<<"] status:  FREE" << endl;
		break;
	    default:
		cout << "[node "<<router->local_id<<"] DBS status:  NOTVALID!!" << endl;
		exit(0);
	}
    }
    else
    {
		cout << "[node "<<router->local_id<<"] print_status:  ERROR, router not set" << endl;
    }

	    

}

// Dynamic Behaviour Status (DBS)
void DiSR::update_status()
{
    // TODO: put here some timer-like stuff ?
    if (this->status==BOOTSTRAP)
	bootstrap_node();

    if (1)
	if (this->status==ASSIGNED && has_free_link() )
	    investigate_links();

    if (this->status==CANDIDATE_STARTING)
    {
	timeout--;
	if (timeout>0)
	{
	    if (timeout%50==0)
		cout << "[node "<<router->local_id<<"] DiSR::update_status(), CANDIDATE_STARTING remaining timeout: " << timeout << endl;
	}
	else
	{
	    cout << "[node "<<router->local_id<<"] DiSR::update_status(), CANDIDATE timeout RESET!" << endl;
	    timeout = MAX_TIMEOUT;
	    this->setStatus(FREE);
	    // TODO: also reset links!
	}
    }

    // TODO: do the same for normal segment candidates ? use different
    // timeout ?
    if (this->status==CANDIDATE)
    {
	timeout--;
	if (timeout>0)
	{
	    if (timeout%50==0)
		cout << "[node "<<router->local_id<<"] DiSR::update_status(), CANDIDATE remaining timeout: " << timeout << endl;
	}
	else
	{
	    cout << "[node "<<router->local_id<<"] DiSR::update_status(), CANDIDATE timeout RESET!" << endl;
	    timeout = MAX_TIMEOUT;
	    this->setStatus(FREE);
	    // TODO: also reset links!
	}
    }
}

void DiSR::investigate_links()
{
	cout << "[node "<<router->local_id<<"] DiSR::investigate_links()" << endl;

	/////////////////////////////////////////////////////////////////////////////////
	//must search for a segment

	int candidate_link = next_free_link();

	if (candidate_link!=NOT_VALID)
	{
	    this->setStatus(READY_SEARCHING);
	    TSegmentId segment_id(router->local_id,candidate_link);
	    // mark the link and the node with id of segment request
	    link_tvisited[candidate_link] = segment_id;
	    this->setStatus(ACTIVE_SEARCHING);


	    // prepare the packet
	    TPacket packet;
	    packet.id = segment_id;
	    packet.src_id = router->local_id;
	    packet.type = SEGMENT_REQUEST;
	    packet.dir_in = DIRECTION_LOCAL;
	    packet.dir_out = candidate_link;
	    packet.hop_no = 0;

	    cout << "[node "<<router->local_id<<"] DiSR::investigate_links() injecting SEGMENT_REQUEST " << segment_id << " towards direction " << candidate_link << endl;
	    router->inject_to_network(packet);

	}
	else
	{
	    cout << "[node  "<<router->local_id<<"] DiSR::investigate_link() (no suitable links)" << endl;
	}
	///////////////////// end injecting starting segment //////

}


void DiSR::bootstrap_node()
{

	// go directly to ACTIVE status 
	cout << "[node "<<router->local_id<<"] DiSR::bootstrap_node() starting node BOOTSTRAP, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	
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

	    cout << "[node "<<router->local_id<<"] DiSR::bootstrap_node() injecting STARTING_SEGMENT_REQUEST " << segment_id << " towards direction " << candidate_link << endl;
	    router->inject_to_network(packet);

	}
	else
	{
	    cout << "[node  "<<router->local_id<<"] DiSR::bootstrap_node() CRITICAL! cant inject STARTING_SEGMENT_REQUEST (no suitable links)" << endl;
	}
	///////////////////// end injecting starting segment //////

}

// prepares the packet required to confirm the the request and inject
// it in the network.
// As a consequence, the rx process of the router will find the packet
// on the buffer in the local direction
void DiSR::generate_starting_segment_confirm(TPacket& p)
{
    TSegmentId segment_id = p.id;
    cout << "[node "<<router->local_id<<"] DiSR::generate_starting_segment_confirm() STARTING_SEGMENT_REQUEST " << segment_id << " from direction " << p.dir_in << endl;

    // change only the data required when sending back the packet

    p.type = STARTING_SEGMENT_CONFIRM;
    p.dir_out = p.dir_in;  // must return from where it came!
    p.dir_in = DIRECTION_LOCAL;

    cout << "[node "<<router->local_id<<"] DiSR::generate_starting_segment_confirm() injecting STARTING_SEGMENT_CONFIRM " << segment_id << " towards direction " << p.dir_out << endl;
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

    timeout = MAX_TIMEOUT;

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
