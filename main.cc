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

#include "LoadBalancer.h"
#include "ReplicaServer.h"
#include "CustomStarNode.h"
#include "Logger.h"

#define LISTENINGCLIENTPORT 8080
#define LISTENINGLBPORTFORCLIENT 9090
#define LISTENINGLBPORTFORREPLICA 1111
#define LISTENINGREPLICAPORT 1010

using namespace ns3;
using namespace std;

/**
 * 
 * Conf Param here 
 * 
 * star (gw->) p2p (lb->) replica 
*/

int main (int argc, char *argv[]) {
    std::cout << "__cplusplus value: " << __cplusplus << std::endl;

    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1000));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100kb/s"));
    Config::SetDefault ("ns3::OnOffApplication::MaxBytes", UintegerValue (50000));
   
    uint32_t nSpokes = 100; //number of nodes in the star
    uint32_t seed = 321;
    uint32_t nReplicaServers = 5;
    uint32_t nActiveClient = 2;
    uint32_t nPacketSentByEachClient = 100;
    uint32_t packetSecondsInterval = 1;
    string   starDataRate = "1Mbps";
    string   starDelay = "2ms";
    double   clientChannelErrorRate = 0;
    string   P2PDataRate = "1Mbps";
    string   P2PDelay = "2ms";
    string   replicaDataRate = "1Mbps";
    string   replicaDelay = "2ms";
    uint32_t stickyCacheDim = 5;//nSpokes/3;
    uint32_t selectedAlgorithm = 0;
    
    
    CommandLine cmd;
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

    


    //STAR ALLOCATION
    PointToPointHelper StarpointToPoint;
    StarpointToPoint.SetDeviceAttribute ("DataRate", StringValue (starDataRate));
    StarpointToPoint.SetChannelAttribute ("Delay", StringValue (starDelay));
    PointToPointStarHelper star (nSpokes, StarpointToPoint);
    InternetStackHelper StarStackIP;
    star.InstallStack (StarStackIP);
    star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "/24"));   //STAR NET ADDRESS
    Ptr<Node> StarHub = star.GetHub();
    Ipv4Address StarHub_addr = StarHub->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"star Hub Address: "<<StarHub_addr<<endl;



    //P2P ALLOCATION -> P2P 0 will be the load balancer
    NodeContainer p2pNodes;
    p2pNodes.Create(1); //p2pNodes.Get(0) will be the load balancer
    p2pNodes.Add(star.GetSpokeNode(floor(nSpokes/2)));  //a mid star node will be the gateway  
    PointToPointHelper P2PH;
    P2PH.SetDeviceAttribute("DataRate", StringValue(P2PDataRate));
    P2PH.SetChannelAttribute("Delay", StringValue(P2PDelay));
    NetDeviceContainer p2pDevices;
    p2pDevices = StarpointToPoint.Install(p2pNodes);


    /*packet loss in the channel between star and loadbalancer*/
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (clientChannelErrorRate)); // Set the packet loss rate to 10% (0.1) ATTENTION ATTENTION ATTENTIONATTENTION ATTENTIONATTENTION ATTENTION
    p2pDevices.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


    InternetStackHelper stackH;   //install the stack only on the fresh node, the other 
    stackH.Install (p2pNodes.Get(0));

    Ipv4AddressHelper addressH;
    addressH.SetBase ("12.1.1.0", "/24");    //BUS NET ADDRESS
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = addressH.Assign(p2pDevices);



    //POPULATE ROUTING TABLE - place it before the replica server so they are unreachable by the star's nodes
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    //Ipv4RoutingHelper::PrintRoutingTableAllAt( Time (Seconds (10.0)), stream ,Time::S);



    //REPLICASERVER ALLOCATION
    NodeContainer replicaNodes;
    replicaNodes.Add(p2pNodes.Get(0));
    replicaNodes.Create(nReplicaServers); //number of replicas

    CsmaHelper csmah;
    csmah.SetChannelAttribute ("DataRate", StringValue (replicaDataRate));
    csmah.SetChannelAttribute ("Delay", StringValue (replicaDelay));

    NetDeviceContainer csmaDevices;
    csmaDevices = csmah.Install (replicaNodes);

    InternetStackHelper replicaStackH;
    replicaStackH.Install (replicaNodes);

    Ipv4AddressHelper replicaAddressH;
    replicaAddressH.SetBase("14.1.1.0", "/24");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = replicaAddressH.Assign (csmaDevices);

    

    //PRINT GENERAL INFO
    cout<<"Each client is avaiable at port: "<<LISTENINGCLIENTPORT<<endl;

    Ptr<Node> lb_node_sx = star.GetSpokeNode(floor(nSpokes/2));
    Ipv4Address rcv_addr_star = lb_node_sx->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();   //star interface
    std::cout<<"star gateway address on star network: "<<rcv_addr_star<<endl;

    Ptr<Node> lb_addr = p2pNodes.Get(0);
    
    Ipv4Address rcv_addr_p2p = lb_addr->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();    //p2p interface
    std::cout<<"load balancer address on p2p network (for clients): "<<rcv_addr_p2p<<endl<<" reachable at port: "<<LISTENINGLBPORTFORCLIENT<<endl;

    Ptr<Node> lb_node_dx = replicaNodes.Get(0);
    Ipv4Address rcv_addr_replica = lb_node_dx->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();   //replica node interface
    std::cout<<"load balancer address on replica network (for replica servers): "<<rcv_addr_replica<<" reachable at port: "<<LISTENINGLBPORTFORREPLICA<<endl;

    cout<<"Replica servers addresses"<<endl;
    for(uint i=1; i< replicaNodes.GetN(); i++) {
        cout<<"server "<<i<<" "<<replicaNodes.Get(i)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<" reachable from load balancer at port: "<<LISTENINGREPLICAPORT<<endl;
    }

    

    //COHERENCE TEST -> check if the nodes are equals in the different networks
    Ptr<Node> starSX = star.GetSpokeNode(floor(nSpokes/2));
    Ptr<Node> p2pSX = p2pNodes.Get(1);

    if(starSX == p2pSX) cout<<"TRUE"<<endl;
    else cout<<"FALSE"<<endl;;

    Ptr<Node> p2pDX = p2pNodes.Get(0);
    Ptr<Node> replicaSX = replicaNodes.Get(0);

    if(p2pDX == replicaSX) cout<<"TRUE"<<endl;
    else cout<<"FALSE"<<endl;

    
    //allocate Servers on each node of the replica network
    Ptr<ReplicaServer> replicaServers[replicaNodes.GetN()-1];


    for(uint i=1; i<replicaNodes.GetN();i++) {
        replicaServers[i-1] = CreateObject<ReplicaServer>(replicaNodes.Get(i), LISTENINGREPLICAPORT, LISTENINGLBPORTFORREPLICA); 
    }

    for(uint i=1; i<replicaNodes.GetN();i++) {
        replicaServers[i-1]->start(); 
    }

    

    //create a load balancer that expose itself
    Ptr<Node> lb_node = p2pNodes.Get(0);
    Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(lb_node, LISTENINGLBPORTFORCLIENT, LISTENINGLBPORTFORREPLICA, LISTENINGREPLICAPORT , LISTENINGCLIENTPORT, replicaNodes, selectedAlgorithm, stickyCacheDim); 
    lb->start();


    //allocate on each star node a client to receive responses
    Ptr<CustomStarNode> starNodes[nSpokes];

    for(uint i=0; i<nSpokes; i++) { 
        starNodes[i] = CreateObject<CustomStarNode>(star.GetSpokeNode(i), LISTENINGCLIENTPORT, lb, nPacketSentByEachClient, packetSecondsInterval, i);
    }

    
    Logger logger(nReplicaServers, nPacketSentByEachClient);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1, nSpokes-1); 
    for(uint i=0;i<nActiveClient; i++) {    //number of client which partecipate to the simulation 
        int random_node = dis(gen);  

        Ptr<CustomStarNode> selectedClient = starNodes[random_node];

        /*every time a node is selected add it to the logger watcher*/
        logger.addNode(selectedClient);

        cout<<"\033[0;33mAt time: " << Simulator::Now().GetSeconds()<<"\033[0m "<<"node in the star ( "<<selectedClient->getAddress() <<" ) contacting load balancer at addr: "<<lb->getAddressForClient()<<endl;
        selectedClient->start();
      
    }    


    //this func get the stop time  for now a longer time is a patch   
    Simulator::Schedule(Seconds(100.0), &Logger::getStats, &logger);

    Simulator::Run();
    Simulator::Destroy();
    
    return 0;

}
