/*****************************************************************************

  DiSR.cpp -- DiSR algorithm model implementation

  Davide Patti, davide.patti@dieei.unict.it

*****************************************************************************/
#include "nanoxim.h"
#include "TRouter.h"

//---------------------------------------------------------------------------

TSegmentId::TSegmentId()
{
    this->node = NOT_RESERVED;
    this->link = NOT_RESERVED;
}

void DiSR::set_router(TRouter * r)
{
    this->router = r;
    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    // Whener the router pointer is updated, status must be resetted
    if (r->local_id == GlobalParams::bootstrap) 
	setStatus(BOOTSTRAP);
    else
	setStatus(FREE);

    reset_cyclelinks();
}


DiSR_status DiSR::getStatus() const
{
    // sanity check of some assumed environmnet values
    switch (this->status) {
	case BOOTSTRAP:
	    assert(this->router->local_id == GlobalParams::bootstrap);
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
	    if (tvisited)
		cout << "[node " <<router->local_id<< "] DiSR::getStatus()  FREE with tvisited" << this->status << endl;
	    assert(tvisited==false);
	    assert(visited==false);
	    break;
	default:
	    cout << "[node " <<router->local_id<< "] DiSR::getStatus()  WARNING not valid status " << this->status << endl;
    }

    return (this->status);
}

bool DiSR::isAssigned() const
{
    // TODO: check if visited is equivalent to belonging to some segment
    return visited;
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

    for (unsigned int i = 0;i<directions.size();i++)
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
    TSegmentId packet_segment_id = p.id;
    TSegmentId local_segment_id = this->segID;

    ////////////////////////////////////////////////////////
    if (p.type == STARTING_SEGMENT_REQUEST )
    ////////////////////////////////////////////////////////
    {

	cout << "[node "<<router->local_id << "] DiSR::process() found STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;

	 // locally generated // //////////////////////////////////////////////////
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	     // just inject to the link found by next_free_link() during bootstrap
	    return p.dir_out;
	}
	// A starting segment request packet returned to its orginal source:
	// - must check that is not a timeout deprecated request
	// - must confirm segment (if not done yet...)
	// Note: this is a starting segment, the node is already visited, there's no need to move from tvisited to visited
	else if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    // if not confirmed yet...
	    if (!(this->segID.isAssigned()))
	    {
		// search for a tvisited links with same id
		// if not present, the id has been deprecated from timeout
		for (int d=0;d<DIRECTIONS;d++)
		{
		    if (link_tvisited[d]==packet_segment_id)
		    {
			this->segID = packet_segment_id;
			// TODO: missing the same for 'd' direction ??
			link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
			link_visited[p.dir_in] = packet_segment_id;

			// since link LED has been updated:
			reset_cyclelinks();

			cout << "[node "<<router->local_id<<"] DiSR::process() confirming STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;
			setStatus(ASSIGNED);

			generate_segment_confirm(p);
			return ACTION_CONFIRM; 
		    }
		}
		// if we are here, no tvisited link was associated to the request

		cout << "[node "<< router->local_id <<  "] DiSR::process() discarding deprecated STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;
		return ACTION_DISCARD;
	    }
	    else // starting segment with segID already confirmed...
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process() discarding already confirmed STARTING_SEGMENT_REQUEST " << packet_segment_id << endl;
		return ACTION_DISCARD;
	    }
	    
	}
	// foreign starting segment request ////////////////////////////////////
	else if (  p.src_id!=router->local_id ) 
	{
	    // free nodes should contribute to flooding, wihtout looking at the segment id. 
	    // This becasue there's no way of figuring out if the id is a deprecated flooding...
	    if (this->getStatus()==FREE)
	    {
		cout << "[node "<<router->local_id << "] DiSR::process() enable ACTION_FLOOD" << endl;

		this->segID = packet_segment_id;
		tvisited = true;
		link_tvisited[p.dir_in] = packet_segment_id;

		this->set_request_path(p.dir_in);

		setStatus(CANDIDATE_STARTING);
		// TODO: after flooding links has been selecte by the
		// router, the LED variable should be update accordingly
		return ACTION_FLOOD;
	    } 
	    // a node that has already done flooding must check if the id of the request is newer
	    // In that case, must re-do the flooding with the new id
	    else if (this->getStatus()==CANDIDATE_STARTING )
	    {
		if ( this->segID.getLink()<packet_segment_id.getLink())
		{
		    cout << "[node "<<router->local_id << "] DiSR::process() STARTING_SEGMENT_REQUEST " << packet_segment_id << " overwrites deprecated  " << this->segID << ", enable ACTION_FLOOD" << endl;
		    // reset the previous LED data
		    for (int i=0;i<DIRECTIONS;i++)
		    {
			// not valid direction (e.g. borderline) shouldn't be updated
			if (link_visited[i].isValid() && link_tvisited[i].isValid())
			{
			    link_visited[i].set(NOT_RESERVED,NOT_RESERVED);
			    link_tvisited[i].set(NOT_RESERVED,NOT_RESERVED);
			}

		    }
		    // since link status has been updated, the pointer of the current link to be investigated must be resetted
		    reset_cyclelinks();

		    this->segID = packet_segment_id;
		    tvisited = true;
		    link_tvisited[p.dir_in] = packet_segment_id;

		    this->set_request_path(p.dir_in);

		    setStatus(CANDIDATE_STARTING);
		    // TODO: after flooding links has been selecte by the
		    // router, the LED variable should be update accordingly
		    return ACTION_FLOOD;
		}
		else
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process() already candidate " << this->segID << " discarding STARTING_SEGMENT_REQUEST  " << packet_segment_id << endl;
		    print_status();
		    return ACTION_DISCARD;
		}
	    }
		// not free node should ignore flooding and discard packet, reasons:
		// - already candidate starting, so flooding already done
		// - already assigned, so starting segment request deprecated
	    else
	    {
		print_status();
		cout << "[node "<< router->local_id <<  "] DiSR::process() discarding flooding " << endl;
		return ACTION_DISCARD;
	    }
	    
	}

	assert(false);
    }
    /////////////////////////////////////////////////////////
    else if (p.type == SEGMENT_REQUEST)
    ////////////////////////////////////////////////////////
    {
	cout << "[node "<<router->local_id << "] DiSR::process() found SEGMENT_REQUEST with ID " << packet_segment_id << endl;

	// present in the local buffer, not necessarily locally
	// generate (e.g. when occurring ACTION_RETRY_REQUEST)
	if ( p.dir_in==DIRECTION_LOCAL ) /////////////////////////////////////////////////////////////////////////////////////////////////
	{
	     // just inject to the link found by next_free_link() 
	    return p.dir_out;
	}
	// foreign segment request
	else if (  p.src_id!=router->local_id ) /////////////////////////////////////////////////////////////////////////////////////////
	{
	    /* A SEGMENT_REQUEST packet reached a free node
	     * If a free link is available node becomes CANDIDATE and forward the packet
	     * Otherwise, segment request is cancelled along that direction  */
	    if (this->getStatus()==FREE)
	    {
		// first of all, the incoming direction becomes tvisited 
		link_visited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
		link_tvisited[p.dir_in] = packet_segment_id;

		//next, search for a suitable link
		int freedirection = next_free_link();

		// a free direction has been found...
		if ( (freedirection>=0) && (freedirection<DIRECTION_LOCAL) )
		{
		    this->segID = packet_segment_id;
		    this->tvisited = true;
		    this->visited = false;

		    link_tvisited[freedirection] = packet_segment_id;

		    cout << "[node "<< router->local_id <<  "] DiSR::process() setting CANDIDATE id " << this->segID << " forwarding to " << freedirection <<endl;
		    this->setStatus(CANDIDATE);
		    set_request_path(p.dir_in); // future confirm packet will be forwarded along this direction 
		    // TODO: more consistenly update this value, (e.g.  // currently updated also in ACTION_FLOOD of // TRouter...)
		    return freedirection;
		}
		else if (freedirection == CYCLE_TIMEOUT)
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process(), CYCLE_TIMEOUT, cancelling SEGMENT_REQUEST id " << packet_segment_id << endl;
		    free_direction(p.dir_in);
		    generate_segment_cancel(p);
		    return ACTION_CANCEL_REQUEST;
		}
		else if (freedirection == NO_LINK)
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process(), re-processing SEGMENT_REQUEST id at next cycle " << packet_segment_id << endl;
		    return ACTION_SKIP;
		}
		else
		    assert(false);


	    }
	    /*
	    // A SEGMENT_REQUEST packet reached a node candidate for starting segment:
	    // - In every case, this means the starting segment has already been estabilished and the node must cancel a previous LED settings
	    // - Further, if a free link is available, should forward
	    // the request  packet, otherwise send a cancel request
	    //
	    // TODO: UPDATE CRITICAL
	    // This is not ncessarily true: that is, maybe the node WILL BELONG to the starting segment that is being
	    // formed, so the segment request should not interrupt this process! */
	    else if (this->getStatus()==CANDIDATE_STARTING) 
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process() cancelling previous CANDIDATE_STARTING " << this->segID << " due SEGMENT_REQUEST with id " << packet_segment_id << endl;
		// reset the previous LED data
		for (int i=0;i<DIRECTIONS;i++)
		{
		    // not valid direction (e.g. borderline) shouldn't
		    // be updated
		    if (link_visited[i].isValid() && link_tvisited[i].isValid())
		    {
			link_visited[i].set(NOT_RESERVED,NOT_RESERVED);
			link_tvisited[i].set(NOT_RESERVED,NOT_RESERVED);
		    }

		}
		// since link status has been updated, the pointer of
		// the current link to be investigated must be resetted
		reset_cyclelinks();

		// the incoming direction becomes tvisited 
		link_visited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
		link_tvisited[p.dir_in] = packet_segment_id;

		int freedirection = next_free_link();

		if ( (freedirection>=0) && (freedirection<DIRECTION_LOCAL) )
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process() setting CANDIDATE with id " << packet_segment_id << endl;
		    this->segID = packet_segment_id;
		    this->tvisited = true;
		    this->visited = false;

		    link_tvisited[freedirection] = packet_segment_id;

		    this->setStatus(CANDIDATE);
		    this->set_request_path(p.dir_in); // future confirm packet will be forwarded along this direction 
		    return freedirection;
		}
		else if (freedirection==CYCLE_TIMEOUT)
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process(), CYCLE_TIMEOUT, cancelling SEGMENT_REQUEST id " << packet_segment_id << endl;
		    // This can happen when defective links are present
		    free_direction(p.dir_in);
		    generate_segment_cancel(p);
		    return ACTION_CANCEL_REQUEST;
		}
		else if (freedirection==NO_LINK)
		{
		    cout << "WARNING [node "<< router->local_id <<  "] DiSR::process() no free link after cancelling a CANDIDATE_STARTING " << packet_segment_id << endl;
		    free_direction(p.dir_in);
		    generate_segment_cancel(p);
		    return ACTION_CANCEL_REQUEST;
		}
		else
		    assert(false);
	    }
	    // a segment request reached a node currently performing a find segment process
	    // This means that the node is already assigned and visited and thus can confirm the request
	    // NOTE: this is the same as ASSIGNED case below
	    else if (this->getStatus()==ACTIVE_SEARCHING)
	    {
		assert(this->visited);
		// the incoming direction becomes visited 
		link_visited[p.dir_in] = packet_segment_id;
		link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
		generate_segment_confirm(p);
		this->set_request_path(p.dir_in); // future confirm packet will be forwarded along this direction 
		return ACTION_CONFIRM;

	    }
	    // a segment request reached an already assigned node
	    // thus can confirm the request
	    // NOTE: this is the same as ACTIVE_SEARCHING case, the only difference is that no search is currently ongoing (e.g. no free links)
	    else if (this->getStatus()==ASSIGNED)
	    {
		assert(this->visited);
		// the incoming direction becomes visited 
		link_visited[p.dir_in] = packet_segment_id;
		link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
		generate_segment_confirm(p);
		this->set_request_path(p.dir_in); // future confirm packet will be forwarded along this direction 
		return ACTION_CONFIRM;

	    }
	    // a segment request reached a node already candidate, cancel the request along that direction
	    // TODO: or shuould retry ? if the node becomes assigned
	    // the segment could be confirmed !
	    //
	    else if (this->getStatus()==CANDIDATE)
	    {
		// a locally generated segment request that has been already processed
		if (this->segID==packet_segment_id && p.dir_in==DIRECTION_LOCAL)
		{
		    cout << "[node "<< router->local_id <<  "] WARNING: already processed locally generated SEGMENT_REQUEST  " << packet_segment_id << endl;

		    // re-try along the same direction...

		    for (int i=0;i<DIRECTIONS;i++)
		    {
			// check for the previously reserved direction
			// note that we should avoid the incoming request path, which is also tvisited
			if (link_tvisited[i] == packet_segment_id && (i!=request_path) )
			{
			    cout << "[node "<< router->local_id <<  "] DiSR::process() re-trying SEGMENT_REQUEST  " << packet_segment_id << " along DIR " << i << endl;
			    return i;
			}
		    }
		    // you shouldn't be here...
		    cout << "CRITICAL [node "<< router->local_id <<  "] DiSR::process() no reserved direction for already processed SEGMENT_REQUEST  " << packet_segment_id << endl;
		    assert(false);
		}
		// In two cases an already candidate node should reject request:
		// - the request id is different (of course...)
		// - the request id is the same for which the node is candidate, but the packet does NOT come from local direction, i.e. made a loop path 
		// returning to the node
		else
		{
		    if (!(this->segID==packet_segment_id))
			cout << "[node "<< router->local_id <<  "] DiSR::process() already CANDIDATE with id " << this->segID << ", cancelling request "<< packet_segment_id << " from " << p.dir_in << endl;
		    else
			cout << "[node "<< router->local_id <<  "] DiSR::process() CANDIDATE with same id, discarding request"<< packet_segment_id << " from " << p.dir_in << ", forwarding already done! " << endl;

		    free_direction(p.dir_in);
		    generate_segment_cancel(p);
		    // the incoming direction becomes free
		    return ACTION_CANCEL_REQUEST;
		}

	    }
	    else
	    {
		// TODO: catch any remainig case
		cout << "[node "<< router->local_id <<  "] DiSR::process() CRITICAL, unsupported status" << this->getStatus() << endl;
		print_status();
		assert(false);
		return NOT_VALID;
	    }
	    
	}
	// the request packet returned to its original source
	// must cancel the request
	else if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    // trivial sanity check, since it's a request originated from this node...
	    //assert(this->segID==packet_segment_id);

	    cout << "[node "<< router->local_id <<  "] DiSR::process() CANDIDATE with same id, discarding request"<< packet_segment_id << " from " << p.dir_in << ", current node is the request issuer! " << endl;

	    // the incoming direction becomes free
	    free_direction(p.dir_in);
	    generate_segment_cancel(p);
	    return ACTION_CANCEL_REQUEST;
	}
	assert(false);
    }

    /////////////////////////////////////////////////////////
    else if (p.type == STARTING_SEGMENT_CONFIRM)
    ////////////////////////////////////////////////////////
    {

	cout << "[node "<<router->local_id << "] DiSR::process() processing STARTING_SEGMENT_CONFIRM with ID " << packet_segment_id << endl;

	 // locally generated starting segment packet confirmation, 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id << "] DiSR::process() sending locally generated STARTING_SEGMENT_CONFIRM with id " << packet_segment_id << " towards " << p.dir_out << endl;
	     // inject the packet to the appropriate link previously found by generate_segment_confirm()
	    return p.dir_out;
	}
	// foreign confirm segment request
	else if (  p.src_id!=router->local_id ) 
	{

	    if  ( (this->getStatus()==CANDIDATE_STARTING) && (local_segment_id == packet_segment_id) )
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process()  CANDIDATE_STARTING to segment id: " << local_segment_id << " has been **ASSIGNED!**" << endl;
		// node status changes from tvisited to visited
		this->tvisited = false;
		this->visited = true;

		// the incoming link and flooding path change from tvsited to visited with the segment id
		this->link_visited[p.dir_in] = packet_segment_id;
		this->link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);

		this->link_visited[this->request_path] = packet_segment_id;
		this->link_tvisited[this->request_path].set(NOT_RESERVED,NOT_RESERVED);

		// ...while all other links can return free....
		for (int i=0;i<DIRECTIONS;i++) {
		    if ( (i!=this->request_path) && (i!=p.dir_in) )
		    {
			// exclude not valid directions (e.g. boundaries)

			if (link_visited[i].isValid())
			{
			    link_visited[i].set(NOT_RESERVED,NOT_RESERVED);
			    link_tvisited[i].set(NOT_RESERVED,NOT_RESERVED);
			}
		    }
		}

		// since link status has been updated, the pointer of
		// the current link to be investigated must be resetted
		reset_cyclelinks();

		// all other directions tvisitred during flooding
		// should be released
		// TODO: deprecated ? this->release_forwarding_paths();

		this->setStatus(ASSIGNED);
		cout << "[node "<< router->local_id <<  "] DiSR::process()  forwarding STARTING_SEGMENT_CONFIRM " << packet_segment_id << " back to " << this->request_path << endl;
		return this->request_path;
		
		// return FORWARD_CONFIRM;
	    }
	    else 
	    {
		cout << "[node "<<router->local_id << "] DiSR::process()  CRITICAL, receving STARTING_SEGMENT_CONFIRM with inconsistent environment:" << endl;
		cout << "[node "<<router->local_id << "] DiSR::process()  local segid = " << local_segment_id << " , packet id = " << packet_segment_id << ", ";
		print_status();
		assert(false);
		return ACTION_SKIP;
	    }
	    
	    
	}
	else // the confirmation packet returned to its orginal source, all done!
	if ( (p.src_id==router->local_id) && (p.dir_in!=DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id<<"] DiSR::process()  STARTING_SEGMENT_CONFIRM id " << packet_segment_id <<  " ended !" << endl;
	    setStatus(ASSIGNED);

	    // there's no need to set as visited, since the
	    // initiator must be visited by definition

	    // the incoming link changes from tvsited to visited with the segment id
	    this->link_visited[p.dir_in] = packet_segment_id;
	    this->link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);
	    return ACTION_END_CONFIRM; 
	}
	assert(false);
    }
    /////////////////////////////////////////////////////////
    else if (p.type == SEGMENT_CONFIRM)
    ////////////////////////////////////////////////////////
    {

	cout << "[node "<<router->local_id << "] DiSR::process() processing SEGMENT_CONFIRM with ID " << packet_segment_id << endl;

	 // locally generated segment packet confirmation, 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id << "] DiSR::process() sending locally generated SEGMENT_CONFIRM with id " << packet_segment_id << " towards " << p.dir_out << endl;
	     // inject the packet to the appropriate link previously found by generate_segment_confirm()
	    return p.dir_out;
	}
	// not-local confirm segment packet
	else if (  p.src_id!=router->local_id ) 
	{
	    // the confirmation packet returned to segment initiator, all done!
	    if ( (packet_segment_id.getNode()==router->local_id) )
	    {
		cout << "[node "<<router->local_id<<"] DiSR::process()  SEGMENT_CONFIRM id " << packet_segment_id <<  " ended !" << endl;

		// there's no need to set as visited, since the
		// initiator must be visited by definition

		// the incoming link changes from tvsited to visited with the segment id
		this->link_visited[p.dir_in] = packet_segment_id;
		this->link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);

		setStatus(ASSIGNED);
		return ACTION_END_CONFIRM; 
	    }
	    else

	    // any candidate node must become assigned and forward
	    if  ( (this->getStatus()==CANDIDATE) && (local_segment_id == packet_segment_id) )
	    {

		cout << "[node "<< router->local_id <<  "] DiSR::process()  CANDIDATE to segment id: " << local_segment_id << " has been **ASSIGNED!**" << endl;
		setStatus(ASSIGNED);

		// node status changes from tvisited to visited
		this->tvisited = false;
		this->visited = true;

		// the incoming link and forward path change from tvsited to visited with the segment id
		this->link_visited[p.dir_in] = packet_segment_id;
		this->link_tvisited[p.dir_in].set(NOT_RESERVED,NOT_RESERVED);

		this->link_visited[this->request_path] = packet_segment_id;
		this->link_tvisited[this->request_path].set(NOT_RESERVED,NOT_RESERVED);

		cout << "[node "<< router->local_id <<  "] DiSR::process()  forwarding  SEGMENT_CONFIRM " << packet_segment_id << " back to " << this->request_path << endl;
		return this->request_path;
		
	    }
	    else 
	    {
		cout << "[node "<<router->local_id << "] DiSR::process()  CRITICAL, receving SEGMENT_CONFIRM with inconsistent environment:" << endl;
		cout << "[node "<<router->local_id << "] DiSR::process()  local segid = " << local_segment_id << " , packet id = " << packet_segment_id << ", ";
		print_status();
		assert(false);
		return ACTION_SKIP;
	    }
	    
	    
	}
	assert(false);
    } // SEGMENT_CONFIRM

    /////////////////////////////////////////////////////////
    else if (p.type == SEGMENT_CANCEL)
    ////////////////////////////////////////////////////////
    {
	cout << "[node "<<router->local_id << "] DiSR::process() processing SEGMENT_CANCEL " << packet_segment_id << endl;

	 // locally generated segment packet cancel, 
	if ( (p.src_id==router->local_id) && (p.dir_in==DIRECTION_LOCAL) )
	{
	    cout << "[node "<<router->local_id << "] DiSR::process() sending locally generated SEGMENT_CANCEL " << packet_segment_id << " towards " << p.dir_out << endl;
	     // inject the packet to the appropriate link previously found by generate_segment_cancel()
	    return p.dir_out;
	}
	// not-locally generated cancel segment packet
	else 
	{
	    // IMPORTANT: the incoming should be set as free in every case, but
	    // is cleaned only after the following, so that is not considered when searching 
	    // next_free_link()
	    int new_direction = next_free_link();

	    // the incoming link changes from tvsited to free 
	    this->free_direction(p.dir_in);

	    if ( (new_direction>=0) && (new_direction<DIRECTION_LOCAL) )
	    {
		// the choosen direction becomes tvisited
		link_tvisited[new_direction] = packet_segment_id;

		// prepare the new request packet usign infos from cancel packet
		TPacket packet;
		packet.id = packet_segment_id;
		packet.src_id = packet_segment_id.getNode(); //same as the original segment request packet
		packet.type = SEGMENT_REQUEST;
		packet.dir_in = DIRECTION_LOCAL;
		packet.dir_out = new_direction;
		packet.hop_no = p.hop_no;

		cout << "[node "<< router->local_id <<  "] DiSR::process() Retrying SEGMENT_REQUEST " << this->segID << " along " << new_direction <<endl;

		router->inject_to_network(packet);
		return ACTION_RETRY_REQUEST;
	    }
	    else if (new_direction==CYCLE_TIMEOUT) // no free links to retry the request
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process(), CYCLE_TIMEOUT for SEGMENT_REQUEST " << packet_segment_id  << endl;

		// if the initial request started from here, no need
		// to forward the segment cancel 
		if (this->router->local_id == packet_segment_id.getNode() )
		{
		    cout << "[node "<< router->local_id <<  "] DiSR::process(), ending request on initiator " << packet_segment_id  << endl;
		    return ACTION_END_CANCEL;
		}
		else
		{
		    // even the request path should be cleared
		    this->free_direction(this->request_path);

		    // node status changes from tvisited to free
		    this->tvisited = false;
		    this->visited = false;
		    this->setStatus(FREE);
		    cout << "[node "<< router->local_id <<  "] DiSR::process(), forwarding SEGMENT_CANCEL " << packet_segment_id << " back to " << this->request_path << endl;
		    // is it not necessary to use generate_segment_cancel(), since we already have a cancel packet
		    return this->request_path;
		}

		// TODO: should we do this , when ?
		// reset free link search pointer, since LED have been
		// updated
		assert(false);
		reset_cyclelinks();

		//cout << "[node "<< router->local_id <<  "] DiSR::process()  freeing request path " << this->request_path << " and incoming dir " << p.dir_in << endl;
		//cout << "[node "<< router->local_id <<  "] DiSR::process()  current_link = " << this->current_link << endl;
	    }
	    else if (new_direction==NO_LINK)
	    {
		cout << "[node "<< router->local_id <<  "] DiSR::process(), re-processing SEGMENT_CANCEL id " << packet_segment_id << " at next cycle " << endl;
		return ACTION_SKIP;
	    }
	    
	} // locally/not locally
	// why no of the above cases has been matched ?!?
	assert(false);
    }
    assert(false);
    return NOT_VALID;
}


void DiSR::set_request_path(int path)
{
#ifdef VERBOSE
    cout << "[node "<<router->local_id<<"] DiSR::set_request_path() to " << path <<  endl;
#endif
    this->request_path = path;
}


void DiSR::reset_cyclelinks()
{
    this->cyclelinks_timeout = GlobalParams::cyclelinks;
    this->cycle_start = 0;
    this->current_link = 0;
#ifdef VERBOSE
    cout << "[node "<<router->local_id<<"] DiSR::reset_cyclelinks() setting cycle_start =  " << current_link << endl;
#endif
}


int DiSR::next_free_link()
{
    if (current_link==DIRECTION_LOCAL) 
    {
	//cout << "[DiSR::next_free_link() on  "<<router->local_id<<"] re-starting cycle... " << current_link << endl;
	current_link=DIRECTION_NORTH;
    }
    cout << "[node "<<router->local_id<<"] DiSR::next_free_link() start = " << cycle_start << ", curr="<<current_link << endl;

    // new Semantic:
    //
    // - if a free link is found: return its direction and set current to the next to be analyzed
    // - if no free link is found: the cycle is stopped when current = start, timeout is decreased, and if 0, CYCLE_TIMEOUT, otherwise NO_LINK

    // if start is moved multiple free direction found can last endlessly because start is NOT touched, e.g. found 1, next found 0, next found 3, next found 2

    bool stop = false;
    int found_dir = NO_LINK;

    if (this->cyclelinks_timeout==0)
    {
	cout << "[node "<<router->local_id<<"] DiSR::next_free_link() CYCLE_TIMEOUT " << endl;
	reset_cyclelinks();
	return CYCLE_TIMEOUT;
    }

    while (!stop)
    {
	cout << "[node "<<router->local_id<<"] DiSR::next_free_link() analyzing direction " << current_link << endl;

	if ( (link_visited[current_link].isFree()) && (link_tvisited[current_link].isFree()))
	{
	    found_dir = current_link;
	    stop = true;
	}

	current_link++;
	if (current_link==DIRECTION_LOCAL) 
	{
	    //cout << "[DiSR::next_free_link() on  "<<router->local_id<<"] re-starting cycle... " << current_link << endl;
	    current_link=DIRECTION_NORTH;
	}

	// cycle completed: decrease timeout, stop search and if timeout not yet 0 then wait for next invocation...
	if (current_link==cycle_start)
	{
	    this->cyclelinks_timeout--;
	    cout << "[node "<<router->local_id<<"] DiSR::next_free_link() completed cycle at next dir " << current_link << ", timeout "<<cyclelinks_timeout<<"/"<<GlobalParams::cyclelinks<<endl;
	    stop = true;
	} 
    }
    if (found_dir==NO_LINK) cout << "no link found! " << endl;
    else
	cout << " found dir " << found_dir << endl;

    return found_dir;
}

// TOTOD: check if useless
bool DiSR::has_free_link() const
{
    int tmp_link = current_link;

    bool stop = false;

    if (tmp_link==DIRECTION_LOCAL) tmp_link=DIRECTION_NORTH;

    int start = tmp_link;

    while (!stop)
    {
#ifdef VERBOSE
	cout << "[node "<<router->local_id<<"] DiSR::has_free_link() analyzing direction " << tmp_link << endl;
#endif


	if ( (link_visited[tmp_link].isFree()) && (link_tvisited[tmp_link].isFree()))
	{
#ifdef VERBOSE
	    cout << "found free link " << tmp_link <<  endl;
#endif

	    return true;
	}
	tmp_link++;

	if (tmp_link==DIRECTION_LOCAL) tmp_link=DIRECTION_NORTH;

	if (tmp_link==start) stop = true;
    }
#ifdef VERBOSE
    cout << "no link found! " << endl;
#endif

    return false;
}

void DiSR::print_status() const
{
#ifdef VERBOSE
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

#endif
	    

}

// Dynamic Behaviour Status (DBS)
void DiSR::update_status()
{
    //cout << "[node "<<router->local_id<<"] DiSR::update_status()... " << endl;
    //print_status();
    // TODO: put here some timer-like stuff ?
    if (this->status==BOOTSTRAP)
	bootstrap_node();

    if (this->status==ASSIGNED && has_free_link() )
    {
	cout << "[node "<<router->local_id<<"] DiSR::update_status(), can start_investigate_links() ! " << endl;
	start_investigate_links();
    }

    // if this is the initial node the bootstrapped the starting segmnet request
    if (this->status==CANDIDATE_STARTING && this->visited)
    {
	// timeout is not unlimited
	if (bootstrap_timeout!=-1)
	{
	    bootstrap_timeout--;
	    if (bootstrap_timeout>0)
	    {
		if (bootstrap_timeout%1==0)
		    cout << "[node "<<router->local_id<<"] DiSR::update_status(), bootstrap node emaining timeout: " << bootstrap_timeout << endl;
	    }
	    else // this condition should be really critical, assuming a proper timeout has been used
	    {
		cout << "CRITICAL [node "<<router->local_id<<"] DiSR::update_status(), bootstrap timeout RESET!" << endl;
		//assert(false);
		bootstrap_timeout = GlobalParams::bootstrap_timeout;
		this->setStatus(BOOTSTRAP);
	    }
	}
    }

}

// a visited node connected to not visited/tvisited links can start
// investigating its links
void DiSR::start_investigate_links()
{
	cout << "[node "<<router->local_id<<"] DiSR::start_investigate_links()" << endl;

	/////////////////////////////////////////////////////////////////////////////////
	//must search for a segment

	int candidate_link = next_free_link();

	if ( (candidate_link>=0) && (candidate_link<DIRECTION_LOCAL) )
	{
	    this->setStatus(READY_SEARCHING);
	    TSegmentId segment_id;
	    segment_id.set(router->local_id,candidate_link);
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

	    cout << "[node "<<router->local_id<<"] DiSR::start_investigate_links() injecting SEGMENT_REQUEST " << segment_id << " towards direction " << candidate_link << endl;
	    router->inject_to_network(packet);

	}
	else
	{
	    cout << "[node  "<<router->local_id<<"] DiSR::start_investigate_link() WARNING, no suitable links!" << endl;
	}

}

void DiSR::bootstrap_node()
{

	// go directly to ACTIVE status 
	cout << "[node "<<router->local_id<<"] DiSR::bootstrap_node() starting node BOOTSTRAP, ready to inject STARTING_SEGMENT_REQUEST" << endl;
	
	/////////////////////////////////////////////////////////////////////////////////
	//must search for starting segment

	int candidate_link = next_free_link();

	if (candidate_link>=0 && candidate_link < DIRECTIONS)
	{
	    TSegmentId segment_id;
	    segment_id.set(router->local_id,candidate_link);
	    // mark the link with id of segment request
	    link_tvisited[candidate_link] = segment_id;
	    visited = true;
	    this->setStatus(CANDIDATE_STARTING);


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
void DiSR::generate_segment_confirm(TPacket& p)
{
    TSegmentId segment_id = p.id;

    if (p.type==STARTING_SEGMENT_REQUEST)
    {
	cout << "[node "<<router->local_id<<"] DiSR::generate_segment_confirm() STARTING_SEGMENT_REQUEST " << segment_id << " from direction " << p.dir_in << endl;
	p.type = STARTING_SEGMENT_CONFIRM;
    }
    else if (p.type==SEGMENT_REQUEST)
    {
	cout << "[node "<<router->local_id<<"] DiSR::generate_segment_confirm() SEGMENT_REQUEST " << segment_id << " from direction " << p.dir_in << endl;
	p.type = SEGMENT_CONFIRM;

    }
    else
    {
	assert(false);
    }

    // change only the data required when sending back the packet
    p.dir_out = p.dir_in;  // must return from where it came!
    p.dir_in = DIRECTION_LOCAL;
    p.src_id = router->local_id; // required in non-starting segment confirmation packets

    cout << "[node "<<router->local_id<<"] DiSR::generate_segment_confirm() injecting confirm with id " << segment_id << " towards direction " << p.dir_out << endl;
    router->inject_to_network(p);

}

void DiSR::generate_segment_cancel(TPacket& p)
{
    TSegmentId segment_id = p.id;

    if (p.type==SEGMENT_REQUEST)
    {
	cout << "[node "<<router->local_id<<"] DiSR::generate_segment_cancel() cancelling " << segment_id << " from direction " << p.dir_in << endl;
	p.type = SEGMENT_CANCEL;
    }
    else
    {
	assert(false);
    }

    // change only the data required when sending back the packet
    p.dir_out = p.dir_in;  // must return from where it came!
    p.dir_in = DIRECTION_LOCAL;
    p.src_id = router->local_id; // required in non-starting segment confirmation packets

    cout << "[node "<<router->local_id<<"] DiSR::generate_segment_cancel() injecting SEGMENT_CANCEL with id " << segment_id << " towards direction " << p.dir_out << endl;
    router->inject_to_network(p);

}

void DiSR::reset()
{
    visited= false;
    tvisited= false;
    segID.set(NOT_RESERVED,NOT_RESERVED);
    //starting = false;
    terminal = false;
    subnet = NOT_RESERVED;
    current_link = DIRECTION_NORTH;

    for (int i =0;i<DIRECTIONS;i++)
    {
	// note: not valid links shouldnt' be set as free
	if (link_visited[i].isValid())
	{
	    link_visited[i].set(NOT_RESERVED,NOT_RESERVED);
	    link_tvisited[i].set(NOT_RESERVED,NOT_RESERVED);
	}
    }

    // To test the model, the node 0 is always used for bootstrapping 
    // the whole algorithm
    // Whener the router pointer is updated, status must be resetted


    if ((router!=NULL) && (router->local_id == GlobalParams::bootstrap)) 
    {
	bootstrap_timeout = GlobalParams::bootstrap_timeout;
	status = BOOTSTRAP;
    }
    else
	status = FREE;
}

void DiSR::invalidate_direction(int d)
{
    link_visited[d].set(NOT_VALID,NOT_VALID);
    link_tvisited[d].set(NOT_VALID,NOT_VALID);

}

void DiSR::free_direction(int d)
{
    if (link_visited[d].isAssigned()) 
	cout << "\n WARNING: avoiding freeing ASSIGNED link on DIR " << d << endl;
    else
	link_visited[d].set(NOT_RESERVED,NOT_RESERVED);

    if (!link_visited[d].isValid()) cout << "\n WARNING: freeing NOT VALID link on DIR " << d << endl;
    assert(link_visited[d].isValid());
    //assert(!link_visited[d].isAssigned());
    link_tvisited[d].set(NOT_RESERVED,NOT_RESERVED);

}


bool DiSR::sanity_check()
{
    return true;
}

TSegmentId DiSR::getLocalSegmentID() const
{
    return this->segID;
}
TSegmentId DiSR::getLinkSegmentID(int d) const
{
    assert(d<DIRECTIONS);
    return this->link_visited[d];
}
