#ifndef REPLICA_SERVER_H
#define REPLICA_SERVER_H

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-echo-server.h"

#include "CustomServer.h"
#include "CustomClient.h"

using namespace ns3;
using namespace std;

/**
 * Allocate a server in a replica node to satisfy the requests of the lb using a client. Server and Client are managed using CustomServer and CustomClient
*/
class ReplicaServer : public Object {

  public:

    ReplicaServer(Ptr<Node> replicaNode, uint exposingReplicaPort, uint loadBalancerPort) {
        this->replicaNode = replicaNode;
        this->exposingReplicaPort = exposingReplicaPort;   //port used to listen
        this->replicaAddr =  this->replicaNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

        this->loadBalancerPort = loadBalancerPort;  //port to contact the load balancer

        this->server = CreateObject<CustomServer>(this->replicaNode, this->replicaAddr, this->exposingReplicaPort);

    }


    ReplicaServer() {}


    ~ReplicaServer() {}


    void start() {
        this->server->startServer(&ReplicaServer::ReplicaReceivePacket, this);
    }


    void ReplicaReceivePacket(Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from))) {
            InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
            Ipv4Address fromIpv4 = fromAddr.GetIpv4();

            // each packet has a tag and it is used for managing the responses once the replica
            // servers responde to the load balancer

            Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

            //collecting payload                        
            uint32_t dataSize = packet->GetSize();

            uint8_t *buffer = new uint8_t[packet->GetSize ()];
            packet->CopyData(buffer, packet->GetSize ());
            std::string payload = std::string((char*)buffer);

            CustomTag retriviedTag;
            packet->PeekPacketTag(retriviedTag);

            std::cout<<"\033[0;33mAt time: "<<Simulator::Now()<<"\033[0m "<<"REPLICA: I am: "<<receiver<< " I Received a packet of size " << dataSize << " bytes from " << fromIpv4<<" using my other interface containing message: \" " <<payload<<" \" as the original sender and tag: \""<<retriviedTag.GetData()<<"\""<<endl;

            /* replying to the load balancer using the other port -> done instantiating a custom client*/
            cout<<"\033[0;33mAt time: "<<Simulator::Now()<<"\033[0m "<<"REPLICA: I am "<<replicaAddr<<" Replying for the request sent by: "<<payload<<" (by lb), with tag: "<<retriviedTag.GetData()<<endl;
            Ptr<CustomClient> replicaClient = CreateObject<CustomClient>(this->replicaNode);
            replicaClient->sendTo(fromIpv4, this->loadBalancerPort, payload, retriviedTag);

        }
    }


    uint getReplicaPort() {
        return this->exposingReplicaPort;
    }


    Ipv4Address getReplicaAddr() {
        return this->replicaAddr;
    }


    static TypeId GetTypeId(void) {
        static TypeId tid = TypeId("ReplicaServer")
                                .SetParent<Object>()
                                .SetGroupName("Demo")
                                .AddConstructor<ReplicaServer>();
        return tid;
    }



    private:
    
        Ptr<Node> replicaNode;
        Ipv4Address replicaAddr; 
        //Ptr<Socket> socket; 
        uint exposingReplicaPort;
        uint loadBalancerPort;

        Ptr<CustomServer> server; 

};

#endif