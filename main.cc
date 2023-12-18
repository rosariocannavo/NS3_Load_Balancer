#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"

#include <vector>

#include "src/LoadBalancer.h"
#include "src/ReplicaServer.h"
#include "src/CustomStarNode.h"
#include "src/Logger.h"
#include "src/Experiment.h"     

#define LISTENINGCLIENTPORT 8080
#define LISTENINGLBPORTFORCLIENT 9090
#define LISTENINGLBPORTFORREPLICA 1111
#define LISTENINGREPLICAPORT 1010

using namespace ns3;
using namespace std;

int main (int argc, char *argv[]) {
    std::cout << "__cplusplus value: " << __cplusplus << std::endl;
    

    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1000));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100kb/s"));
    Config::SetDefault ("ns3::OnOffApplication::MaxBytes", UintegerValue (50000));
    
    uint numberOfSimulation = 100;
    uint32_t nSpokes = 100; 
    uint32_t seed = 321;
    uint32_t nReplicaServers = 1;
    uint32_t nActiveClient = 100;
    uint32_t nPacketSentByEachClient = 1;   //1o if nreplicas is 10
    uint32_t packetSecondsInterval = 1;
    string   starDataRate = "1Mbps";
    string   starDelay = "2ms";
    double   clientChannelErrorRate = 0;
    string   P2PDataRate = "1Mbps";
    string   P2PDelay = "2ms";
    string   replicaDataRate = "1Mbps";
    string   replicaDelay = "2ms";
    uint32_t stickyCacheDim = 1; //nSpokes/3; 
    uint32_t selectedAlgorithm = 2;
    
    
    CommandLine cmd;
    cmd.AddValue ("numberOfSimulation", "Number of simulation to execute with actual configuration", numberOfSimulation);
    cmd.AddValue ("nSpokes", "Number of external nodes to place in the star", nSpokes);
    cmd.AddValue ("seed", "RNG seed", seed);
    cmd.AddValue ("nReplicaServers", "Number of replica server to allocate", nReplicaServers);
    cmd.AddValue ("nActiveClient", "The total number of client nodes that request packets from the load balancer (1 to nSpokes)", nActiveClient);
    cmd.AddValue ("nPacketSentByEachClient", "The number of packets sent by each client node to the load balancer", nPacketSentByEachClient);
    cmd.AddValue ("packetSecondsInterval", "The time interval between one packet send and another of a client (in Seconds)", packetSecondsInterval);
    cmd.AddValue ("starDataRate", "The data rate (bandwidth) of the star channel. This parameter defines the rate at which data can be transmitted over the channel", starDataRate);
    cmd.AddValue ("starDelay", "The delay (latency) of the star channel. This parameter defines the time it takes for packets to traverse a channel", starDelay);
    cmd.AddValue ("clientChannelErrorRate", "The error rate (or packet loss rate) of the communication channel between clients and the load balancer (0.10 = 10%)", clientChannelErrorRate);
    cmd.AddValue ("P2PDataRate", "The data rate (bandwidth) of the P2P channel (star to lb). This parameter defines the rate at which data can be transmitted over the channel", P2PDataRate);
    cmd.AddValue ("P2PDelay", "The delay (latency) of the P2P channel (star to lb). This parameter defines the time it takes for packets to traverse a channel", P2PDelay);
    cmd.AddValue ("replicaDataRate", "The data rate (bandwidth) of the Replica channel (lb to servers). This parameter defines the rate at which data can be transmitted over the channel", replicaDataRate);
    cmd.AddValue ("replicaDelay", "The delay (latency) of the P2P channel (lb to servers). This parameter defines the time it takes for packets to traverse a channel (expressed in ns)", replicaDelay);
    cmd.AddValue ("stickyCacheDim", "The number of maximum entry of the cache for sticky session", stickyCacheDim);
    cmd.AddValue ("algorithm", "Algorithm used by the load balancer: \n \t\t\t\t\t0: Round Robin; \n \t\t\t\t\t1: IpHash \n \t\t\t\t\t2: Random\n\t\t\t\t\t", selectedAlgorithm);

    cmd.Parse (argc, argv);

    Time::SetResolution (Time::NS);

    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    //struct to pass system params to Experiment class
    Params param;

    param.numberOfSimulation = numberOfSimulation;
    param.nSpokes = nSpokes;
    param.seed = seed;
    param.nReplicaServers = nReplicaServers;
    param.nActiveClient = nActiveClient;
    param.nPacketSentByEachClient = nPacketSentByEachClient;
    param.packetSecondsInterval = packetSecondsInterval;
    param.starDataRate = starDataRate;
    param.starDelay = starDelay;
    param.clientChannelErrorRate = clientChannelErrorRate;
    param.P2PDataRate = P2PDataRate;
    param.P2PDelay = P2PDelay;
    param.replicaDataRate = replicaDataRate;
    param.replicaDelay = replicaDelay;
    param.stickyCacheDim = stickyCacheDim;
    param.selectedAlgorithm = selectedAlgorithm;

    Logger logger(nActiveClient, nReplicaServers, nPacketSentByEachClient,numberOfSimulation);

    Experiment experiment(logger);

    for(uint i=0; i<numberOfSimulation;i++) {
        param.seed++; //change the simulation seed
        experiment.Run(param, i);
    }

    return 0;

}
