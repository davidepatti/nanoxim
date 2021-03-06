/*****************************************************************************

  TRouter.cpp -- Router implementation

*****************************************************************************/
#include "TRouter.h"
#include "Stats.h"

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
	if (GlobalParams::disr) disr.update_status();
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

	    if ( (req_rx[i].read()==1-current_level_rx[i]) && !buffer[i].IsFull() )
	    {
		//cout << "[node " << local_id <<"] rxProcess() can receive from dir " << i << " with non-empty buffer" << endl;
		TPacket received_packet = packet_rx[i].read();

		if(GlobalParams::verbose_mode > VERBOSE_OFF)
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
	}
	// DiSR
      if (GlobalParams::disr) this->disr.reset();
    }
  else
    {
	// For each direction, the output of the process function
      // note: since the process function can also involve control packets (e.g. DiSR) its output can be either a direction to be reserved
      // or an action to be taken (e.g. flooding)
	int process_out[DIRECTIONS+1];

	for (int d=0;d<DIRECTIONS+1;d++)
	    process_out[d] = NOT_VALID;



      // /////////////////////////////////////////////////////////////////////////
      // 1st phase: Reservation
      // /////////////////////////////////////////////////////////////////////////
	for(int j=0; j<DIRECTIONS+1; j++)
	{
	    int i = (start_from_port + j) % (DIRECTIONS + 1);


	    if ( !buffer[i].IsEmpty() )
	    {
		TPacket packet = buffer[i].Front();
		packet.dir_in = i;

		process_out[i] = process(packet);
#ifdef VERBOSE
		cout << "[node " << local_id <<"] txProcess (1st phase reservation) : buffer["<<i<<"] not empty @time " << sc_time_stamp().to_double()/1000 <<  endl;
		cout << "[node " << local_id <<"] process_out["<<i<<"]  = " << process_out[i] << " @time " << sc_time_stamp().to_double()/1000 <<  endl;
#endif

		// broadcast required //////////////////////////
		if (process_out[i] == ACTION_FLOOD)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_FLOOD [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		    vector<int> directions;

		    //  broadcast should not send to the following directions:
		    // - DIRECTION_LOCAL (that is 4)
		    // - the direction which the packet came from (that is i)
		    for (int d=0;d<DIRECTIONS;d++)
		    {
			if ( (d!=i) && (reservation_table.isAvailable(d)) )
			{
			    //cout << "[node " << local_id << "]:txProcess (flooding) adding reservation  i="<<i<<",o="<<d<<endl;
			    directions.push_back(d);
			}
		    }
		    reservation_table.reserve(i, directions);

		    // TODO: Update DiSR LED - here or in actual forwading ???
		    this->disr.setLinks(TVISITED,directions,packet.id);
		    //this->disr.forwarding_path = packet.dir_in;

		}
		else if (process_out[i]==ACTION_SKIP)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_SKIP [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		    //TODO: take some action in reservation phase ?
		}
		else if (process_out[i]==ACTION_DISCARD)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_DISCARD [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		    //TODO: take some action in reservation phase ?
		}
		else  if (process_out[i]==ACTION_END_CONFIRM)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_END_CONFIRM [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		}

		// not control mode, just reserve a direction
		else if ( (process_out[i]>=0 && process_out[i]<=4))
		{
		    if (reservation_table.isAvailable(process_out[i]) )
			reservation_table.reserve(i, process_out[i]);
		    else
			cout << "[node " << local_id << "]:txProcess WARNING not available reservation i="<<i<<",o="<<process_out<<endl;
			

		}
		else if (process_out[i]==ACTION_CONFIRM)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_CONFIRM [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		  // a confirmation packet has been injected in the local buffer that will be processed on next cycle
		  process_out[DIRECTION_LOCAL] = ACTION_SKIP;
                 
		}
		else if (process_out[i]==ACTION_CANCEL_REQUEST)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_CANCEL_REQUEST [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		  // Similar to confirmation packet, a cancel packet has been injected in the local buffer that will be processed on next cycle
		  process_out[DIRECTION_LOCAL] = ACTION_SKIP;
		}
		else  if (process_out[i]==ACTION_END_CANCEL)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_END_CANCEL [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		}
		else  if (process_out[i]==ACTION_RETRY_REQUEST)
		{
		  cout << "[node " << local_id << "]: process["<<i<<"] =  ACTION_RETRY_REQUEST [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		  // a new packet has been injected in the local buffer that will be processed on next cycle
		  process_out[DIRECTION_LOCAL] = ACTION_SKIP;
		}
		else if (process_out[i]==NOT_VALID)
		{
		  cout << "[node " << local_id << "]: WARNING, process["<<i<<"] =  NOT_VALID [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		    assert(false);
		}
		else 
		{
		  cout << "[node " << local_id << "]: CRITICAL, UNSUPPORTED process["<<i<<"] =  " << process_out[i] << " [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		    assert(false);
		}
	    }
	}
      start_from_port++;

#ifdef VERBOSE
	      cout << "[node " << local_id <<"] DEBUG between 1st phase - forwarding: " << endl;
	      for (int z=0;z<DIRECTIONS+1;z++) 
		  cout << "\t DEBUG process_out["<<z<<"] is " << process_out[z] << endl;
#endif 
      // 2nd phase: Forwarding
      for(int i=0; i<DIRECTIONS+1; i++)
      {
	  if ( !buffer[i].IsEmpty() )
	  {
#ifdef VERBOSE
	      cout << "[node " << local_id <<"] txProcess (forwarding): buffer["<<i<<"] not empty @time " << sc_time_stamp().to_double()/1000 <<  endl;
#endif 
	      TPacket packet = buffer[i].Front();

	      ////////////////////////////////////////////////////////////
	      if (process_out[i]==ACTION_FLOOD)
	      {
		  vector<int> directions = reservation_table.getMultiOutputPort(i);

		  // Note that even with ACTION_FLOOD set  directions could be an empty or single value vector, if no other channels were available at the moment 

		  if (directions.size()>0)
		  {
		      // DEBUG
		      cout << "[node " << local_id << "] FORWARDING from DIR " << i << " to multiple directions: ";
		      for (unsigned int j=0;j<directions.size();j++)
			  cout << directions[j] << ",";

		      cout << endl;

		      for (unsigned int j=0;j<directions.size();j++)
		      {
			  int o = directions[j]; // current out dir

			  if ( current_level_tx[o]== ack_tx[o].read() )
			  {
			      packet_tx[o].write(packet);
			      current_level_tx[o] = 1 - current_level_tx[o];
			      req_tx[o].write(current_level_tx[o]);

			      // DEBUG
			      //cout << "****DEBUG***** " << " node " << local_id << " is FORWARDING writing " << current_level_tx[o] << " on DIR " << o << endl;

			      // TODO: always release ?
			      reservation_table.release(o);

			  }
			  else
			  {
			      assert(false);
			  }
		      }

		  }
		  // Anyway, during flooding phase the packet should be removed from buffer, since the reserved output destinations mean
		  // that those outputs have been flooded
		  flush_buffer(i);

	      }
	      else if (process_out[i] == ACTION_DISCARD)
	      {
		  // just trash the packet 
		  flush_buffer(i);
	      }
	      else if (process_out[i] == ACTION_SKIP)
	      {
		  cout << "[node " << local_id << "] skipping processing from dir " << i << endl;
		  // skip packet processing to next cycle
	      }
	      else  if (process_out[i]==ACTION_CONFIRM)
	      {
		  // - The received packet will issue a confirmation phase
		  // starting from the current router
		  // - An appropriate confirmation packet has been generated and put
		  // to the local  buffer by the DiSR::process()
		  // - Must remove the received packet from the input buffer 
		  // - Must also cancel the state of process_out to avoid the scanning of direction to continue in
		  // this cycle
		  // - Must set the incoming link id with the proper segment id

		  flush_buffer(i);
		  //		  process_out[i] = NOT_VALID;
	      }
	      else if (process_out[i] == ACTION_END_CONFIRM)
	      {
		  // just trash the packet 
		  flush_buffer(i);
		  /*
		     cout << "[node " << local_id << "] thrashing confirmed request from dir " << i << endl;
		   */
	      }
	      else if (process_out[i]==ACTION_CANCEL_REQUEST)
	      {
		  flush_buffer(i);
	      }
	      else if (process_out[i] == ACTION_END_CANCEL)
	      {
		  // just trash the packet 
		  flush_buffer(i);
		  /*
		     cout << "[node " << local_id << "] thrashing confirmed request from dir " << i << endl;
		   */
	      }
	      else if (process_out[i]==NOT_VALID)
	      {
		  // Case 1: 
		  // this also happens when a confirmation event thrashes the
		  // received packet on a given direction D in order to
		  // inject a CONFIRM packet from the local direction towards D. The buffer[DIRECTION_LOCAL] is found not empty
		  // but the associated process_out remains NOT_VALID
		  cout << "[node " << local_id << "]: WARNING, process["<<i<<"] =  NOT_VALID [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		  assert(false);
	      }
		else  if (process_out[i]==ACTION_RETRY_REQUEST)
		{
		  flush_buffer(i);
		}

	      /// single destination, no action ////////////////////////////////
	      else if (process_out[i]>=0 && process_out[i]<=4) 
	      {
		  int o = reservation_table.getOutputPort(i);
		  if (o>=0)
		  {
		      cout << "[node " << local_id << "] FORWARDING FROM " << i << " TO " << o << endl;

		      if ( current_level_tx[o] == ack_tx[o].read() )
		      {
#ifdef VERBOSE
			  cout << "**DEBUG** " << "@node " << local_id << " @time " <<sc_time_stamp().to_double()/1000 << " ABP current_level_tx["<<o<<"]="<<current_level_tx[o] << ", ack:" << ack_tx[o].read() << " req: " << req_tx[o]<< endl;
#endif
			  packet_tx[o].write(packet);
			  current_level_tx[o] = 1 - current_level_tx[o];
			  req_tx[o].write(current_level_tx[o]);
			  flush_buffer(i);

			  // TODO: always release ?
			  reservation_table.release(o);

#ifdef VERBOSE
			  cout << "**DEBUG** " << "@node " << local_id << " @time " <<sc_time_stamp().to_double()/1000 << " ABP current_level_tx["<<o<<"]="<<current_level_tx[o] << ", ack:" << ack_tx[o].read() << " req: " << req_tx[o]<< endl;
#endif
			  // Update stats
		      }
		      else
		      {
			  cout << "WARNING " << "@node " << local_id << " @time " <<sc_time_stamp().to_double()/1000 << "___ ABP not ready____ " << endl;
			  cout << "@node " << local_id << " @time " <<sc_time_stamp().to_double()/1000 << " ABP current_level_tx["<<o<<"]="<<current_level_tx[o] << ", ack:" << ack_tx[o].read() << " req: " << req_tx[o]<< endl;
			  cout << "@node " << local_id << " @time " <<sc_time_stamp().to_double()/1000 << " releasing table entry " << o << endl;
			  reservation_table.release(o);
		      }

		  }
		  else
		  {
		      cout << "[node " << local_id << "] WARNING: no available reservation for input  " << i << endl;
		  }

		  ///////////////////////////////////////////////
	      }
	      else 
	      {
		  cout << "[node " << local_id << "]: CRITICAL, UNSUPPORTED process["<<i<<"] =  " << process_out[i] << " [id " << packet.id << "] @time " <<sc_time_stamp().to_double()/1000<<endl;
		  assert(false);
	      }
	  } // if buffer not empty
      } // for directions
    } // else read
}

//---------------------------------------------------------------------------

vector<int> TRouter::routingFunction(const TPacket& p) 
{
  TCoord position  = id2Coord(local_id);
  //TCoord src_coord = id2Coord(p.src_id);
  TCoord dst_coord = id2Coord(p.dst_id);

  switch (GlobalParams::routing_algorithm)
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
    if (GlobalParams::disr)
    {
	return this->disr.process(p);
    }

    // routing disabled
    assert(false);

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

void TRouter::flush_buffer(int d)
{
#ifdef VERBOSE
    cout << "[node " << local_id << "] flushing buffer direction " << d << endl;
#endif
    this->buffer[d].Pop();
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
	    if (my_coord.y==GlobalParams::mesh_dim_y-1) return NOT_VALID;
	    my_coord.y++;
	    break;
	case DIRECTION_EAST:
	    if (my_coord.x==GlobalParams::mesh_dim_x-1) return NOT_VALID;
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
