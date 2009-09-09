/*****************************************************************************

  TNode.h -- Node definition

 *****************************************************************************/
#ifndef __TNODE_H__
#define __TNODE_H__

//---------------------------------------------------------------------------

#include <systemc.h>
#include "TRouter.h"
#include "TProcessingElement.h"

SC_MODULE(TNode)
{

  // I/O Ports

  sc_in_clk           clock;        // The input clock for the node
  sc_in<bool>         reset;        // The reset signal for the node

  sc_in<TPacket>        packet_rx[DIRECTIONS];   // The input channels
  sc_in<bool>         req_rx[DIRECTIONS];    // The requests associated with the input channels
  sc_out<bool>        ack_rx[DIRECTIONS];    // The outgoing ack signals associated with the input channels

  sc_out<TPacket>       packet_tx[DIRECTIONS];   // The output channels
  sc_out<bool>        req_tx[DIRECTIONS];    // The requests associated with the output channels
  sc_in<bool>         ack_tx[DIRECTIONS];    // The outgoing ack signals associated with the output channels

  // Signals

  sc_signal<TPacket>    packet_rx_local;   // The input channels
  sc_signal<bool>     req_rx_local;    // The requests associated with the input channels
  sc_signal<bool>     ack_rx_local;    // The outgoing ack signals associated with the input channels

  sc_signal<TPacket>    packet_tx_local;   // The output channels
  sc_signal<bool>     req_tx_local;    // The requests associated with the output channels
  sc_signal<bool>     ack_tx_local;    // The outgoing ack signals associated with the output channels
  
  // Instances
  TRouter*            r;               // Router instance
  TProcessingElement* pe;              // Processing Element instance

  // Constructor

  SC_CTOR(TNode)
  {

    // Router pin assignments
    r = new TRouter("Router");
    r->clock(clock);
    r->reset(reset);
    for(int i=0; i<DIRECTIONS; i++)
    {
      r->packet_rx[i](packet_rx[i]);
      r->req_rx[i](req_rx[i]);
      r->ack_rx[i](ack_rx[i]);

      r->packet_tx[i](packet_tx[i]);
      r->req_tx[i](req_tx[i]);
      r->ack_tx[i](ack_tx[i]);

    }

    r->packet_rx[DIRECTION_LOCAL](packet_tx_local);
    r->req_rx[DIRECTION_LOCAL](req_tx_local);
    r->ack_rx[DIRECTION_LOCAL](ack_tx_local);

    r->packet_tx[DIRECTION_LOCAL](packet_rx_local);
    r->req_tx[DIRECTION_LOCAL](req_rx_local);
    r->ack_tx[DIRECTION_LOCAL](ack_rx_local);

    // Processing Element pin assignments
    pe = new TProcessingElement("ProcessingElement");
    pe->clock(clock);
    pe->reset(reset);

    pe->packet_rx(packet_rx_local);
    pe->req_rx(req_rx_local);
    pe->ack_rx(ack_rx_local);

    pe->packet_tx(packet_tx_local);
    pe->req_tx(req_tx_local);
    pe->ack_tx(ack_tx_local);

  }

};

#endif
