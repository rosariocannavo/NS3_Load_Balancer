#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"
#include <ns3/ipv4-address.h>
#include <ns3/tag.h>


#include "CustomTag.h"
#include "CustomClient.h"
#include "CustomServer.h"

using namespace ns3;
using namespace std;

/**
 * This class is the heart of the project, allocate a load balancer that listen to the client -> make requests to the replica -> listen the response of the replica -> send the response to the client
 * This class use CustomClient and CustomServer to communicate
 * Only the load balancer knows the addresses of the replica nodes
*/
class LoadBalancer : public Object {

    public:

        LoadBalancer(Ptr<Node> lbNode, uint exposingClientPort, uint exposingReplicaPort, uint receivingReplicaPort, uint receivingClientPort,  NodeContainer availableServers) {
            this->lbNode = lbNode;
            this->exposingReplicaPort = exposingReplicaPort;    //port to listen for the replica
            this->exposingClientPort = exposingClientPort;      //port to listen for the client
            this->receivingClientPort = receivingClientPort;    //port to contact the client after the replica responded
            this->receivingReplicaPort = receivingReplicaPort;  //port to contact the replica
            this->availableServers = availableServers;

            this->lbAddrToStarInterface = this->lbNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            this->lbAddrToReplicaInterface = this->lbNode->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();

            /*allocate client side socket (take messages from the star)*/
            this->starServant = CreateObject<CustomServer>(this->lbNode, this->lbAddrToStarInterface, this->exposingClientPort);

            /*allocate replica side socket (take messages from the replicas)*/ 
            this->replicaServant = CreateObject<CustomServer>(this->lbNode, this->lbAddrToReplicaInterface, this->exposingReplicaPort);
        }


        LoadBalancer() {}


        ~LoadBalancer() {}


        void start() {
            this->starServant->startServer(&LoadBalancer::ReceivePacketFromClient, this);
            this->replicaServant->startServer(&LoadBalancer::ReceivePacketFromReplica, this);
        }


        //this function wait for a communication from the load balancer as response
        void ReceivePacketFromReplica(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;

            while ((packet = socket->RecvFrom(from))) {
                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();

                uint32_t dataSize = packet->GetSize();

                Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

                uint8_t *buffer = new uint8_t[packet->GetSize ()];
                packet->CopyData(buffer, packet->GetSize ());
                std::string payload = std::string((char*)buffer);

                cout <<"LBfromReplica: I am: "<<receiver<< "I Received a packet of size " << dataSize << " bytes from " << fromIpv4 << " which is a replicaserver, to send to"<<payload<<endl;
                

                //trasmit the processed request to requirent client
                CustomTag tag;
                packet->PeekPacketTag(tag);

                ns3::Ipv4Address clientAddress(payload.c_str());
                

                cout<<"LBfromReplicaSender: sending obtained response for tag: "<<tag.GetData()<<" to the client "<<payload.c_str()<<" who sent the request"<<endl;
                Ptr<CustomClient> responseClient = CreateObject<CustomClient>(availableServers.Get(0));
                responseClient->sendTo(clientAddress, this->receivingClientPort, "final response by LB");

            }
        }


        //this function wait a communication from the clients and redirect it to the replica servers
        void ReceivePacketFromClient(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;

            while ((packet = socket->RecvFrom(from))) {
                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();
                    
                uint32_t dataSize = packet->GetSize();

                Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();


                //this is the point where we could implement a cache and a sticky behaviour


                std::cout <<"LB: I am: "<<receiver<< "I Received a packet of size " << dataSize << " bytes from " << fromIpv4 << std::endl;

                Ptr<Node> selectedNode = RoundRobinSelection();

                if (selectedNode != nullptr) {

                    Ipv4Address RRaddr = selectedNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
                    std::cout << "\033[0;31m LB: selected RR Node address: " << RRaddr<<"\033[0m"<<endl;  

                    CustomTag tag;
                    tag.SetData(packet->GetUid());
                    cout<<"LB: adding tag "<<packet->GetUid()<<" to packet"<<endl; 

                    //convert ipv4addr to string
                    std::ostringstream oss;
                    oss << fromIpv4;

                    cout<<"LB: sending message to the selected replica server: "<<RRaddr<<endl;
                    Ptr<CustomClient> lbClient = CreateObject<CustomClient>(availableServers.Get(0));
                    lbClient->sendTo(RRaddr, this->receivingReplicaPort, oss.str() , tag);
                
                } else {
                    std::cout << "No available servers." << std::endl;
                }
            }    
        }


        Ipv4Address getAddressForClient() {
            return this->lbAddrToStarInterface;
        }

        Ipv4Address getAddressForReplica() {
            return this->lbAddrToReplicaInterface;
        }


        uint getClientPort() {
            return this->exposingClientPort;
        }


        uint getReplicaPort() {
            return this->exposingReplicaPort;
        }
        

        static TypeId GetTypeId (void) {
            static TypeId tid = TypeId ("LoadBalancer")
            .SetParent<Object> ()
            .SetGroupName ("Demo")
            .AddConstructor<LoadBalancer> ();
            return tid;
        }


    private:

        Ptr<Node> lbNode;
        Ipv4Address lbAddrToStarInterface;
        Ipv4Address lbAddrToReplicaInterface;

        uint exposingReplicaPort;
        uint exposingClientPort;
        uint receivingClientPort;
        uint receivingReplicaPort;
        Ptr<CustomServer> starServant;
        Ptr<CustomServer> replicaServant;
        uint32_t currentRRIndex;
        NodeContainer availableServers;

        //this algorithm select one of the avaialable replica servers
        Ptr<Node> RoundRobinSelection() {
            if (availableServers.GetN() == 0) { return nullptr; }

            currentRRIndex = (currentRRIndex + 1) % availableServers.GetN();

            if (currentRRIndex == 0) { currentRRIndex = 1; }

            Ptr<Node> selectedNode = availableServers.Get(currentRRIndex);

            return selectedNode;
        }

};

#endif

