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
#include "CustomTime.h"
#include "DelayTag.h"

#include <random>
#include <chrono>


using namespace ns3;
using namespace std;


/**
 * Allocate a server in a replica node to satisfy the requests of the lb using a client. Server and Client are managed using CustomServer and CustomClient
 * 
 * Conf param here
 * 
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

            Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

            uint32_t dataSize = packet->GetSize();

            //collecting payload                        
            uint8_t *buffer = new uint8_t[packet->GetSize ()];
            packet->CopyData(buffer, packet->GetSize ());
            std::string payload = std::string((char*)buffer);

            CustomTag idTag;
            packet->PeekPacketTag(idTag);

            StickyTag receivedSticky;
            packet->PeekPacketTag(receivedSticky);

            std::cout<<"\033[0;33mAt time: " << CustomTime::getNowInTime().GetSeconds()<<"\033[0m "<<"REPLICA: I am: "<<receiver<< " I Received a packet of size " << dataSize << " bytes from " << fromIpv4<<" using my other interface containing message: \" " <<payload<<" \" as the original sender and tag: \""<<idTag.GetData()<<"\""<<endl;

            /*attach the computation time as tag to the response packet*/
            DelayTag delayTag;


            /*apply delay based on sticky*/
            if(receivedSticky.GetFlag() == 0) {
                //Non sticky        
                delayTag.SetDelay(simulateExecTime(2.0, 5.0, 0 )) ;

                
            }else {
                //sticky
                delayTag.SetDelay(simulateExecTime(1.0, 2.0, 1));
            }

            
            /* replying to the load balancer using the other port -> done instantiating a custom client*/
            cout<<"\033[0;33mAt time: " << CustomTime::getNowInTime().GetSeconds()<<"\033[0m "<<"REPLICA: I am "<<replicaAddr<<" Replying for the request sent by: "<<payload<<" (by lb), with tag: "<<idTag.GetData()<<endl;
            
            Ptr<CustomClient> replicaClient = CreateObject<CustomClient>(this->replicaNode);
            replicaClient->sendTo(fromIpv4, this->loadBalancerPort, payload, idTag, delayTag);
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

        double accumulatedDelayTime;
        Ptr<Node> replicaNode;  
        Ipv4Address replicaAddr; 
        uint exposingReplicaPort;
        uint loadBalancerPort;
        Ptr<CustomServer> server; 


        double simulateExecTime(double min, double max, uint flag) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(min, max);
            double delayTime = dis(gen); 

            if(flag) cout<<"\033[0;34mReplica: response solving time: "<<delayTime<<"s <STICKY>\033[0m "<<endl;
            else     cout<<"\033[0;34mReplica: response solving time: "<<delayTime<<"s <NON STICKY>\033[0m "<<endl;

            return delayTime;
            //CustomTime::addDelay(delayTime);



        }
        

};

#endif