#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"

#include "LoadBalancer.h"
#include "ReplicaServer.h"

/*
  star -> p2p -> lan 
  star -> lb -> replica
*/

using namespace ns3;
using namespace std;

int main (int argc, char *argv[]) {
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1000));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100kb/s"));
    Config::SetDefault ("ns3::OnOffApplication::MaxBytes", UintegerValue (50000));
   
    uint32_t nSpokes = 10;
    uint32_t seed = 123;
    CommandLine cmd;
    cmd.AddValue ("nSpokes", "Number of external nodes to place in the star", nSpokes);
    cmd.AddValue ("seed", "RNG seed", seed);
    cmd.Parse (argc, argv);

    Time::SetResolution (Time::NS);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);


    //STAR ALLOCATION
    PointToPointHelper StarpointToPoint;
    StarpointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    StarpointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    PointToPointStarHelper star (nSpokes, StarpointToPoint);
    InternetStackHelper StarStackIP;
    star.InstallStack (StarStackIP);
    star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "/24"));   //STAR NET ADDRESS
    Ptr<Node> StarHub = star.GetHub();
    Ipv4Address StarHub_addr = StarHub->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"indirizo dell'hub: "<<StarHub_addr<<endl;


    //P2P ALLOCATION -> P2P 0 will be the load balancer
    NodeContainer p2pNodes;
    p2pNodes.Create(1); //p2pNodes.Get(0) will be the load balancer
    p2pNodes.Add(star.GetSpokeNode(floor(nSpokes/2)));  //a mid star node will be the gateway  
    PointToPointHelper P2PH;
    P2PH.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    P2PH.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer p2pDevices;
    p2pDevices = StarpointToPoint.Install(p2pNodes);
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
    replicaNodes.Create(1);

    PointToPointHelper p2pReplicaH;
    p2pReplicaH.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2pReplicaH.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices01 = p2pReplicaH.Install(replicaNodes);
    //NetDeviceContainer devices12 = p2pReplicaH.Install(replicaNodes.Get(0), replicaNodes.Get(2));
   // NetDeviceContainer devices23 = p2pReplicaH.Install(replicaNodes.Get(0), replicaNodes.Get(3));

    InternetStackHelper ReplicaStackH;
    ReplicaStackH.Install(replicaNodes.Get(1));
    //ReplicaStackH.Install(replicaNodes.Get(2));
    //ReplicaStackH.Install(replicaNodes.Get(3));

    Ipv4AddressHelper replicaAddressH;
    replicaAddressH.SetBase("14.1.1.0", "/24");
    Ipv4InterfaceContainer interfaces01 = replicaAddressH.Assign(devices01);
    //Ipv4InterfaceContainer interfaces12 = replicaAddressH.Assign(devices12);
    //Ipv4InterfaceContainer interfaces23 = replicaAddressH.Assign(devices23);


    //Aggregate replicanodes with a ReplicaServer Class

    
    /*
    Need to fix this code -> more than one replica does not work
    NodeContainer replicaNodes[2];
    for(uint i=0;i<2;i++) {
        replicaNodes[i].Add(p2pNodes.Get(0));
        replicaNodes[i].Create(1);
    }

    PointToPointHelper p2pReplicaH;
    p2pReplicaH.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2pReplicaH.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices[2];
    for(uint i=0;i<2;i++) {
        devices[i] = p2pReplicaH.Install(replicaNodes[i]);
    }

    InternetStackHelper ReplicaStackH;
    for(uint i=0;i<2;i++) {
        ReplicaStackH.Install(replicaNodes[i].Get(0));
        ReplicaStackH.Install(replicaNodes[i].Get(1));
    }

    Ipv4AddressHelper replicaAddressH;
    Ipv4InterfaceContainer interfaces[2];
    replicaAddressH.SetBase("14.1.1.0", "/24");
    for(uint i=0; i<2;i++) {
        
        interfaces[i] = replicaAddressH.Assign(devices[i]);
        cout<<"SANTO"<<endl;
    }
    */

    
    //server on replica 1
    Ptr<Node> repl_rcv_1 = replicaNodes.Get(1); 
    Ipv4Address repl_rcv_addr_1 = repl_rcv_1->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"repl_rcv_addr_1 address in the replica lan: "<<repl_rcv_addr_1<<endl;
    Ptr<Socket> serverSocket_1 = Socket::CreateSocket(repl_rcv_1, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_1 = InetSocketAddress(repl_rcv_addr_1, 9);
    serverSocket_1->Bind(localAddress_1);
    serverSocket_1->SetRecvCallback(MakeCallback(&ReplicaServer::ReplicaReceivePacket));
    
    /*
    // //server on replica 2
    Ptr<Node> repl_rcv_2 = replicaNodes.Get(2); 
    Ipv4Address repl_rcv_addr_2 = repl_rcv_2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"repl_rcv_addr_2 address in the replica lan: "<<repl_rcv_addr_2<<endl;
    Ptr<Socket> serverSocket_2 = Socket::CreateSocket(repl_rcv_2, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_2 = InetSocketAddress(repl_rcv_addr_2, 9);
    serverSocket_2->Bind(localAddress_2);
    serverSocket_2->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));

    // //server on replica 3
    Ptr<Node> repl_rcv_3 = replicaNodes.Get(3); 
    Ipv4Address repl_rcv_addr_3 = repl_rcv_3->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"repl_rcv_addr_3 address in the replica lan: "<<repl_rcv_addr_3<<endl;
    Ptr<Socket> serverSocket_3 = Socket::CreateSocket(repl_rcv_3, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_3 = InetSocketAddress(repl_rcv_addr_3, 9);
    serverSocket_3->Bind(localAddress_3);
    serverSocket_3->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));

    */


    


    // //PRINT ADDRESS BETWEEN STAR P2P AND REPLICA
    Ptr<Node> lb_node_sx = star.GetSpokeNode(floor(nSpokes/2));

    Ipv4Address rcv_addr_star = lb_node_sx->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();   //star interface
    std::cout<<endl<<"star gateway address on star network: "<<rcv_addr_star<<endl;

    Ipv4Address rcv_addr_p2p = lb_node_sx->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();    //p2p interface
    std::cout<<"load balancer address on p2p network: "<<rcv_addr_p2p<<endl;

    Ptr<Node> lb_node_dx = replicaNodes.Get(0);
    Ipv4Address rcv_addr_replica = lb_node_dx->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();   //replica node interface
    std::cout<<"load balancer address on replica network: "<<rcv_addr_replica<<endl<<endl;


    //COHERENCE TEST
    Ptr<Node> starSX = star.GetSpokeNode(floor(nSpokes/2));
    Ptr<Node> p2pSX = p2pNodes.Get(1);

    if(starSX == p2pSX) cout<<"TRUE"<<endl;
    else cout<<"FALSE"<<endl;;

    Ptr<Node> p2pDX = p2pNodes.Get(0);
    Ptr<Node> replicaSX = replicaNodes.Get(0);

    if(p2pDX == replicaSX) cout<<"TRUE"<<endl;
    else cout<<"FALSE"<<endl;


    
 

    /*
    //TEST WITH OBJECT AGGREGATION FROM STAR TO LB
    Ptr<Node> lb_node = p2pNodes.Get(0);
    Ipv4Address lb_node_addr = lb_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();


    Ptr<Object> obj = CreateObject<Object>();
    Ptr<Socket> socket = Socket::CreateSocket(lb_node, UdpSocketFactory::GetTypeId());
    obj->AggregateObject(socket);
    InetSocketAddress localServerAddress = InetSocketAddress(lb_node_addr, 9);
    obj->GetObject<Socket>()->Bind(localServerAddress);
    obj->GetObject<Socket>()->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));

    obj->AggregateObject(p2pNodes.Get(0));      //aggregate the LB nodes
    //this is were the load balancer get exposed
    Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(replicaNodes);
    obj->AggregateObject(lb);


    Ptr<Node> star_client_node = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address star_client_node_addr = star_client_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    cout<<"node in the star ( "<<star_client_node_addr <<" ) contacting load balancer at addr: "<<lb_node_addr<<endl;
    */


    /*this class here is probably not needed because we want to simulate n requests*/
    //CustomClient starClient = CustomClient(star_client_node);
    //starClient.sendTo(lb_node_addr, 9, "ciao");
    





    //create a load balancer that expose itself
    Ptr<Node> lb_node = p2pNodes.Get(0);
    Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(lb_node, 9, replicaNodes);

    //allocate a client in a random server in the star to contact the load balancer
    UdpEchoClientHelper echoClient_aggr (lb->getAddress(), lb->getPort());   
    echoClient_aggr.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient_aggr.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient_aggr.SetAttribute ("PacketSize", UintegerValue (1024));
    
    Ptr<Node> star_client_node = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address star_client_node_addr = star_client_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    cout<<"node in the star ( "<<star_client_node_addr <<" ) contacting load balancer at addr: "<<lb->getAddress()<<endl;

    ApplicationContainer clientApps_aggr = echoClient_aggr.Install (star_client_node); //install on the node the udp client
    clientApps_aggr.Start (Seconds (1.0));
    clientApps_aggr.Stop (Seconds (20.0));
    

    

 


 

    // AnimationInterface anim("demo_star2.xml");
    // float x = 500.0, y = 500.0, dx = 50.0;

    // anim.SetConstantPosition(star.GetHub(), x, y);
    // anim.UpdateNodeColor(star.GetHub(), 255, 0, 0); // Red color for hub

    // for (uint32_t i = 0; i < nSpokes; ++i) {
    //     if (star.GetSpokeNode(i) == star.GetHub()) continue;
    //     anim.SetConstantPosition(star.GetSpokeNode(i), x + dx * (i + 1), y);
    //     anim.UpdateNodeColor(star.GetSpokeNode(i), 0, 255, 0); // Green color for spokes
    // }   

    // anim.SetConstantPosition(p2pNodes.Get(0), x + dx * (nSpokes + 2 + 1), y);
    // anim.UpdateNodeColor(p2pNodes.Get(0), 0, 0, 255); // Blue color for p2pNodes.Get(0)

    // for (uint32_t i = 0; i < replicaNodes.GetN(); ++i) {
    //     anim.SetConstantPosition(replicaNodes.Get(i), x + dx * (i + 1) + 50, y + 50);
    //     anim.UpdateNodeColor(star.GetSpokeNode(i), 255,255, 0); // Green color for spokes
    // }


    


    Simulator::Run ();
    Simulator::Destroy ();

    return 0;




}