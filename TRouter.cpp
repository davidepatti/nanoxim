/*****************************************************************************

  TRouter.cpp -- Router implementation

*****************************************************************************/
#include "TRouter.h"

//---------------------------------------------------------------------------

void TRouter::rxProcess()
{
    if(reset.read())
    {
	// Clear outputs and indexes of receiving protocol
	for(int i=0; i<DIRECTIONS+1; i++)
	{
	    ack_rx[i].write(0);
	    current_level_rx[i] = 0;
	}
	reservation_table.clear();
    }
    else
    {
	// NB:when performing DiSR setup, new packets are buffered in
	// local PE buffer
	if (TGlobalParams::disr) 
	    DiSR_update_status();
	// For each channel decide if a new packet can be accepted
	//
	// This process simply sees a flow of incoming packets. All arbitration related issues are addressed in the txProcess()

	for(int i=0; i<DIRECTIONS+1; i++)
	{
	    // To accept a new packet, the following conditions must match:
	    //
	    // 1) there is an incoming request
	    // 2) there is a free slot in the input buffer of direction i

	    if ( (req_rx[i].read()==1-current_level_rx[i]) && !buffer[i].IsFull() )
	    {
		cout << "[ROUTER " << local_id <<"] rxProcess can receive from dir " << i << " with non-empty buffer" << endl;
		TPacket received_packet = packet_rx[i].read();

		if(TGlobalParams::verbose_mode > VERBOSE_OFF)
		{
		    cout << sc_time_stamp().to_double()/1000 << ": Router[" << local_id <<"], Input[" << i << "], Received packet: " << received_packet << endl;
		}

		// Store the incoming packet in the circular buffer
		buffer[i].Push(received_packet);            

		// Negate the old value for Alternating Bit Protocol (ABP)
		current_level_rx[i] = 1-current_level_rx[i];
	    }
	    ack_rx[i].write(current_level_rx[i]);
	}
    }
}

//---------------------------------------------------------------------------

void TRouter::txProcess()
{
  if (reset.read())
    {
      // Clear outputs and indexes of transmitting protocol
      for(int i=0; i<DIRECTIONS+1; i++)
	{
	  req_tx[i].write(0);
	  current_level_tx[i] = 0;
	  // DiSR
	  if (TGlobalParams::disr)
	      resetDiSR();
	}
    }
  else
    {
      // 1st phase: Reservation
      for(int j=0; j<DIRECTIONS+1; j++)
	{
	  int i = (start_from_port + j) % (DIRECTIONS + 1);

	  if ( !buffer[i].IsEmpty() )
	    {
	      cout << "[ROUTER " << local_id <<"] txProcess: buffer["<<i<<"] not empty" << endl;
	      TPacket packet = buffer[i].Front();
	      packet.dir_in = i;

	      int o = process(packet);

	      if (o == DIRECTION_ALL)
	      {
		    cout << "[ROUTER " << local_id << "]:txProcess should broadcast packet with id " << packet.src_id << endl; 
	      }
	      else

	      if (reservation_table.isAvailable(o))
		{
		    cout << "[ROUTER " << local_id << "]:txProcess reservation_table available with i="<<i<<",o="<<o<<endl;
		  reservation_table.reserve(i, o);
		  if(TGlobalParams::verbose_mode > VERBOSE_OFF)
		    {
		      cout << sc_time_stamp().to_double()/1000 
			   << ": Router[" << local_id 
			   << "], Input[" << i << "] (" << buffer[i].Size() << " packets)" 
			   << ", reserved Output[" << o << "], packet: " << packet << endl;
		    }		      
		}
	    }
	}
      start_from_port++;

      // 2nd phase: Forwarding
      for(int i=0; i<DIRECTIONS+1; i++)
	{
	  if ( !buffer[i].IsEmpty() )
	    {
	      TPacket packet = buffer[i].Front();

	      int o = reservation_table.getOutputPort(i);
	      if (o != NOT_RESERVED)
		{
		  if ( current_level_tx[o] == ack_tx[o].read() )
		    {
                      if(TGlobalParams::verbose_mode > VERBOSE_OFF)
			{
			  cout << sc_time_stamp().to_double()/1000 
			       << ": Router[" << local_id 
			       << "], Input[" << i << "] forward to Output[" << o << "], packet: " << packet << endl;
			}

		      packet_tx[o].write(packet);
		      current_level_tx[o] = 1 - current_level_tx[o];
		      req_tx[o].write(current_level_tx[o]);
		      buffer[i].Pop();

		      // TODO: always release ?
		    reservation_table.release(o);
			
		      // Update stats
		    }
		}
	    }
	}
    } // else
}

//---------------------------------------------------------------------------

vector<int> TRouter::routingFunction(const TPacket& p) 
{
  TCoord position  = id2Coord(local_id);
  TCoord src_coord = id2Coord(p.src_id);
  TCoord dst_coord = id2Coord(p.dst_id);

  switch (TGlobalParams::routing_algorithm)
    {
    case ROUTING_XY:
      return routingXY(position, dst_coord);

    default:
      assert(false);
    }

  assert(false);
  // something weird happened, you shouldn't be here
  return (vector<int>)(0);
}

//---------------------------------------------------------------------------

int TRouter::process(const TPacket& p)
{

    // DiSR setup traffic management
    // TODO: make it in a better way...
    if (TGlobalParams::disr)
    {
	return processDiSR(p);
    }

    //deliver to local PE
    if (p.dst_id == local_id)
	return DIRECTION_LOCAL;

    // ...leaved for future compatibility with adaptive routing
    vector<int> candidate_channels = routingFunction(p);

    // TODO: check if ok for YX
    return candidate_channels[0];
}

int TRouter::processDiSR(const TPacket& p)
{
    if ( (p.type == FIRST_SEG_REQUEST) && (p.src_id!=local_id) )
    {
	cout << "["<<local_id<<"]: received FIRST SEG REQUEST with ID " << p.src_id << endl;
	return DIRECTION_ALL;
    }

    return DIRECTION_SOUTH;
}

int TRouter::DiSR_next_free_link()
{
    while (DiSR_data.current_link<DIRECTION_LOCAL)
    {

	if ( (!DiSR_data.link_visited[DiSR_data.current_link]) && (!DiSR_data.link_tvisited[DiSR_data.current_link]))
	    return DiSR_data.current_link++;
	DiSR_data.current_link++;
    }

    return NOT_VALID;
}

void TRouter::inject_to_network(const TPacket& p)
{

    if (!buffer[DIRECTION_LOCAL].IsFull())
    {
	cout << "["<<local_id<<"]:Injecting packet!!" << endl;
	buffer[DIRECTION_LOCAL].Push(p);
    }
    else
	cout << "["<<local_id<<"]:cant Inject packet (buffer full)" << endl;
}


void TRouter::DiSR_search_first_segment()
{
	DiSR_data.status = SEARCH_FIRST_SEG;
	int candidate_link = DiSR_next_free_link();

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
	    DiSR_data.status = END_SEARCH;
	}

}

void TRouter::DiSR_update_status()
{

    if (local_id==0)
    {
	if (DiSR_data.status == INITIAL)
	{ 
	    //must search for first segment
	    DiSR_search_first_segment();
	}
    }
}


//---------------------------------------------------------------------------

vector<int> TRouter::routingXY(const TCoord& current, const TCoord& destination)
{
  vector<int> directions;
  
  if (destination.x > current.x)
    directions.push_back(DIRECTION_EAST);
  else if (destination.x < current.x)
    directions.push_back(DIRECTION_WEST);
  else if (destination.y > current.y)
    directions.push_back(DIRECTION_SOUTH);
  else
    directions.push_back(DIRECTION_NORTH);

  return directions;
}

//---------------------------------------------------------------------------

void TRouter::configure(const int _id, const unsigned int _max_buffer_size)
{
  local_id = _id;
  start_from_port = DIRECTION_LOCAL;
  

  for (int i=0; i<DIRECTIONS+1; i++)
    buffer[i].SetMaxBufferSize(_max_buffer_size);
}

//---------------------------------------------------------------------------

int TRouter::reflexDirection(int direction) const
{
    if (direction == DIRECTION_NORTH) return DIRECTION_SOUTH;
    if (direction == DIRECTION_EAST) return DIRECTION_WEST;
    if (direction == DIRECTION_WEST) return DIRECTION_EAST;
    if (direction == DIRECTION_SOUTH) return DIRECTION_NORTH;

    // you shouldn't be here
    assert(false);
    return NOT_VALID;
}

//---------------------------------------------------------------------------

int TRouter::getNeighborId(int _id, int direction) const
{
    TCoord my_coord = id2Coord(_id);

    switch (direction)
    {
	case DIRECTION_NORTH:
	    if (my_coord.y==0) return NOT_VALID;
	    my_coord.y--;
	    break;
	case DIRECTION_SOUTH:
	    if (my_coord.y==TGlobalParams::mesh_dim_y-1) return NOT_VALID;
	    my_coord.y++;
	    break;
	case DIRECTION_EAST:
	    if (my_coord.x==TGlobalParams::mesh_dim_x-1) return NOT_VALID;
	    my_coord.x++;
	    break;
	case DIRECTION_WEST:
	    if (my_coord.x==0) return NOT_VALID;
	    my_coord.x--;
	    break;
	default:
	    cout << "direction not valid : " << direction;
	    assert(false);
    }

    int neighbor_id = coord2Id(my_coord);

  return neighbor_id;
}

void TRouter::resetDiSR()
{
    DiSR_data.visited=0;
    DiSR_data.tvisited=0;
    for (int i =0;i<DIRECTIONS;i++)
    {
	DiSR_data.link_visited[i] = 0;
	DiSR_data.link_tvisited[i] = 0;
    }

    DiSR_data.starting = 0;
    DiSR_data.terminal = 0;
    DiSR_data.segment = 0;
    DiSR_data.subnet = 0;
    DiSR_data.current_link = DIRECTION_NORTH;
    DiSR_data.status = INITIAL;
}


//---------------------------------------------------------------------------
