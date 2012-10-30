/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementaton of the global statistics
 */

#include "GlobalStats.h"
#include <cstdio>
using namespace std;

GlobalStats::GlobalStats(const TNet * _net)
{
    net = _net;

#ifdef TESTING
    drained_total = 0;
#endif
}

double GlobalStats::getAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int received_packets =
		net->t[x][y]->r->stats.getReceivedPackets();

	    if (received_packets) {
		avg_delay +=
		    received_packets *
		    net->t[x][y]->r->stats.getAverageDelay();
		total_packets += received_packets;
	    }
	}

    avg_delay /= (double) total_packets;

    return avg_delay;
}

double GlobalStats::getAverageDelay(const int src_id,
					 const int dst_id)
{
    TNode *node = net->searchNode(dst_id);

    assert(node != NULL);

    return node->r->stats.getAverageDelay(src_id);
}

double GlobalStats::getMaxDelay()
{
    double maxd = -1.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    TCoord coord;
	    coord.x = x;
	    coord.y = y;
	    int node_id = coord2Id(coord);
	    double d = getMaxDelay(node_id);
	    if (d > maxd)
		maxd = d;
	}

    return maxd;
}

double GlobalStats::getMaxDelay(const int node_id)
{
    TCoord coord = id2Coord(node_id);

    unsigned int received_packets =
	net->t[coord.x][coord.y]->r->stats.getReceivedPackets();

    if (received_packets)
	return net->t[coord.x][coord.y]->r->stats.getMaxDelay();
    else
	return -1.0;
}

double GlobalStats::getMaxDelay(const int src_id, const int dst_id)
{
    TNode *node = net->searchNode(dst_id);

    assert(node != NULL);

    return node->r->stats.getMaxDelay(src_id);
}

vector < vector < double > > GlobalStats::getMaxDelayMtx()
{
    vector < vector < double > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    TCoord coord;
	    coord.x = x;
	    coord.y = y;
	    int id = coord2Id(coord);
	    mtx[y][x] = getMaxDelay(id);
	}

    return mtx;
}

double GlobalStats::getAverageThroughput(const int src_id,
					      const int dst_id)
{
    TNode *node = net->searchNode(dst_id);

    assert(node != NULL);

    return node->r->stats.getAverageThroughput(src_id);
}

double GlobalStats::getAverageThroughput()
{
    unsigned int total_comms = 0;
    double avg_throughput = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int ncomms =
		net->t[x][y]->r->stats.getTotalCommunications();

	    if (ncomms) {
		avg_throughput +=
		    ncomms * net->t[x][y]->r->stats.getAverageThroughput();
		total_comms += ncomms;
	    }
	}

    avg_throughput /= (double) total_comms;

    return avg_throughput;
}

unsigned int GlobalStats::getReceivedPackets()
{
    unsigned int n = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    n += net->t[x][y]->r->stats.getReceivedPackets();

    return n;
}

unsigned int GlobalStats::getReceivedFlits()
{
    unsigned int n = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    n += net->t[x][y]->r->stats.getReceivedFlits();
#ifdef TESTING
	    drained_total += net->t[x][y]->r->local_drained;
#endif
	}

    return n;
}

double GlobalStats::getThroughput()
{
    assert(false);
    int total_cycles;
    /*
    int total_cycles =
	GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;

	*/
    //  int number_of_ip = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
    //  return (double)getReceivedFlits()/(double)(total_cycles * number_of_ip);

    unsigned int n = 0;
    unsigned int trf = 0;
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int rf = net->t[x][y]->r->stats.getReceivedFlits();

	    if (rf != 0)
		n++;

	    trf += rf;
	}
    return (double) trf / (double) (total_cycles * n);

}

vector < vector < unsigned long > > GlobalStats::getRoutedFlitsMtx()
{

    vector < vector < unsigned long > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    /*
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    mtx[y][x] = net->t[x][y]->r->getRoutedFlits();
	    */

    return mtx;
}

double GlobalStats::getPower()
{
    double power = 0.0;

    /*
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    power += net->t[x][y]->r->getPower();
	    */

    return power;
}

void GlobalStats::showStats(std::ostream & out )
{
    FILE * fp;

    /*
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    net->t[x][y]->r->stats.showStats(y * GlobalParams:: mesh_dim_x + x, out, true);
	    */

    char fn[40];
    sprintf(fn,"graph_%d.gv",GlobalParams::disr_bootstrap_node);

    if ( (fp = fopen(fn,"w"))!= NULL)
    {
	// draw the network layout and declare nodes
	fprintf(fp,"\n digraph G { graph [layout=dot] ");

	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	{
	    fprintf(fp,"\n {rank=same; ");
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    {
		TSegmentId tid = net->t[x][y]->r->disr.getLocalSegmentID();
		if (tid.isAssigned())
		    fprintf(fp,"N%d [shape=circle, fixedsize=true]; ",net->t[x][y]->r->local_id);
		else
		    fprintf(fp,"N%d [shape=square, fixedsize=true]; ",net->t[x][y]->r->local_id);
	    }
	    fprintf(fp," }");
	}

	// draw horizontal edges...
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	{
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    {
		int curr_id = net->t[x][y]->r->local_id;

		if (x != GlobalParams::mesh_dim_x-1)
		{
		    TSegmentId tid = net->t[x][y]->r->disr.getLinkSegmentID(DIRECTION_EAST);
		    if (tid.isAssigned())
			fprintf(fp,"\nN%d->N%d [dir=both, label=\"%d.%d\"]",curr_id,curr_id+1,tid.getNode(),tid.getLink());
		    else
			fprintf(fp,"\nN%d->N%d [dir=none, label=\".\"]",curr_id,curr_id+1);

		}
	    }
	}

	// draw vertical edges...
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	{
	    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    {
		int curr_id = net->t[x][y]->r->local_id;
		int south_id = net->t[x][y]->r->getNeighborId(curr_id,DIRECTION_SOUTH);

		if (y != GlobalParams::mesh_dim_y-1)
		{
		    TSegmentId tid = net->t[x][y]->r->disr.getLinkSegmentID(DIRECTION_SOUTH);
		    if (tid.isAssigned())
			fprintf(fp,"\nN%d->N%d [dir=both, label=\"%d.%d\"]",curr_id,south_id,tid.getNode(),tid.getLink());
		    else
			fprintf(fp,"\nN%d->N%d [dir=none, label=\".\"]",curr_id,south_id);

		}
	    }
	}

	fprintf(fp,"\n }");
	fclose(fp);
	sprintf(fn,"dot -Tpng -o graph_%d.png graph_%d.gv",GlobalParams::disr_bootstrap_node,GlobalParams::disr_bootstrap_node);
        system(fn);
    }
    else
    {
	cout << "\n Cannot write output graphwiz file...";
    }


    /*
    digraph G {
	graph [layout=dot]

	{rank=same;
	A;
	B;
	}
	
	{rank=same;
	C;
	D;
	}

	A -> B [dir=back, label="3.2"] 
	A -> C [dir=both, label="2.1"]
	C -> D [dir=forward, label="5.4"]
	B -> D [dir=none, label="3.2"]

}
*/
    /*
    out << "% Total received packets: " << getReceivedPackets() << endl;
    out << "% Total received flits: " << getReceivedFlits() << endl;
    out << "% Global average delay (cycles): " << getAverageDelay() <<
	endl;
    out << "% Global average throughput (flits/cycle): " <<
	getAverageThroughput() << endl;
    //out << "% Throughput (flits/cycle/IP): " << getThroughput() << endl;
    out << "% Max delay (cycles): " << getMaxDelay() << endl;
    out << "% Total energy (J): " << getPower() << endl;

    if (detailed) {
	out << endl << "detailed = [" << endl;
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
		net->t[x][y]->r->stats.showStats(y *
						 GlobalParams::
						 mesh_dim_x + x, out,
						 true);
	out << "];" << endl;

	// show MaxDelay matrix
	vector < vector < double > > md_mtx = getMaxDelayMtx();

	out << endl << "max_delay = [" << endl;
	for (unsigned int y = 0; y < md_mtx.size(); y++) {
	    out << "   ";
	    for (unsigned int x = 0; x < md_mtx[y].size(); x++)
		out << setw(6) << md_mtx[y][x];
	    out << endl;
	}
	out << "];" << endl;

	// show RoutedFlits matrix
	vector < vector < unsigned long > > rf_mtx = getRoutedFlitsMtx();

	out << endl << "routed_flits = [" << endl;
	for (unsigned int y = 0; y < rf_mtx.size(); y++) {
	    out << "   ";
	    for (unsigned int x = 0; x < rf_mtx[y].size(); x++)
		out << setw(10) << rf_mtx[y][x];
	    out << endl;
	}
	out << "];" << endl;
    }
    */
}
