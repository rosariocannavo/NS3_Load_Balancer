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

/* this class does not use customclient because is used for simulation*/
class CustomStarNode : public Object {
    public:
        
        CustomStarNode(Ptr<Node> starNode, uint exposingRcvPort, Ptr<LoadBalancer> lb) {
            this->lb = lb;

            this->starNode = starNode;
            this->exposingRcvPort = exposingRcvPort;
            this->starNodeAddr = starNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

            //TODO!: implement with customServer
            /* Install a simple server to receive data*/
            this->rcvSocket = Socket::CreateSocket(this->starNode, UdpSocketFactory::GetTypeId());
            this->rcvSocket->Bind(InetSocketAddress(this->starNodeAddr, this->exposingRcvPort));

        }   


        CustomStarNode() {}


        ~CustomStarNode() {}


        void start() {
            this->rcvSocket->SetRecvCallback(MakeCallback(&CustomStarNode::ReceivePacketAsFinalResponse, this));

            /* install a client with clientHelper*/
            UdpEchoClientHelper echoClientHelper(lb->getAddressForClient(), lb->getClientPort());
            echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (1));
            echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
            echoClientHelper.SetAttribute ("PacketSize", UintegerValue (1024)); 
            this->applicationContainer = echoClientHelper.Install(this->starNode);
            this->applicationContainer.Start (Seconds (1.0));
            this->applicationContainer.Stop (Seconds (10.0));
        }

        
        void ReceivePacketAsFinalResponse(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;
            
            while ((packet = socket->RecvFrom(from))) {
                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();

                CustomTag tag;
                packet->PeekPacketTag(tag);

                uint8_t *buffer = new uint8_t[packet->GetSize ()];
                packet->CopyData(buffer, packet->GetSize ());
                std::string payload = std::string((char*)buffer);

                cout<<"CLIENT: I am "<<this->starNodeAddr<<", my request has been fulfilled by: "<<fromIpv4<<" I received response: \""<<payload<<"\""<<endl;
               
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
        Ptr<Socket> rcvSocket;
        Ptr<Node> starNode;
        Ptr<LoadBalancer> lb;
        uint exposingRcvPort;
        Ipv4Address starNodeAddr;
        ApplicationContainer applicationContainer; 
    
};