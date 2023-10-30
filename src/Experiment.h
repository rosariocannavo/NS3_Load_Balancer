#ifndef EXPERIMENT_H
#define EXPERIMENT_H

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

struct Params {
    uint numberOfSimulation;
    uint32_t nSpokes;
    uint32_t seed;
    uint32_t nReplicaServers;
    uint32_t nActiveClient;
    uint32_t nPacketSentByEachClient;
    uint32_t packetSecondsInterval;
    string   starDataRate;
    string   starDelay;
    double   clientChannelErrorRate;
    string   P2PDataRate;
    string   P2PDelay;
    string   replicaDataRate;
    string   replicaDelay;
    uint32_t stickyCacheDim;
    uint32_t selectedAlgorithm;
};

class Experiment {

    public:

        //use the copy constructor of Logger
        Experiment(const Logger &logger) : logger(logger) { }


        ~Experiment() { }


        void Run(Params param, int currentNOfSimulation) {
            //change seed of the simulation
            RngSeedManager::SetSeed(param.seed);

            cout<<"STARTING SIMULATION "<<currentNOfSimulation+1<<" of "<<param.numberOfSimulation<<endl;;
            //STAR ALLOCATION
            PointToPointHelper StarpointToPoint;
            StarpointToPoint.SetDeviceAttribute ("DataRate", StringValue (param.starDataRate));
            StarpointToPoint.SetChannelAttribute ("Delay", StringValue (param.starDelay));
            PointToPointStarHelper star (param.nSpokes, StarpointToPoint);
            InternetStackHelper StarStackIP;
            star.InstallStack (StarStackIP);
            star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.0.0", "/22"));   //STAR NET ADDRESS up to 1000 nodes
            Ptr<Node> StarHub = star.GetHub();
            Ipv4Address StarHub_addr = StarHub->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            std::cout<<"star Hub Address: "<<StarHub_addr<<endl;



            //P2P ALLOCATION -> P2P 0 will be the load balancer
            NodeContainer p2pNodes;
            p2pNodes.Create(1); //p2pNodes.Get(0) will be the load balancer
            p2pNodes.Add(star.GetSpokeNode(floor(param.nSpokes/2)));  //a mid star node will be the gateway  
            PointToPointHelper P2PH;
            P2PH.SetDeviceAttribute("DataRate", StringValue(param.P2PDataRate));
            P2PH.SetChannelAttribute("Delay", StringValue(param.P2PDelay));
            NetDeviceContainer p2pDevices;
            p2pDevices = StarpointToPoint.Install(p2pNodes);


            /*packet loss in the channel between star and loadbalancer*/
            Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
            em->SetAttribute ("ErrorRate", DoubleValue (param.clientChannelErrorRate)); // Set the packet loss rate to 10% (0.1) ATTENTION ATTENTION ATTENTIONATTENTION ATTENTIONATTENTION ATTENTION
            p2pDevices.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


            InternetStackHelper stackH;   //install the stack only on the fresh node, the other 
            stackH.Install (p2pNodes.Get(0));

            Ipv4AddressHelper addressH;
            addressH.SetBase ("12.1.0.0", "/30");    //BUS NET ADDRESS
            Ipv4InterfaceContainer p2pInterfaces;
            p2pInterfaces = addressH.Assign(p2pDevices);



            //POPULATE ROUTING TABLE - place it before the replica server so they are unreachable by the star's nodes
            Ipv4GlobalRoutingHelper::PopulateRoutingTables();
            //Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
            //Ipv4RoutingHelper::PrintRoutingTableAllAt( Time (Seconds (10.0)), stream ,Time::S);



            //REPLICASERVER ALLOCATION
            NodeContainer replicaNodes;
            replicaNodes.Add(p2pNodes.Get(0));
            replicaNodes.Create(param.nReplicaServers); //number of replicas

            CsmaHelper csmah;
            csmah.SetChannelAttribute ("DataRate", StringValue (param.replicaDataRate));
            csmah.SetChannelAttribute ("Delay", StringValue (param.replicaDelay));

            NetDeviceContainer csmaDevices;
            csmaDevices = csmah.Install (replicaNodes);

            InternetStackHelper replicaStackH;
            replicaStackH.Install (replicaNodes);

            Ipv4AddressHelper replicaAddressH;
            replicaAddressH.SetBase("14.1.0.0", "/28"); //up to 10 nodes
            Ipv4InterfaceContainer csmaInterfaces;
            csmaInterfaces = replicaAddressH.Assign (csmaDevices);

            

            //PRINT GENERAL INFO
            cout<<"Each client is avaiable at port: "<<LISTENINGCLIENTPORT<<endl;

            Ptr<Node> lb_node_sx = star.GetSpokeNode(floor(param.nSpokes/2));
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
            Ptr<Node> starSX = star.GetSpokeNode(floor(param.nSpokes/2));
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
            Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(lb_node, LISTENINGLBPORTFORCLIENT, LISTENINGLBPORTFORREPLICA, LISTENINGREPLICAPORT , LISTENINGCLIENTPORT, replicaNodes, param.selectedAlgorithm, param.stickyCacheDim); 
            lb->start();


            //allocate on each star node a client to receive responses
            Ptr<CustomStarNode> starNodes[param.nSpokes];

            for(uint i=0; i<param.nSpokes; i++) { 
                starNodes[i] = CreateObject<CustomStarNode>(star.GetSpokeNode(i), LISTENINGCLIENTPORT, lb, param.nPacketSentByEachClient, param.packetSecondsInterval, i);
            }
            

            /*initialize the logger*/
            //Logger logger(param.nActiveClient, param.nReplicaServers, param.nPacketSentByEachClient);


            /*generate the random number which represent the n client selected*/
            std::set<int> uniqueNumbers;
            std::srand(static_cast<unsigned>(std::time(nullptr)));


            while (uniqueNumbers.size() < param.nActiveClient) {
                int randomNum = std::rand() % (param.nSpokes-1 + 1);
                if (uniqueNumbers.count(randomNum) == 0) {
                    uniqueNumbers.insert(randomNum);
                }
            }


            for(const int& num : uniqueNumbers) {    
                Ptr<CustomStarNode> selectedClient = starNodes[num];

                /*every time a node is selected add it to the logger watcher*/
                logger.addNode(selectedClient, currentNOfSimulation);

                cout<<"\033[0;33mAt time: " << Simulator::Now().GetSeconds()<<"\033[0m "<<"node in the star ( "<<selectedClient->getAddress() <<" ) contacting load balancer at addr: "<<lb->getAddressForClient()<<endl;
                selectedClient->start();
            
            }    


            //this func get the stop time  for now a longer time is a patch   
            Simulator::Schedule(Seconds(1000000), &Logger::simulateStats, &logger, currentNOfSimulation);      //getStats

            Simulator::Run();
            Simulator::Destroy();
        }

    private:
        Logger logger;
};

#endif 