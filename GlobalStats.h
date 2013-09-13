/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the global statistics
 */

#ifndef __NOXIMGLOBALSTATS_H__
#define __NOXIMGLOBALSTATS_H__

#include <iostream>
#include <vector>
#include <iomanip>
#include <map>
#include "TNet.h"
#include "TNode.h"
using namespace std;

class GlobalStats {

  public:

    GlobalStats(const TNet * _net);


    // Returns the aggragated average delay (cycles)
    double getAverageDelay();

    // Returns the aggragated average delay (cycles) for communication src_id->dst_id
    double getAverageDelay(const int src_id, const int dst_id);

    // Returns the max delay
    double getMaxDelay();

    // Returns the max delay (cycles) experimented by destination
    // node_id. Returns -1 if node_id is not destination of any
    // communication
    double getMaxDelay(const int node_id);

    // Returns the max delay (cycles) for communication src_id->dst_id
    double getMaxDelay(const int src_id, const int dst_id);

    // Returns tha matrix of max delay for any node of the network
     vector < vector < double > > getMaxDelayMtx();

    // Returns the aggragated average throughput (flits/cycles)
    double getAverageThroughput();

    // Returns the aggragated average throughput (flits/cycles) for
    // communication src_id->dst_id
    double getAverageThroughput(const int src_id, const int dst_id);

    // Returns the total number of received packets
    unsigned int getReceivedPackets();

    // Returns the total number of received flits
    unsigned int getReceivedFlits();

    // Returns the maximum value of the accepted traffic
    double getThroughput();

    // Returns the number of routed flits for each router
     vector < vector < unsigned long > > getRoutedFlitsMtx();

    // Returns the total power
    double getPower();

    // Shows global statistics
    void writeStats();

    void drawGraphviz();

    void updateLatency(double last);


#ifdef TESTING
    unsigned int drained_total;
#endif

  private:
    // DiSR stats functions and structures ////////////////////
    void generate_disr_stats();
    /*  percentage of nodes covered/assigned by the DiSR */
    void compute_disr_node_coverage();
    /*  percentage of nodes covered/assigned by the DiSR */
    void compute_disr_link_coverage();

    // time required to complete the whole DiSR proces 
    void compute_disr_latency();

    struct 
    {
	int total_nodes;
	int total_links;
	int defective_nodes;
	int covered_nodes;
	int covered_links;
	double node_coverage;
	double link_coverage;
	double working_link_coverage;
	map<TSegmentId,vector<int> > segmentList;
	int nsegments;
	double average_seg_length;
	double latency;
    }
    DiSR_stats;

    string basefilename() const;


    const TNet *net;
};

#endif
