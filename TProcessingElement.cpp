/*****************************************************************************

  TProcessingElement.cpp -- Processing Element (PE) implementation

 *****************************************************************************/
#include "TProcessingElement.h"

//---------------------------------------------------------------------------

int TProcessingElement::randInt(int min, int max)
{
  return min + (int)((double)(max-min+1) * rand()/(RAND_MAX+1.0));
}

//---------------------------------------------------------------------------

void TProcessingElement::rxProcess()
{
  if(reset.read())
  {
    ack_rx.write(0);
    current_level_rx = 0;
  }
  else
  {
    if(req_rx.read()==1-current_level_rx)
    {
      TPacket packet_tmp = packet_rx.read();
      if(GlobalParams::verbose_mode > VERBOSE_OFF)
      {
        cout << sc_simulation_time() << ": ProcessingElement[" << local_id << "] RECEIVING " << packet_tmp << endl;
      }
      current_level_rx = 1-current_level_rx;     // Negate the old value for Alternating Bit Protocol (ABP)
    }
    ack_rx.write(current_level_rx);
  }
}

//---------------------------------------------------------------------------

void TProcessingElement::txProcess()
{
    if(reset.read())
    {
	req_tx.write(0);
	current_level_tx = 0;
    }
    else
    {
	TPacket packet;

	if (canShot(packet))
	{
	    cout << "[PE "<< local_id<<"] can shot" << endl;
	    packet_queue.push(packet);
	}

	//cout << "[PE "<< local_id<<"]:txProcess (checking if ack_tx == current_level)" << endl;
	//cout << "[PE "<< local_id<<"] ack_tx " << ack_tx.read() << " current_level_tx " << current_level_tx << endl;

	if(ack_tx.read() == current_level_tx)
	{
	    if(!packet_queue.empty())
	    {
		cout << "[PE " << local_id <<"] can send and has not emtpy queue" << endl;
		TPacket packet = nextPacket();                  // Generate a new packet
		if(GlobalParams::verbose_mode > VERBOSE_OFF)
		{
		    cout << sc_time_stamp().to_double()/1000 << ": ProcessingElement[" << local_id << "] SENDING " << packet << endl;
		}
		packet_tx->write(packet);                     // Send the generated packet
		current_level_tx = 1-current_level_tx;    // Negate the old value for Alternating Bit Protocol (ABP)
		req_tx.write(current_level_tx);
	    }
	}
    }
}

//---------------------------------------------------------------------------

TPacket TProcessingElement::nextPacket()
{
  TPacket packet = packet_queue.front();

  /* TODO: already set by canShot (no more flits)
  packet.src_id      = packet.src_id;
  packet.dst_id      = packet.dst_id;
  packet.timestamp   = packet.timestamp;
  packet.hop_no      = 0;
  //  packet.payload     = DEFAULT_PAYLOAD;
  //  */

  //packet.type = SEGMENT_REQUEST;
  
  packet_queue.pop();

  return packet;
}

//---------------------------------------------------------------------------

bool TProcessingElement::canShot(TPacket& packet)
{
    bool   shot = false;
    //double threshold;
    // TODO: just to set the rate for testing...
    int rate = 100000;

    // TODO: add code here to choose PE behaviour
    int behaviour;

    behaviour = GlobalParams::disr;
    // DiSR, current testing approch:
    // - default is an XY routing where only node 0 sends packets
    // to a random destination
    // - if disr is enabled with 1, a DiSR setup is being tested.
    // In this case no packet should be generated from a PE, since is
    // more appropriate to think DiSR setup as a router configuration
    // issue.

    switch(behaviour)
    { 
	case 0:

	    if ( (local_id==0) && (((int)sc_time_stamp().to_double())%rate==0) )
	    {
		shot = true;
		packet = trafficRandom();
	    }
	    break;
	case 1:
	    return false;
	    break;

	default:
	    shot = false;
	    assert(false);
    }
    return shot;
}

//---------------------------------------------------------------------------

TPacket TProcessingElement::trafficRandom()
{
  TPacket p;
  p.src_id = local_id;
  //double range_start = 0.0;

  //cout << "\n " << sc_time_stamp().to_double()/1000 << " PE " << local_id << " rnd = " << rnd << endl;

  int max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y)-1;

  // Random destination distribution
  do
  {
    p.dst_id = randInt(0, max_id);

  } while(p.dst_id==p.src_id);

  cout << "[PE "<<local_id<<"]: created packet with dst "<<p.dst_id << endl;
  
  p.timestamp = sc_time_stamp().to_double()/1000;

  return p;
}


//---------------------------------------------------------------------------

inline double TProcessingElement::log2ceil(double x)
{
  return ceil(log(x)/log(2.0));
}


//---------------------------------------------------------------------------

void TProcessingElement::fixRanges(const TCoord src, TCoord& dst)
{
  // Fix ranges
  if(dst.x<0) dst.x=0;
  if(dst.y<0) dst.y=0;
  if(dst.x>=GlobalParams::mesh_dim_x) dst.x=GlobalParams::mesh_dim_x-1;
  if(dst.y>=GlobalParams::mesh_dim_y) dst.y=GlobalParams::mesh_dim_y-1;
}

//---------------------------------------------------------------------------
