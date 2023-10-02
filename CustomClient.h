#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-socket.h"


using namespace ns3;
using namespace std;

class CustomClient {
    public:

        CustomClient() {}

        CustomClient(Ptr<Node> node) {
            this->node = node;
            this->socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
            this->socket->Bind();
        }


        ~CustomClient() {}


        void sendTo(Ipv4Address receiver, uint bindingPort, string message,  CustomTag tag) {
            cout<<"I am "<<this->node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<" sending custom message "<<message<<" to "<<receiver<<" on port "<<bindingPort<<endl;

            this->socket->Connect(InetSocketAddress(receiver, bindingPort));

            Ptr<Packet> packet = Create<Packet>((const uint8_t*)message.c_str(), message.size());

            packet->AddPacketTag(tag);
            
            this->socket->Send(packet);
        }


        void sendTo(Ipv4Address receiver, uint bindingPort, string message) {
            cout<<"I am "<<this->node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<" sending custom message "<<message<<" to "<<receiver<<" on port "<<bindingPort<<endl;

            this->socket->Connect(InetSocketAddress(receiver, bindingPort));

            Ptr<Packet> packet = Create<Packet>((const uint8_t*)message.c_str(), message.size());

            this->socket->Send(packet);
        }


    private:
        Ptr<Node> node;
        Ptr<Socket> socket; 

};