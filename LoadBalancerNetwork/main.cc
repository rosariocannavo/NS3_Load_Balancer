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
#include "CustomTag.h"

/*
  star -> p2p -> lan 
*/

using namespace ns3;
using namespace std;


static void ReplicaReceivePacket(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
        Ipv4Address fromIpv4 = fromAddr.GetIpv4();

    
        //each packet has a tag and it is used for managing the responses once the replica servers responde to the load balancer
        
        CustomTag tag;
        tag.SetData(static_cast<uint>(packet->GetUid()));
        packet->AddPacketTag(tag);

        CustomTag retriviedTag;
        packet->PeekPacketTag(retriviedTag);
        cout<<"Retrivied packetTag: "<<retriviedTag.GetData()<<endl;

        //there you should manage the response
        uint32_t dataSize = packet->GetSize();
        cout<<"CUSTOM REPLICA SERVER ";
        std::cout << "Received a packet of size " << dataSize << " bytes from " << fromIpv4 << std::endl;  
    }    
}

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
    serverSocket_1->SetRecvCallback(MakeCallback(&ReplicaReceivePacket));
    
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
    
    //PING TEST FROM A NODE IN THE STAR TO ONE REPLICA - IT SHOULD NOT PING
    
    Ptr<Node> star_snd_test = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address star_snd_addr_test = star_snd_test->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"snd address inside the star: "<<star_snd_addr_test<<endl<<endl;

    UdpEchoClientHelper echoClient (repl_rcv_addr_1, 9);   
    echoClient.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (star_snd_test); //install on the node the udp client
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));
    
    */
    
    /*   
    //PING TEST FROM A NODE IN THE STAR TO THE LOAD BALANCER - IT SHOULD PING

    Ptr<Node> lb_rcv_test = p2pNodes.Get(0);
    Ipv4Address lb_rcv_addr_test = lb_rcv_test->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv address == load balancer: "<<lb_rcv_addr_test<<endl;

    Ptr<Socket> serverSocket2 = Socket::CreateSocket(lb_rcv_test, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress2 = InetSocketAddress(lb_rcv_addr_test, 9);
    serverSocket2->Bind(localAddress2);
    serverSocket2->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));


    //use the same client of before 
    Ptr<Node> star_snd_test = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address star_snd_addr_test = star_snd_test->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"snd address inside the star: "<<star_snd_addr_test<<endl<<endl;

    
    UdpEchoClientHelper echoClient2 (lb_rcv_addr_test, 9);   
    echoClient2.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps2 = echoClient2.Install (star_snd_test); //install on the node the udp client
    clientApps2.Start (Seconds (2.0));
    clientApps2.Stop (Seconds (10.0));
    */

    
   
    //PING TEST FROM THE LOAD BALANCER TO ONE OF THE REPLICA SERVER - IT SHOULD PING 

  

    Ptr<Node> lb_snd_test = replicaNodes.Get(0); //take the load balancer
    Ipv4Address lb_snd_addr_test = lb_snd_test->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

    cout<<"LOAD BALANCER ID: "<<lb_snd_test->GetId()<<endl;

    cout<<"load balancer address: "<<lb_snd_addr_test<<endl;
    cout<<"server address: "<<repl_rcv_addr_1<<endl<<endl;

    UdpEchoClientHelper echoClient3 (repl_rcv_addr_1, 9);   
    echoClient3.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient3.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps3 = echoClient3.Install (lb_snd_test); //install on the node the udp client
    clientApps3.Start (Seconds (2.0));
    clientApps3.Stop (Seconds (10.0));
    
    
    /*

    //TEST WITH OBJECT AGGREGATION
    Ptr<Node> lb_node = p2pNodes.Get(0);
    Ipv4Address lb_node_addr = lb_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();


    Ptr<Object> obj = CreateObject<Object>();
    Ptr<Socket> socket = Socket::CreateSocket(lb_node, UdpSocketFactory::GetTypeId());
    obj->AggregateObject(socket);
    InetSocketAddress localServerAddress = InetSocketAddress(lb_node_addr, 9);
    obj->GetObject<Socket>()->Bind(localServerAddress);
    obj->GetObject<Socket>()->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));
    obj->AggregateObject(p2pNodes.Get(0));      //aggregate the LB nodes

    Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(replicaNodes);
    obj->AggregateObject(lb);
    


    Ptr<Node> snd2 = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address snd_addr2 = snd2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"snd address inside the star: "<<snd_addr2<<endl<<endl;

    
    UdpEchoClientHelper echoClient_aggr (lb_node_addr, 9);   
    echoClient_aggr.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient_aggr.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient_aggr.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps_aggr = echoClient_aggr.Install (snd2); //install on the node the udp client
    clientApps_aggr.Start (Seconds (2.0));
    clientApps_aggr.Stop (Seconds (10.0));

    */


    
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