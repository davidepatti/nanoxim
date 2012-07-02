#ifndef __NANOXIM_DEFS_H__
#define __NANOXIM_DEFS_H__

//---------------------------------------------------------------------------

#include <cassert>
#include <systemc.h>
#include <vector>

using namespace std;


#define DIRECTIONS             4

// Define the directions as numbers
#define DIRECTION_NORTH        0
#define DIRECTION_EAST         1
#define DIRECTION_SOUTH        2
#define DIRECTION_WEST         3
#define DIRECTION_LOCAL        4

// ACTIONS
// flood all available out directions, ignore packet, confirm requests
#define ACTION_FLOOD    100
#define ACTION_DISCARD	101
#define ACTION_SKIP	102
#define ACTION_FORWARD_REQUEST 103 // check if necessary
#define ACTION_END_CONFIRM 104
#define ACTION_CONFIRM 105
#define ACTION_CANCEL_REQUEST 106
#define ACTION_END_CANCEL 107

// Generic not reserved resource
#define NOT_RESERVED          -2

// To mark invalid or non exhistent values
#define NOT_VALID             -1

// Routing algorithms
#define ROUTING_XY             0

// type of link to be set
#define VISITED 1
#define TVISITED 2
#define ALL 3

#define MAX_TIMEOUT 100



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

enum DiSR_status { BOOTSTRAP, 
                   READY_SEARCHING, 
		   ACTIVE_SEARCHING, 
		   CANDIDATE, 
		   CANDIDATE_STARTING, 
		   ASSIGNED, 
		   FREE 
};

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
  STARTING_SEGMENT_CONFIRM,
  SEGMENT_REQUEST,
  SEGMENT_CONFIRM,
  SEGMENT_CANCEL
};


//---------------------------------------------------------------------------
// Distribuited SR data


class TSegmentId
{
private:
	int node;
	int link;

public:
	TSegmentId();

	inline bool operator == (const TSegmentId& segid) const
	{
	    return ( (segid.node==node) && (segid.link==link));
	}
	inline bool operator < (const TSegmentId& segid) const
	{
	    return ( node<segid.node );
	}

	inline bool isFree() const
	{
	    return ( (node == NOT_RESERVED) && (link == NOT_RESERVED));

	}
	inline bool isValid() const
	{
	    return ( (node != NOT_VALID) && (link != NOT_VALID));

	}

	inline void invalidate()
	{
	    this->node = NOT_VALID;
	    this->link = NOT_VALID;
	}

	inline void set(int node, int link)
	{
	    this->node = node;
	    this->link = link;

	}

	int getNode() { return this->node; };
	int getLink() { return this->link; };

};


class TPacket;
class TRouter;


class DiSR
{

  // Distribuited SR related functions and data
    public:
  void reset();
  void update_status();
  int process(TPacket& p);
  void set_router(TRouter *);
  void invalidate(int);
  DiSR_status getStatus() const;
  void setLinks(int type, const vector<int>& directions,const TSegmentId& id);


    private:
  int next_free_link();
  int has_free_link() const;
  bool sanity_check();
  void print_status() const;
  void bootstrap_node();
  void setStatus(const DiSR_status&);
  void generate_segment_confirm(TPacket&);
  void generate_segment_cancel(TPacket&);
  void start_investigate_links();
  void continue_investigate_links();

  void set_request_path(int);

  int request_path; // the direction from which a request came

  // Local environment data (LED)

  TSegmentId segID;
  bool visited;
  bool tvisited;
    // intented as bidirectional!
  TSegmentId link_visited[4];
  TSegmentId link_tvisited[4];

  bool starting;
  bool terminal;
  int subnet;
  int current_link;
  TRouter * router;

  int timeout;

  DiSR_status status;
  
};
//---------------------------------------------------------------------------
// TPacket -- Packet definition
struct TPacket
{
  TSegmentId	     id;   // required for DiSR 
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
    return (packet.id==id && packet.src_id==src_id && packet.type==type && packet.payload==payload && packet.hop_no==hop_no);
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
	case STARTING_SEGMENT_CONFIRM: os << "Packet Type is STARTING_SEGMENT_CONFIRM" << endl; break;
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

inline ostream& operator << (ostream& os, TSegmentId& segid)
{
    if (segid.isFree())
	os << "(.)";
    else if (!segid.isValid())
	os << "(!)";
    else
      os << "(" << segid.getNode() << "." << segid.getLink() << ")";

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



