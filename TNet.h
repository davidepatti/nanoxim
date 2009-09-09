/*****************************************************************************

  TNet.h -- Nanonetwork definition

 *****************************************************************************/
#ifndef __TNET_H__
#define __TNET_H__

//---------------------------------------------------------------------------

#include <systemc.h>
#include "TNode.h"

SC_MODULE(TNet)
{

  // I/O Ports

  sc_in_clk        clock;        // The input clock for the Net
  sc_in<bool>      reset;        // The reset signal for the Net

  // Signals

  sc_signal<bool>    req_to_east[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    req_to_west[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    req_to_south[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    req_to_north[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];

  sc_signal<bool>    ack_to_east[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    ack_to_west[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    ack_to_south[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<bool>    ack_to_north[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];

  sc_signal<TPacket>   packet_to_east[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<TPacket>   packet_to_west[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<TPacket>   packet_to_south[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];
  sc_signal<TPacket>   packet_to_north[MAX_STATIC_DIM+1][MAX_STATIC_DIM+1];

  // Matrix of tiles

  TNode*             t[MAX_STATIC_DIM][MAX_STATIC_DIM];

  // Constructor

  SC_CTOR(TNet)
  {

    // Build the Mesh
    buildMesh();
  }

  // Support methods
  TNode* searchNode(const int id) const;


 private:
  void buildMesh();
};

//---------------------------------------------------------------------------

#endif

