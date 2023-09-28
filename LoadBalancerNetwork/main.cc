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
    P2PH.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    P2PH.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer p2pDevices;
    p2pDevices = StarpointToPoint.Install(p2pNodes);
    InternetStackHelper stackH;   //install the stack only on the fresh node, the other 
    stackH.Install (p2pNodes.Get(0));

    Ipv4AddressHelper addressH;
    addressH.SetBase ("12.1.1.0", "/24");    //BUS NET ADDRESS
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = addressH.Assign(p2pDevices);


    

    //REPLICASERVER ALLOCATION
    NodeContainer replicaNodes;
    replicaNodes.Add(p2pNodes.Get(0));
    replicaNodes.Create(3);

    PointToPointHelper p2pReplicaH;
    p2pReplicaH.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pReplicaH.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer devices01 = p2pReplicaH.Install(replicaNodes.Get(0), replicaNodes.Get(1));
    NetDeviceContainer devices12 = p2pReplicaH.Install(replicaNodes.Get(0), replicaNodes.Get(2));
    NetDeviceContainer devices23 = p2pReplicaH.Install(replicaNodes.Get(0), replicaNodes.Get(3));

    InternetStackHelper ReplicaStackH;
    ReplicaStackH.Install(replicaNodes.Get(1));
    ReplicaStackH.Install(replicaNodes.Get(2));
    ReplicaStackH.Install(replicaNodes.Get(3));

    Ipv4AddressHelper replicaAddressH;
    replicaAddressH.SetBase("14.1.1.0", "/24");

    Ipv4InterfaceContainer interfaces01 = replicaAddressH.Assign(devices01);
    Ipv4InterfaceContainer interfaces12 = replicaAddressH.Assign(devices12);
    Ipv4InterfaceContainer interfaces23 = replicaAddressH.Assign(devices23);


    //server on replica 1
    Ptr<Node> rcv_1 = replicaNodes.Get(1); 
    Ipv4Address rcv_addr_1 = rcv_1->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv_1 address in the replica lan: "<<rcv_addr_1<<endl;
    Ptr<Socket> serverSocket_1 = Socket::CreateSocket(rcv_1, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_1 = InetSocketAddress(rcv_addr_1, 9090);
    serverSocket_1->Bind(localAddress_1);
    serverSocket_1->SetRecvCallback(MakeCallback(&ReplicaReceivePacket));

    //server on replica 2
    Ptr<Node> rcv_2 = replicaNodes.Get(2); 
    Ipv4Address rcv_addr_2 = rcv_2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv_2 address in the replica lan: "<<rcv_addr_2<<endl;
    Ptr<Socket> serverSocket_2 = Socket::CreateSocket(rcv_2, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_2 = InetSocketAddress(rcv_addr_2, 9090);
    serverSocket_2->Bind(localAddress_2);
    serverSocket_2->SetRecvCallback(MakeCallback(&ReplicaReceivePacket));

    //server on replica 3
    Ptr<Node> rcv_3 = replicaNodes.Get(3); 
    Ipv4Address rcv_addr_3 = rcv_3->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv_3 address in the replica lan: "<<rcv_addr_3<<endl;
    Ptr<Socket> serverSocket_3 = Socket::CreateSocket(rcv_3, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress_3 = InetSocketAddress(rcv_addr_3, 9090);
    serverSocket_3->Bind(localAddress_3);
    serverSocket_3->SetRecvCallback(MakeCallback(&ReplicaReceivePacket));




    //POPULATE ROUTING TABLE
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    //PRINT ADDRESS BETWEEN STAR P2P AND REPLICA
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


    
    
    //PING TEST FROM A NODE IN THE STAR TO ONE REPLICA - IT SHOULD NOT PING
    Ptr<Node> rcv = replicaNodes.Get(2); 
    Ipv4Address rcv_addr = rcv->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv address in the replica lan: "<<rcv_addr<<endl;

    Ptr<Socket> serverSocket = Socket::CreateSocket(rcv, UdpSocketFactory::GetTypeId()); 
    InetSocketAddress localAddress = InetSocketAddress(rcv_addr, 9);
    serverSocket->Bind(localAddress);
    serverSocket->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));


    Ptr<Node> snd = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address snd_addr = snd->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"snd address inside the star: "<<snd_addr<<endl<<endl;

    
    UdpEchoClientHelper echoClient (rcv_addr, 9);   
    echoClient.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (snd); //install on the node the udp client
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));


    //PING TEST FROM A NODE IN THE STAR TO THE LOAD BALANCER - IT SHOULD PING
    Ptr<Node> rcv2 = p2pNodes.Get(0);
    Ipv4Address rcv_addr2 = rcv2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"rcv address == load balancer: "<<rcv_addr2<<endl;

    // Ptr<Socket> serverSocket2 = Socket::CreateSocket(rcv2, UdpSocketFactory::GetTypeId()); 
    // InetSocketAddress localAddress2 = InetSocketAddress(rcv_addr2, 9);
    // serverSocket2->Bind(localAddress2);
    // serverSocket2->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));


    // Ptr<Node> snd2 = star.GetSpokeNode(1); //take the first node of the star
    // Ipv4Address snd_addr2 = snd2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    // std::cout<<"snd address inside the star: "<<snd_addr2<<endl<<endl;

    
    // UdpEchoClientHelper echoClient2 (rcv_addr2, 9);   
    // echoClient2.SetAttribute ("MaxPackets", UintegerValue (4));
    // echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

    // ApplicationContainer clientApps2 = echoClient2.Install (snd2); //install on the node the udp client
    // clientApps2.Start (Seconds (2.0));
    // clientApps2.Stop (Seconds (10.0));



    //PING TEST FROM THE LOAD BALANCER TO ONE OF THE REPLICA SERVER - IT SHOULD PING 

    //WARN -> IT DOES NOT PING

    Ptr<Node> snd_3 = replicaNodes.Get(0); //take the load balancer
    Ipv4Address snd_addr_3 = snd_3->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"load balancer address: "<<snd_addr_3<<endl<<endl;

    UdpEchoClientHelper echoClient3 (rcv_addr_2, 9090);   
    echoClient3.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient3.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps3 = echoClient3.Install (snd_3); //install on the node the udp client
    clientApps3.Start (Seconds (2.0));
    clientApps3.Stop (Seconds (10.0));



    //TEST WITH OBJECT AGGREGATION
    Ptr<Object> obj = CreateObject<Object>();
    Ptr<Socket> socket = Socket::CreateSocket(rcv2, UdpSocketFactory::GetTypeId());
    obj->AggregateObject(socket);
    InetSocketAddress localServerAddress = InetSocketAddress(rcv_addr2, 9);
    obj->GetObject<Socket>()->Bind(localServerAddress);
    obj->GetObject<Socket>()->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));
    obj->AggregateObject(p2pNodes.Get(0));      //aggregate the LB nodes

    Ptr<LoadBalancer> lb = CreateObject<LoadBalancer>(replicaNodes);
    obj->AggregateObject(lb);
    



    Ptr<Node> snd2 = star.GetSpokeNode(1); //take the first node of the star
    Ipv4Address snd_addr2 = snd2->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    std::cout<<"snd address inside the star: "<<snd_addr2<<endl<<endl;

    
    UdpEchoClientHelper echoClient2 (rcv_addr2, 9);   
    echoClient2.SetAttribute ("MaxPackets", UintegerValue (4));
    echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps2 = echoClient2.Install (snd2); //install on the node the udp client
    clientApps2.Start (Seconds (2.0));
    clientApps2.Stop (Seconds (10.0));

    


    
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
