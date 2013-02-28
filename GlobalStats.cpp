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


// Note: different style for DiSR stats functions:
// - private methods that update local struct collecting all data


void GlobalStats::generate_disr_stats()
{
    get_disr_defective_nodes();

    compute_disr_node_coverage();
    compute_disr_link_coverage();
    /*
    compute_disr_average_path_length();
    compute_disr_average_link_weight();
    compute_disr_unidirectional_turn_restrictions();
    */
}

void GlobalStats::get_disr_defective_nodes()
{
    // the input parameter is a probability of failure, this gets the actual
    // number of defective nodes
    int defective = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
    {
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    if (net->t[x][y]->r->disr.isAssigned());
    }
    this->DiSR_stats.defective_nodes = defective;
}

/* get the percentage of nodes covered/assigned by the DiSR 
 *
 * */
void GlobalStats::compute_disr_node_coverage()
{
    int covered = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
    {
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    if (net->t[x][y]->r->disr.isAssigned())
	    {
		covered++;
		TSegmentId seg_id = net->t[x][y]->r->disr.getLocalSegmentID();
		int node_id = net->t[x][y]->r->local_id;
		this->DiSR_stats.segmentList[seg_id].push_back(node_id);
		cout << "Adding node " << node_id << " to segment " << seg_id << endl;
	    }
    }

    this->DiSR_stats.total_nodes = GlobalParams::mesh_dim_y * GlobalParams::mesh_dim_x;
    this->DiSR_stats.covered_nodes = covered;
    this->DiSR_stats.node_coverage = (double)covered/this->DiSR_stats.total_nodes;
    this->DiSR_stats.nsegments = this->DiSR_stats.segmentList.size();
    this->DiSR_stats.average_seg_length = covered/(double)(this->DiSR_stats.nsegments);



}

/* get the percentage of links covered/assigned by the DiSR 
 *
 * */
void GlobalStats::compute_disr_link_coverage()
{
    int covered = 0;
    int total_links = 0;

    // horizontal edges...
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
    {
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	{
	    if (x != GlobalParams::mesh_dim_x-1)
	    {
		total_links++;

		TSegmentId tid = net->t[x][y]->r->disr.getLinkSegmentID(DIRECTION_EAST);
		if (tid.isAssigned())
		    covered++;

	    }
	}
    }

    // vertical edges...
    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
    {
	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	{
	    if (y != GlobalParams::mesh_dim_y-1)
	    {
		total_links++;
		TSegmentId tid = net->t[x][y]->r->disr.getLinkSegmentID(DIRECTION_SOUTH);
		if (tid.isAssigned())
		    covered++;
	    }
	}
    }

    this->DiSR_stats.total_links = total_links;
    this->DiSR_stats.covered_links = covered;
    this->DiSR_stats.link_coverage = (double)covered/total_links;
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

void GlobalStats::drawGraphviz()
{
    FILE * fp;

    char fn[50];
    sprintf(fn,"graph_%dx%d_b%d",GlobalParams::mesh_dim_x,GlobalParams::mesh_dim_y,GlobalParams::bootstrap);
    if (GlobalParams::cyclelinks)
	strcat(fn,"_cyclelinks");

    if ( (fp = fopen(strcat(fn,".gv"),"w"))!= NULL)
    {
	// draw the network layout and declare nodes
	fprintf(fp,"\n digraph G { graph [layout=dot] ");

	for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	{
	    fprintf(fp,"\n {rank=same; ");
	    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	    {
		TSegmentId tid = net->t[x][y]->r->disr.getLocalSegmentID();
		int local_id = net->t[x][y]->r->local_id;

		if (net->t[x][y]->r->disr.isAssigned())
		{
		    if (local_id == GlobalParams::bootstrap)
			fprintf(fp,"N%d [shape=circle, style=filled, fixedsize=true]; ",local_id);
		    else
			fprintf(fp,"N%d [shape=circle, fixedsize=true]; ",local_id);

		}
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
			fprintf(fp,"\nN%d->N%d [dir=none, color=red, style=bold, label=\"%d.%d\"]",curr_id,curr_id+1,tid.getNode(),tid.getLink());
		    else
			fprintf(fp,"\nN%d->N%d [dir=none, style=dotted, label=\".\"]",curr_id,curr_id+1);

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
			fprintf(fp,"\nN%d->N%d [dir=none, color=red, style=bold, label=\"%d.%d\"]",curr_id,south_id,tid.getNode(),tid.getLink());
		    else
			fprintf(fp,"\nN%d->N%d [dir=none, style=dotted, label=\".\"]",curr_id,south_id);


		}
	    }
	}

	fprintf(fp,"\n }");
	fclose(fp);
	char cmd[100];
	sprintf(cmd,"dot -Tpng -o %s.png %s",fn,fn);
        system(cmd);
    }
    else
    {
	cout << "\n Cannot write output graphwiz file...";
    }
}

void GlobalStats::showStats(std::ostream & out )
{

    generate_disr_stats();
    out << " DiSR analytical results " << endl;
    out << "--------------------------------------------------- " << endl;
    out << "total nodes: " << DiSR_stats.total_nodes << endl;
    out << "total links: " << DiSR_stats.total_links << endl; 
    out << "defective nodes: " << DiSR_stats.defective_nodes << endl;
    out << "nodes covered: " << DiSR_stats.covered_nodes << endl;
    out << "links covered: " << DiSR_stats.covered_links << endl;
    out << "% node coverage: " << DiSR_stats.node_coverage << endl;
    out << "% link coverage: " << DiSR_stats.link_coverage << endl;
    out << "number of segments: " << DiSR_stats.nsegments << endl;
    out << "average segment length: " << DiSR_stats.average_seg_length<< endl;

    map<TSegmentId, vector<int> >::const_iterator it;

    for (it = DiSR_stats.segmentList.begin(); it!=DiSR_stats.segmentList.end(); ++it)
    {
	TSegmentId tmpid = it->first;
	cout <<  "\n Segment " << tmpid << ": ";
	for (unsigned int i = 0; i< DiSR_stats.segmentList[tmpid].size(); i++)
	    cout << DiSR_stats.segmentList[tmpid][i] << " , ";

    }
}
