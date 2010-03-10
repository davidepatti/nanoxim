
//---------------------------------------------------------------------------

#include "nanoxim.h"

//---------------------------------------------------------------------------

void showHelp(char selfname[])
{
  cout << "Usage: " << selfname << " [options]\nwhere [options] is one or more of the following ones:" << endl;
  cout << "\t-help\t\tShow this help and exit" << endl;
  cout << "\t-verbose N\tVerbosity level (1=low, 2=medium, 3=high, default off)" << endl;
  cout << "\t-dimx N\t\tSet the mesh X dimension to the specified integer value (default " << DEFAULT_MESH_DIM_X << ")" << endl;
  cout << "\t-dimy N\t\tSet the mesh Y dimension to the specified integer value (default " << DEFAULT_MESH_DIM_Y << ")" << endl;
  cout << "\t-routing TYPE\tSet the routing algorithm to TYPE where TYPE is one of the following (default " << ROUTING_XY << "):" << endl;
  cout << "\t-sim N\t\tRun for the specified simulation time [cycles] (default " << DEFAULT_SIMULATION_TIME << ")" << endl << endl;
  cout << "\t-disr - Run setup for distribuited Segment-base Routing" << endl;
}

//---------------------------------------------------------------------------

void showConfig()
{
  cout << "Using the following configuration: " << endl;
  cout << "- mesh_dim_x = " << TGlobalParams::mesh_dim_x << endl;
  cout << "- mesh_dim_y = " << TGlobalParams::mesh_dim_y << endl;
  cout << "- buffer_depth = " << TGlobalParams::buffer_depth << endl;
  cout << "- routing_algorithm = " << TGlobalParams::routing_algorithm << endl;
  cout << "- simulation_time = " << TGlobalParams::simulation_time << endl;
}

//---------------------------------------------------------------------------

void checkInputParameters()
{
  if (TGlobalParams::mesh_dim_x <= 1) {
    cerr << "Error: dimx must be greater than 1" << endl;
    exit(1);
  }

  if (TGlobalParams::mesh_dim_y <= 1) {
    cerr << "Error: dimy must be greater than 1" << endl;
    exit(1);
  }

  if (TGlobalParams::buffer_depth < 1)
  {
    cerr << "Error: buffer must be >= 1" << endl;
    exit(1);
  }

}

//---------------------------------------------------------------------------

void parseCmdLine(int arg_num, char *arg_vet[])
{
  if (arg_num == 1)
    cout << "Running with default parameters (use '-help' option to see how to override them)" << endl;
  else
  {
    for (int i=1; i<arg_num; i++)
    {
      if (!strcmp(arg_vet[i], "-help")) 
      {
	showHelp(arg_vet[0]);
	exit(0);
      } 
      else if (!strcmp(arg_vet[i], "-verbose"))
	TGlobalParams::verbose_mode = atoi(arg_vet[++i]);
      else if (!strcmp(arg_vet[i], "-dimx"))
	TGlobalParams::mesh_dim_x = atoi(arg_vet[++i]);
      else if (!strcmp(arg_vet[i], "-dimy"))
	TGlobalParams::mesh_dim_y = atoi(arg_vet[++i]);
      else if (!strcmp(arg_vet[i], "-buffer"))
	TGlobalParams::buffer_depth = atoi(arg_vet[++i]);
      else if (!strcmp(arg_vet[i], "-routing"))
      {
	char *routing = arg_vet[++i];
        if (!strcmp(routing, "xy")) 
	  TGlobalParams::routing_algorithm = ROUTING_XY;
      }
      else if (!strcmp(arg_vet[i], "-sim"))
	TGlobalParams::simulation_time = atoi(arg_vet[++i]);
      else if (!strcmp(arg_vet[i], "-disr")) 
	  TGlobalParams::disr = 1;
      else 
      {
	cerr << "Error: Invalid option: " << arg_vet[i] << endl;
	exit(1);
      }
    }
  }

  checkInputParameters();

}

//---------------------------------------------------------------------------

