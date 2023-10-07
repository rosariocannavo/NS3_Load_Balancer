#ifndef CUSTOM_STARNODE_H
#define CUSTOM_STARNODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"
#include "ns3/udp-socket.h"

#include "CustomServer.h"
#include "TimeStampTag.h"
#include "CustomClient.h"

using namespace ns3;
using namespace std;

/**
 * 
 * Conf Param Here
 * 
 * Allocate a server in a client node (from the star) to receive requests to the load balancer
 * this class does not use customclient because is used for simulation
*/
class CustomStarNode : public Object {

    public:
        
        CustomStarNode(Ptr<Node> starNode, uint exposingRcvPort, Ptr<LoadBalancer> lb) {
            this->lb = lb;

            this->starNode = starNode;
            this->exposingRcvPort = exposingRcvPort;
            this->starNodeAddr = starNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

            //install a server in the node
            this->rcvServer = CreateObject<CustomServer>(this->starNode, this->starNodeAddr, this->exposingRcvPort);

        }   


        CustomStarNode() {}


        ~CustomStarNode() {}


        void start() {
            this->rcvServer->startServer(&CustomStarNode::ReceivePacketAsFinalResponse, this);
            
            // install a client with clientHelper
            UdpEchoClientHelper echoClientHelper(lb->getAddressForClient(), lb->getClientPort());
  
            echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (5));    //each node send 5 packets
            echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
            echoClientHelper.SetAttribute ("PacketSize", UintegerValue (1024));    
            this->applicationContainer = echoClientHelper.Install(this->starNode);
        
            /*Add the tags to the packet sent using the udp client helper calling the AddTagCallback method */
            Ptr<Application> app = this->applicationContainer.Get(0);
            Ptr<UdpEchoClient> client = DynamicCast<UdpEchoClient>(app);
            client->TraceConnectWithoutContext("Tx", MakeCallback(&CustomStarNode::AddTagCallback, this));

            // this->applicationContainer.Start (Seconds (1.0));
            // this->applicationContainer.Stop (Seconds (10.0));
        }

     
        
        void ReceivePacketAsFinalResponse(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;
            
            while ((packet = socket->RecvFrom(from))) {
                //TODO FIX THE RTT TIME maybe create a data structure
                /*get the rcv time to calculate the rtt*/
                Time rcvTime = Simulator::Now();    

                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();

                CustomTag idTag;
                packet->PeekPacketTag(idTag);

                TimestampTag tsTag;
                packet->PeekPacketTag(tsTag);

                Time RTT = rcvTime - tsTag.GetTimestamp();

                uint8_t *buffer = new uint8_t[packet->GetSize ()];
                packet->CopyData(buffer, packet->GetSize ());
                std::string payload = std::string((char*)buffer);   


                cout<<"\033[0;33mAt time: "<<Simulator::Now()<<"\033[0m "<<"\033[0;32mCLIENT: I am "<<this->starNodeAddr<<", my request has been fulfilled by: "<<fromIpv4<<" I received response: \""<<payload<<"\" which is the replica server that managed my requests with tag: "<<idTag.GetData()<<" \033[0m"<<endl;
                cout<<"\033[0;33mAt time: "<<Simulator::Now()<<"\033[0m "<<"\033[0;32mCLIENT: packet sent time: \""<<tsTag.GetTimestamp()<<"\", Packet receive time: \""<<rcvTime<<"\", RTT: "<<RTT.GetMilliSeconds()<<" ms\033[0m"<<endl<<endl;
            }
        }


        Ipv4Address getAddress() {
            return this->starNodeAddr;
        }


        uint getClientPort() {
            return this->exposingRcvPort;
        }


        Ipv4Address getLoadBalancerAddress() {
            return this->lb->getAddressForClient();
        }

        uint getLoadBalancerPort() {
            return this->lb->getClientPort();
        }


        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("CustomStarNode")
                                    .SetParent<Object>()
                                    .SetGroupName("Demo")
                                    .AddConstructor<CustomStarNode>();
            return tid;
        }


    private:

        Ptr<CustomServer> rcvServer;
        Ptr<Node> starNode;
        Ptr<LoadBalancer> lb;
        uint exposingRcvPort;
        Ipv4Address starNodeAddr;
        ApplicationContainer applicationContainer; 
        

        void AddTagCallback(Ptr<const Packet> packet) {
            TimestampTag timestampTag;
            timestampTag.SetTimestamp(Simulator::Now());

            CustomTag idTag;    //identify the single packet
            idTag.SetData(packet->GetUid());

            packet->AddPacketTag(timestampTag);
            packet->AddPacketTag(idTag);
        }

    
};

#endif