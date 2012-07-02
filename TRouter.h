/*****************************************************************************

  TRouter.h -- Router definition

 *****************************************************************************/
#ifndef __TROUTER_H__
#define __TROUTER_H__

//---------------------------------------------------------------------------

#include <systemc.h>
#include "nanoxim.h"
#include "TBuffer.h"
#include "TReservationTable.h"


SC_MODULE(TRouter)
{

  // I/O Ports

  sc_in_clk          clock;        // The input clock for the router
  sc_in<bool>        reset;        // The reset signal for the router

  sc_in<TPacket>       packet_rx[DIRECTIONS+1];   // The input channels (including local one)
  sc_in<bool>        req_rx[DIRECTIONS+1];    // The requests associated with the input channels
  sc_out<bool>       ack_rx[DIRECTIONS+1];    // The outgoing ack signals associated with the input channels

  sc_out<TPacket>      packet_tx[DIRECTIONS+1];   // The output channels (including local one)
  sc_out<bool>       req_tx[DIRECTIONS+1];    // The requests associated with the output channels
  sc_in<bool>        ack_tx[DIRECTIONS+1];    // The outgoing ack signals associated with the output channels

  // Registers

  /*
  TCoord             position;                        // Router position inside the mesh
  */
  int                local_id;                              // Unique ID
  int                routing_type;                    // Type of routing algorithm
  TBuffer            buffer[DIRECTIONS+1];            // Buffer for each input channel 
  bool               current_level_rx[DIRECTIONS+1];  // Current level for Alternating Bit Protocol (ABP)
  bool               current_level_tx[DIRECTIONS+1];  // Current level for Alternating Bit Protocol (ABP)
  TReservationTable  reservation_table;               // Switch reservation table
  DiSR disr;						// DiSR component implementing algorithm locally
  int                start_from_port;                 // Port from which to start the reservation cycle
  // Functions

  void               rxProcess();        // The receiving process
  void               txProcess();        // The transmitting process
  void               configure(const int _id, const unsigned int _max_buffer_size);
  void inject_to_network(const TPacket& p);
  void flush_buffer(int);

  // Constructor

  SC_CTOR(TRouter)
  {
    SC_METHOD(rxProcess);
    sensitive << reset;
    sensitive << clock.pos();

    SC_METHOD(txProcess);
    sensitive << reset;
    sensitive << clock.pos();
  }


 private:
  // performs actual routing + selection
  int process(TPacket& p);
  vector<int> routingFunction(const TPacket& p);



  // routing functions
  vector<int> routingXY(const TCoord& current, const TCoord& destination);
  int reflexDirection(int direction) const;
  int getNeighborId(int _id,int direction) const;



};

#endif
