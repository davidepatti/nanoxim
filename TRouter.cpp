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
	// Perform all disr stuff update not striclty related to the
	// event of actually receiving a new packet. For example:
	// - bootstrapping node for first segment request
	// - TODO: updating timeouts
	if (TGlobalParams::disr) disr.update_status();
	//
	// For each channel decide if a new packet can be accepted
	//
	// This process simply sees a flow of incoming packets. All arbitration related issues are addressed in the txProcess()

	for(int i=0; i<DIRECTIONS+1; i++)
	{
	    // To accept a new packet, the following conditions must match:
	    //
	    // 1) there is an incoming request
	    // 2) there is a free slot in the input buffer of direction i

	    ///////////////
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
	  if (TGlobalParams::disr) this->disr.reset();
	}
    }
  else
    {
	// The output of the process function
      // note: since the process function can also involve control packets (e.g. DiSR) its output can be either a direction to be reserved
      // or an action to be taken (e.g. flooding)
	int process_out;

      // 1st phase: Reservation
      for(int j=0; j<DIRECTIONS+1; j++)
	{
	  int i = (start_from_port + j) % (DIRECTIONS + 1);


	  if ( !buffer[i].IsEmpty() )
	    {
	      cout << "[ROUTER " << local_id <<"] txProcess: buffer["<<i<<"] not empty" << endl;
	      TPacket packet = buffer[i].Front();
	      packet.dir_in = i;

	      process_out = process(packet);

	      // broadcast required //////////////////////////
	      if (process_out == FLOOD_MODE)
	      {
		  vector<int> directions;

		    cout << "[ROUTER " << local_id << "]:txProcess flooding packet from in " << i << " with id " << packet.id << endl; 
		    // note: broadcast should not send to the following directions:
		    // - DIRECTION_LOCAL (that is 4)
		    // - the direction which the packet came from (that is i)
		    for (int d=0;d<DIRECTIONS;d++)
		    {
			  if ( (d!=i) && (reservation_table.isAvailable(d)) )
			  {
			      cout << "[ROUTER " << local_id << "]:txProcess (flooding) adding reservation  i="<<i<<",o="<<d<<endl;
			      directions.push_back(d);

			      if(TGlobalParams::verbose_mode > VERBOSE_OFF)
			      {
				  cout << sc_time_stamp().to_double()/1000 
				      << ": Router[" << local_id 
				      << "], Input[" << i << "] (" << buffer[i].Size() << " packets)" 
				      << ", reserved Output[" << d << "], packet: " << packet << endl;
			      }		      
			  }
		  }
		  reservation_table.reserve(i, directions);
		  
		  // TODO: Update DiSR LED - here or in actual
		  // forwading ???
		  this->disr.setLinks(TVISITED,directions,packet.id);
		  this->disr.flooding_path = packet.dir_in;

	      }
	      else if (process_out==IGNORE)
	      {
		  //TODO: take some action in reservation phase ?
	      }
	      else  if (process_out==FORWARD_CONFIRM)
	      {
		  cout << "[ROUTER " << local_id << "]:txProcess FORWARD_CONFIRM, adding reservation i="<<i<<",o="<<disr.flooding_path<<endl;
                  if (reservation_table.isAvailable(this->disr.flooding_path))
		      reservation_table.reserve(i, this->disr.flooding_path);
		      else
		      cout << "[ROUTER " << local_id << "]:txProcess FORWARD_CONFIRM, CRITICAL: flooding path "<<disr.flooding_path<< " not available!" << endl;
	      }
	      else  if (process_out==END_CONFIRM)
	      {
	      }

		// not control mode, just reserve a direction
	      else if ( (process_out>=0 && process_out<=4) && (reservation_table.isAvailable(process_out) ) )
		{

		  cout << "[ROUTER " << local_id << "]:txProcess adding reservation i="<<i<<",o="<<process_out<<endl;
		  reservation_table.reserve(i, process_out);

		  if(TGlobalParams::verbose_mode > VERBOSE_OFF)
		    {
		      cout << sc_time_stamp().to_double()/1000 
			   << ": Router[" << local_id 
			   << "], Input[" << i << "] (" << buffer[i].Size() << " packets)" 
			   << ", reserved Output[" << process_out << "], packet: " << packet << endl;
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

	      ////////////////////////////////////////////////////////////
	      if (process_out==FLOOD_MODE)
	      {
		  vector<int> directions = reservation_table.getMultiOutputPort(i);

		  // Note that even with FLOOD_MODE set  directions could be an empty or single value vector, if no other channels were available at the moment 
		  // Anyway, during flooding phase the packet should be removed from buffer, since the reserved output destinations mean
		  // that those outputs have been flooded
		 
		  if (directions.size()>0)
		  {
		      // DEBUG
		      cout << "****DEBUG***** " << " ROUTER " << local_id << " FORWARDING from DIR " << i << " to MULT-DIR ";
		      for (unsigned int j=0;j<directions.size();j++)
		      {
			  cout << directions[j] << ",";
		      }
		      cout << endl;

		      for (unsigned int j=0;j<directions.size();j++)
		      {
			  int o = directions[j]; // current out dir

			  if ( current_level_tx[o]== ack_tx[o].read() )
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

			      // DEBUG
			      cout << "****DEBUG***** " << " ROUTER " << local_id << " is FORWARDING writing " << current_level_tx[o] << " on DIR " << o << endl;

			      // TODO: always release ?
			    reservation_table.release(o);

			    }
		      }

		  }
		  buffer[i].Pop();

	      }
	      else if (process_out == IGNORE)
	      {
		  // just trash the packet 
		  buffer[i].Pop();
	      }
	      /*
	      else  if (process_out==FORWARD_CONFIRM)// USELESS ??!?!?
	      {
		  int o = reservation_table.getOutputPort(i);
		  // DEBUG
		  if (o != NOT_RESERVED)
		  {
		  }
		  else
		  {
		      cout << "****DEBUG***** " << " ROUTER " << local_id << " CRITICAL: no reservation for input  " << i << endl;
		  }
	      }*/
	      else if (process_out == END_CONFIRM)
	      {
		  // just trash the packet 
		  buffer[i].Pop();
	      }
	      /// single dest ////////////////////////////////
	      else if (process_out>=0 && process_out<=4) 
	      {
		  int o = reservation_table.getOutputPort(i);
		  // DEBUG
		  if (o != NOT_RESERVED)
		    {
		      //cout << "****DEBUG***** " << " ROUTER " << local_id << " FORWARDING from DIR " << i << " to DIR " << o << endl;

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

			  // DEBUG
			  //cout << "****DEBUG***** " << " ROUTER " << local_id << " is FORWARDING writing " << current_level_tx[o] << " on DIR " << o << endl;

			  // TODO: always release ?
			reservation_table.release(o);
			    
			  // Update stats
			}
		    }
		  else
		  {
		      cout << "****DEBUG***** " << " ROUTER " << local_id << " CRITICAL: no reservation for input  " << i << endl;
		  }

		  ///////////////////////////////////////////////
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

int TRouter::process(TPacket& p)
{

    // DiSR setup traffic management
    // TODO: make it in a better way...
    if (TGlobalParams::disr)
    {
	return this->disr.process(p);
    }

    //deliver to local PE
    if (p.dst_id == local_id)
	return DIRECTION_LOCAL;

    // ...leaved for future compatibility with adaptive routing
    vector<int> candidate_channels = routingFunction(p);

    // TODO: check if ok for YX
    return candidate_channels[0];
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
  this->disr.set_router(this);
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




//---------------------------------------------------------------------------
