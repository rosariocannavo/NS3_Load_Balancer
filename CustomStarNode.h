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

#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "CustomServer.h"
#include "TimeStampTag.h"
#include "CustomClient.h"
#include "CustomTime.h"
#include "DelayTag.h"


using namespace ns3;
using namespace std;
namespace fs = std::filesystem;



ostream& operator<<(ostream& os, const unordered_map<uint, pair<Time, Time> >* RTTTracer) {
    double totalRTT = 0.0;
    for (const auto& pair : *RTTTracer) {
        os << "Key: " << pair.first<<": ";
        os << " { sndTime: ";
        os << pair.second.first.GetSeconds()<<"s";
        os << ", rcvTime: ";
        os << pair.second.second.GetSeconds()<<"s";
        os << " }, RTT: ";
        Time RTT = pair.second.second - pair.second.first; 
        os << RTT.GetSeconds(); 
        totalRTT += RTT.GetSeconds();
        os << "s\n";
    }

    os <<endl<<"meanRTT: "<<totalRTT / RTTTracer->size()<<endl;

    return os;
}


ostream& operator<<(ostream& os, const pair<Time, Time>& timePair) {
    os << " { sndTime: ";
    os << timePair.first.GetSeconds()<<"s";
    os << ", rcvTime: ";
    os << timePair.second.GetSeconds()<<"s";
    os << " }, RTT: ";
    Time RTT = timePair.second - timePair.first; 
    os << RTT.GetSeconds(); 
    os << "s";
    return os;
}


/**
 * 
 * Conf Param Here
 * 
 * Allocate a server in a client node (from the star) to receive requests to the load balancer
 * this class does not use customclient because is used for simulation
*/
class CustomStarNode : public Object {

    public:
        
        CustomStarNode(Ptr<Node> starNode, uint exposingRcvPort, Ptr<LoadBalancer> lb, uint nPacketToSend, uint PacketSecondsInterval, uint fileId) {
            this->lb = lb;
            this->starNode = starNode;
            this->exposingRcvPort = exposingRcvPort;
            this->starNodeAddr = starNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            this->nPacketToSend = nPacketToSend;
            this->PacketSecondsInterval = PacketSecondsInterval;
            this->fileId = fileId;

            //install a server in the node
            this->rcvServer = CreateObject<CustomServer>(this->starNode, this->starNodeAddr, this->exposingRcvPort);

            RTTTracer = new unordered_map<uint, pair<Time, Time> >();

        }   


        CustomStarNode() {}


        ~CustomStarNode() {}


        void start() {
            this->rcvServer->startServer(&CustomStarNode::ReceivePacketAsFinalResponse, this);
            
            // install a client with clientHelper
            UdpEchoClientHelper echoClientHelper(lb->getAddressForClient(), lb->getClientPort());
  
            echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (this->nPacketToSend));    //each node send 5 packets
            echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (this->PacketSecondsInterval))); //interval between a packet and another
            echoClientHelper.SetAttribute ("PacketSize", UintegerValue (1024));    
            this->applicationContainer = echoClientHelper.Install(this->starNode);
        
            /*Add the tags to the packet sent using the udp client helper calling the AddTagCallback method */
            Ptr<Application> app = this->applicationContainer.Get(0);
            Ptr<UdpEchoClient> client = DynamicCast<UdpEchoClient>(app);
            client->TraceConnectWithoutContext("Tx", MakeCallback(&CustomStarNode::AddTagCallback, this));
        }

     
        
        void ReceivePacketAsFinalResponse(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;
            
            while ((packet = socket->RecvFrom(from))) {
                /*get the rcv time to calculate the rtt*/
                Time rcvTime =CustomTime::getNowInTime();    

                
                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();

                /*retrive the packet tag*/
                CustomTag idTag;
                packet->PeekPacketTag(idTag);

                /*retrieve the delay tag*/
                DelayTag delayTag;
                packet->PeekPacketTag(delayTag);
                
                /*insert the rcv time in the custom structure -> rember to take the tag, the packet->guid is from the received not from the original*/
                /*add the computational delay*/
                (*RTTTracer)[idTag.GetData()].second = rcvTime + ns3::Seconds(delayTag.GetDelay()); 

                /*write in a file the logs of the client*/
                    
                ofstream outputFile("/home/rosario/clientRTT/client"+to_string(this->fileId)+".txt");
                outputFile<< this->RTTTracer;
                outputFile.close();


                uint8_t *buffer = new uint8_t[packet->GetSize ()];
                packet->CopyData(buffer, packet->GetSize ());
                std::string payload = std::string((char*)buffer);   

                cout<<"\033[0;33mAt time: " << CustomTime::getNowInTime().GetSeconds()<<"\033[0m "<<"\033[0;32mCLIENT: I am "<<this->starNodeAddr<<", my request has been fulfilled by: "<<fromIpv4<<" I received response: \""<<payload<<"\" which is the replica server that managed my requests with tag: "<<idTag.GetData()<<" \033[0m"<<endl;
                cout<<"\033[0;33mAt time: " << CustomTime::getNowInTime().GetSeconds()<<"\033[0m "<<"\033[0;32mCLIENT: packetIdTag: \": "<<idTag.GetData()<<"\""<<(*RTTTracer)[idTag.GetData()]<<"\033[0m"<<endl; //here using operator overload
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


        uint getNPacketToSend() {
            return this->nPacketToSend;
        }


        uint getPacketSecondsInterval() {
            return this->PacketSecondsInterval;
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
        uint nPacketToSend;
        uint PacketSecondsInterval;
        uint fileId;
        Ipv4Address starNodeAddr;
        ApplicationContainer applicationContainer; 
        unordered_map<uint, pair<Time, Time> >* RTTTracer;
        

        /*declare this operator overload friend to access the data from the structure*/
        friend std::ostream& operator<<(std::ostream& os, const std::pair<ns3::Time, ns3::Time>& timePair);


        void AddTagCallback(Ptr<const Packet> packet) {
            (*RTTTracer)[packet->GetUid()].first = CustomTime::getNowInTime(); 

            CustomTag idTag;    //identify the single packet
            idTag.SetData(packet->GetUid());
            packet->AddPacketTag(idTag);
        }
  
};

#endif