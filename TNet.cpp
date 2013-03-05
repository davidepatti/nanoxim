/*****************************************************************************

  TNet.cpp -- nanonetwork implementation

 *****************************************************************************/
#include "TNet.h"
#include "Stats.h"

//---------------------------------------------------------------------------

void TNet::buildMesh()
{

  // Create the mesh as a matrix of nodes
  for(int i=0; i<GlobalParams::mesh_dim_x; i++)
    {
      for(int j=0; j<GlobalParams::mesh_dim_y; j++)
	{
	  // Create the single Node with a proper name
	  char node_name[20];
	  sprintf(node_name, "Node[%02d][%02d]", i, j);
	  t[i][j] = new TNode(node_name);

	  // Tell to the router its coordinates
	  t[i][j]->r->configure(j * GlobalParams::mesh_dim_x + i, GlobalParams::buffer_depth);

	  // Tell to the PE its coordinates
	  t[i][j]->pe->local_id = j * GlobalParams::mesh_dim_x + i;

	  // Map clock and reset
	  t[i][j]->clock(clock);
	  t[i][j]->reset(reset);

	  // Map Rx signals
	  t[i][j]->req_rx[DIRECTION_NORTH](req_to_south[i][j]);
	  t[i][j]->packet_rx[DIRECTION_NORTH](packet_to_south[i][j]);
	  t[i][j]->ack_rx[DIRECTION_NORTH](ack_to_north[i][j]);

	  t[i][j]->req_rx[DIRECTION_EAST](req_to_west[i+1][j]);
	  t[i][j]->packet_rx[DIRECTION_EAST](packet_to_west[i+1][j]);
	  t[i][j]->ack_rx[DIRECTION_EAST](ack_to_east[i+1][j]);

	  t[i][j]->req_rx[DIRECTION_SOUTH](req_to_north[i][j+1]);
	  t[i][j]->packet_rx[DIRECTION_SOUTH](packet_to_north[i][j+1]);
	  t[i][j]->ack_rx[DIRECTION_SOUTH](ack_to_south[i][j+1]);

	  t[i][j]->req_rx[DIRECTION_WEST](req_to_east[i][j]);
	  t[i][j]->packet_rx[DIRECTION_WEST](packet_to_east[i][j]);
	  t[i][j]->ack_rx[DIRECTION_WEST](ack_to_west[i][j]);

	  // Map Tx signals
	  t[i][j]->req_tx[DIRECTION_NORTH](req_to_north[i][j]);
	  t[i][j]->packet_tx[DIRECTION_NORTH](packet_to_north[i][j]);
	  t[i][j]->ack_tx[DIRECTION_NORTH](ack_to_south[i][j]);

	  t[i][j]->req_tx[DIRECTION_EAST](req_to_east[i+1][j]);
	  t[i][j]->packet_tx[DIRECTION_EAST](packet_to_east[i+1][j]);
	  t[i][j]->ack_tx[DIRECTION_EAST](ack_to_west[i+1][j]);

	  t[i][j]->req_tx[DIRECTION_SOUTH](req_to_south[i][j+1]);
	  t[i][j]->packet_tx[DIRECTION_SOUTH](packet_to_south[i][j+1]);
	  t[i][j]->ack_tx[DIRECTION_SOUTH](ack_to_north[i][j+1]);

	  t[i][j]->req_tx[DIRECTION_WEST](req_to_west[i][j]);
	  t[i][j]->packet_tx[DIRECTION_WEST](packet_to_west[i][j]);
	  t[i][j]->ack_tx[DIRECTION_WEST](ack_to_east[i][j]);
	}
    }

  // Clear signals for borderline nodes
  for(int i=0; i<=GlobalParams::mesh_dim_x; i++)
    {
      req_to_south[i][0] = 0;
      ack_to_north[i][0] = 0;
      req_to_north[i][GlobalParams::mesh_dim_y] = 0;
      ack_to_south[i][GlobalParams::mesh_dim_y] = 0;

    }

  for(int j=0; j<=GlobalParams::mesh_dim_y; j++)
    {
      req_to_east[0][j] = 0;
      ack_to_west[0][j] = 0;
      req_to_west[GlobalParams::mesh_dim_x][j] = 0;
      ack_to_east[GlobalParams::mesh_dim_x][j] = 0;

    }

  // invalidate reservation table and disr entries for non-exhistent channels
  for(int i=0; i<GlobalParams::mesh_dim_x; i++)
    {
      t[i][0]->r->reservation_table.invalidate(DIRECTION_NORTH);
      t[i][GlobalParams::mesh_dim_y-1]->r->reservation_table.invalidate(DIRECTION_SOUTH);

      // disr
      t[i][0]->r->disr.invalidate_direction(DIRECTION_NORTH);
      t[i][GlobalParams::mesh_dim_y-1]->r->disr.invalidate_direction(DIRECTION_SOUTH);
    }
  for(int j=0; j<GlobalParams::mesh_dim_y; j++)
    {
      t[0][j]->r->reservation_table.invalidate(DIRECTION_WEST);
      t[GlobalParams::mesh_dim_x-1][j]->r->reservation_table.invalidate(DIRECTION_EAST);

      // disr
      t[0][j]->r->disr.invalidate_direction(DIRECTION_WEST);
      t[GlobalParams::mesh_dim_x-1][j]->r->disr.invalidate_direction(DIRECTION_EAST);
    }

  // invalidate reservation table and disr entries for defective channels
  
  if (GlobalParams::defective)
  {
      bool do_defect;

	for (int i=0; i<GlobalParams::mesh_dim_y; i++)
	{
	    for (int j=0; j<GlobalParams::mesh_dim_x-1; j++)
	    {
		cout << "Analyzing horizonal links, node " << (t[j][i]->r->local_id) << endl;
		do_defect = (((double) rand()) / RAND_MAX < GlobalParams::defective);

		if (do_defect)
		{
		    cout << "found link defect " << endl;
		    t[j][i]->r->disr.invalidate_direction(DIRECTION_EAST);
		    t[j+1][i]->r->disr.invalidate_direction(DIRECTION_WEST);
		    
		    t[j][i]->r->reservation_table.invalidate(DIRECTION_EAST);
		    t[j+1][i]->r->reservation_table.invalidate(DIRECTION_WEST);
		}
	    }
	}
      for (int i=0; i<GlobalParams::mesh_dim_x; i++)
      {
	for (int j=0; j<GlobalParams::mesh_dim_y-1; j++)
	{
		cout << "Analyzing vertical links, node " << (t[i][j]->r->local_id) << endl;
		do_defect = (((double) rand()) / RAND_MAX < GlobalParams::defective);

		if (do_defect)
		{
		    cout << "found link defect " << endl;
		    t[i][j]->r->disr.invalidate_direction(DIRECTION_SOUTH);
		    t[i][j+1]->r->disr.invalidate_direction(DIRECTION_NORTH);
		    
		    t[i][j]->r->reservation_table.invalidate(DIRECTION_SOUTH);
		    t[i][j+1]->r->reservation_table.invalidate(DIRECTION_NORTH);
		}
	}
      }
  }

}

//---------------------------------------------------------------------------

TNode* TNet::searchNode(const int id) const
{
  for (int i=0; i<GlobalParams::mesh_dim_x; i++)
    for (int j=0; j<GlobalParams::mesh_dim_y; j++)
      if (t[i][j]->r->local_id == id)
	return t[i][j];

  return false;
}

//---------------------------------------------------------------------------

