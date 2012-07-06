
#ifndef __STATS_H__
#define __STATS_H__

#include <iostream>
#include <iomanip>
#include <vector>
#include "nanoxim.h"
using namespace std;

struct CommHistory {
    int src_id;
     vector < double >delays;
    unsigned int total_received_flits;
    double last_received_flit_time;
};

class Stats {

  public:

    Stats() {
    } void configure(const int node_id, const double _warm_up_time);

    // Access point for stats update
    void receivedFlit(const double arrival_time, const TPacket & packet);

    // Returns the average delay (cycles) for the current node as
    // regards to the communication whose source is src_id
    double getAverageDelay(const int src_id);

    // Returns the average delay (cycles) for the current node
    double getAverageDelay();

    // Returns the max delay for the current node as regards the
    // communication whose source node is src_id
    double getMaxDelay(const int src_id);

    // Returns the max delay (cycles) for the current node
    double getMaxDelay();

    // Returns the average throughput (flits/cycle) for the current node
    // and for the communication whose source is src_id
    double getAverageThroughput(const int src_id);

    // Returns the average throughput (flits/cycle) for the current node
    double getAverageThroughput();

    // Returns the number of received packets from current node
    unsigned int getReceivedPackets();

    // Returns the number of received flits from current node
    unsigned int getReceivedFlits();

    // Returns the number of communications whose destination is the
    // current node
    unsigned int getTotalCommunications();

    // Returns the energy consumed for communication src_id-->dst_id
    // under the following assumptions: (i) Minimal routing is
    // considered, (ii) constant packet size is considered (as the
    // average between the minimum and the maximum packet size).
    double getCommunicationEnergy(int src_id, int dst_id);

    // Shows statistics for the current node
    void showStats(int curr_node, std::ostream & out =
		   std::cout, bool header = false);

  public:


  private:

    int searchCommHistory(int);

    int id;
    vector < CommHistory > chist;
    double warm_up_time;

};

#endif
