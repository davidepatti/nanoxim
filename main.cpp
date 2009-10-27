/*****************************************************************************

  main.cpp -- The testbench

 *****************************************************************************/
/* Copyright 2009
    Davide Patti <dpatti@diit.unict.it>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <systemc.h>
#include "nanoxim.h"
#include "TNet.h"
#include "CmdLineParser.h"


using namespace std;

//---------------------------------------------------------------------------

// Initialize global configuration parameters (can be overridden with command-line arguments)
int   TGlobalParams::mesh_dim_x                       = DEFAULT_MESH_DIM_X;
int   TGlobalParams::mesh_dim_y                       = DEFAULT_MESH_DIM_Y;
int   TGlobalParams::buffer_depth                     = DEFAULT_BUFFER_DEPTH;
int   TGlobalParams::routing_algorithm                = ROUTING_XY;
int   TGlobalParams::verbose_mode		      = DEFAULT_VERBOSE_MODE;
int   TGlobalParams::simulation_time		      = DEFAULT_SIMULATION_TIME;
int   TGlobalParams::rnd_generator_seed               = 0;
int   TGlobalParams::disr               = DEFAULT_DISR_SETUP;

//---------------------------------------------------------------------------

int sc_main(int arg_num, char* arg_vet[])
{

  // Handle command-line arguments
  cout << endl << "\t\tNanoxim - nanonetwork simulator" << endl;
  cout << "\t\t(C) University of Catania" << endl << endl;

  parseCmdLine(arg_num, arg_vet);

  // Signals
  // TODO: check for nanorealistic frequencies
  sc_clock        clock("clock", 1, SC_NS);
  sc_signal<bool> reset;

  // network instance
  TNet* n = new TNet("Net");
  n->clock(clock);
  n->reset(reset);

  // Reset the chip and run the simulation
  reset.write(1);
  cout << "Reset...";
  srand(TGlobalParams::rnd_generator_seed); // time(NULL));
  sc_start(DEFAULT_RESET_TIME, SC_NS);
  reset.write(0);
  cout << " done! Now running for " << TGlobalParams::simulation_time << " cycles..." << endl;
  sc_start(TGlobalParams::simulation_time, SC_NS);

  // Close the simulation
  cout << "network simulation completed." << endl;
  cout << " ( " << sc_time_stamp().to_double()/1000 << " cycles executed)" << endl;


  return 0;

}

