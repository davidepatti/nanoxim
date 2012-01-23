#ifndef __NANOXIM_DEFS_H__
#define __NANOXIM_DEFS_H__

//---------------------------------------------------------------------------

#include <cassert>
#include <systemc.h>
#include <vector>

using namespace std;


// Define the directions as numbers
#define DIRECTIONS             4
#define DIRECTION_NORTH        0
#define DIRECTION_EAST         1
#define DIRECTION_SOUTH        2
#define DIRECTION_WEST         3
#define DIRECTION_LOCAL        4
#define DIRECTION_ALL	       99

// Generic not reserved resource
#define NOT_RESERVED          -2

// To mark invalid or non exhistent values
#define NOT_VALID             -1

// Routing algorithms
#define ROUTING_XY             0


// Verbosity levels
#define VERBOSE_OFF            0
#define VERBOSE_LOW            1
#define VERBOSE_MEDIUM         2
#define VERBOSE_HIGH           3
//---------------------------------------------------------------------------

// Default configuration can be overridden with command-line arguments
#define DEFAULT_VERBOSE_MODE               VERBOSE_OFF
#define DEFAULT_MESH_DIM_X                           4
#define DEFAULT_MESH_DIM_Y                           4
#define DEFAULT_BUFFER_DEPTH                         4
#define DEFAULT_SIMULATION_TIME                  10000
#define DEFAULT_STATS_WARM_UP_TIME  DEFAULT_RESET_TIME
#define DEFAULT_DETAILED                         false
#define DEFAULT_RESET_TIME                        1000
#define DEFAULT_DISR_SETUP			     0

// TODO by Fafa - this MUST be removed!!!
#define MAX_STATIC_DIM 20

//---------------------------------------------------------------------------
// TGlobalParams -- used to forward configuration to every sub-block
struct TGlobalParams
{
  static int verbose_mode;
  static int mesh_dim_x;
  static int mesh_dim_y;
  static int buffer_depth;
  static int routing_algorithm;
  static int simulation_time;
  static int rnd_generator_seed;
  static int disr;
};


//---------------------------------------------------------------------------
// TCoord -- XY coordinates type of the Tile inside the Mesh
class TCoord
{
 public:
  int                x;            // X coordinate
  int                y;            // Y coordinate

  inline bool operator == (const TCoord& coord) const
  {
    return (coord.x==x && coord.y==y);
  }
};



//---------------------------------------------------------------------------
// TPayload -- Payload definition
struct TPayload
{
  sc_uint<32>        data;         // Bus for the data to be exchanged

  inline bool operator == (const TPayload& payload) const
  {
    return (payload.data==data);
  }
};

//---------------------------------------------------------------------------
// TRouteData -- data required to perform routing
struct TRouteData
{
    int current_id;
    int src_id;
    int dst_id;
    int dir_in; // direction from which the packet comes from
};

//---------------------------------------------------------------------------
struct TChannelStatus
{
    int free_slots;  // occupied buffer slots
    bool available; // 
    inline bool operator == (const TChannelStatus& bs) const
    {
	return (free_slots == bs.free_slots && available == bs.available);
    };
};

//---------------------------------------------------------------------------
// Packet type
enum TPacketType
{
  STARTING_SEGMENT_REQUEST,
  SEGMENT_REQUEST,
  SEGMENT_CONFIRM,
  SEGMENT_CANCEL
};

//---------------------------------------------------------------------------
// TPacket -- Packet definition
struct TPacket
{
  int                src_id;
  int                dst_id;
  TPacketType        type;    
  TPayload           payload;      // Optional payload
  double             timestamp;    // Unix timestamp at packet generation
  int                hop_no;       // Current number of hops from source to destination
  int 		     dir_in;       // The direction it came from
  int 	        dir_out; // direction to which the packet is forwarded
  inline bool operator == (const TPacket& packet) const
  {
    return (packet.src_id==src_id && packet.type==type && packet.payload==payload && packet.hop_no==hop_no);
  }

};


// output redefinitions *******************************************

//---------------------------------------------------------------------------
inline ostream& operator << (ostream& os, const TPacket& packet)
{
  if (TGlobalParams::verbose_mode==VERBOSE_HIGH)
  {

      os << "### PACKET ###" << endl;
      os << "Source Node[" << packet.src_id << "]" << endl;
      switch(packet.type)
      {
	case STARTING_SEGMENT_REQUEST: os << "Packet Type is STARTING_SEGMENT_REQUEST" << endl; break;
	case SEGMENT_REQUEST: os << "Packet Type is SEGMENT_REQUEST" << endl; break;
	case SEGMENT_CONFIRM: os << "Packet Type is SEGMENT_CONFIRM" << endl; break;
	case SEGMENT_CANCEL: os << "Packet Type is SEGMENT_CANCEL" << endl; break;
      }
      os << "Total number of hops:" << packet.hop_no << endl;
  }

  return os;
}
//---------------------------------------------------------------------------

inline ostream& operator << (ostream& os, const TChannelStatus& status)
{
  char msg;
  if (status.available) msg = 'A'; 
  else
      msg = 'N';
  os << msg << "(" << status.free_slots << ")"; 
  return os;
}
//---------------------------------------------------------------------------

inline ostream& operator << (ostream& os, const TCoord& coord)
{
  os << "(" << coord.x << "," << coord.y << ")";

  return os;
}


// trace redefinitions *******************************************
//
//---------------------------------------------------------------------------
inline void sc_trace(sc_trace_file*& tf, const TPacket& packet, string& name)
{
  sc_trace(tf, packet.src_id, name+".src_id");
  sc_trace(tf, packet.hop_no, name+".hop_no");
}

//---------------------------------------------------------------------------
inline void sc_trace(sc_trace_file*& tf, const TChannelStatus& bs, string& name)
{
  sc_trace(tf, bs.free_slots, name+".free_slots");
  sc_trace(tf, bs.available, name+".available");
}

// misc common functions **************************************
//---------------------------------------------------------------------------
inline TCoord id2Coord(int id) 
{
  TCoord coord;

  coord.x = id % TGlobalParams::mesh_dim_x;
  coord.y = id / TGlobalParams::mesh_dim_x;

  assert(coord.x < TGlobalParams::mesh_dim_x);
  assert(coord.y < TGlobalParams::mesh_dim_y);

  return coord;
}

//---------------------------------------------------------------------------
inline int coord2Id(const TCoord& coord) 
{
  int id = (coord.y * TGlobalParams::mesh_dim_x) + coord.x;

  assert(id < TGlobalParams::mesh_dim_x * TGlobalParams::mesh_dim_y);

  return id;
}


#endif  



