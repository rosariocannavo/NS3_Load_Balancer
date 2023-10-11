#ifndef CUSTOM_CLIENT_H
#define CUSTOM_CLIENT_H

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

#include "TimeStampTag.h"
#include "CustomTime.h"


using namespace ns3;
using namespace std;

/**
 * Allocate a custom client on a node to send messages
*/
class CustomClient : public Object {

    public:

        CustomClient(Ptr<Node> node) {
            this->node = node;
            this->socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
            this->socket->Bind();
        }


        CustomClient() {}


        ~CustomClient() {}


        /*this function uses ellipsis operator to adapt to a list of generic parameters*/
        template <typename... Tags>
        void sendTo(Ipv4Address receiver, uint bindingPort, string message, Tags... tags) {
            cout << "\033[0;33mAt time: " << CustomTime::getNowInTime().GetSeconds() << "\033[0m "
                << "CC: I am " << this->node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()
                << " sending custom message \"" << message << "\" to " << receiver
                << " on port " << bindingPort << endl;

            this->socket->Connect(InetSocketAddress(receiver, bindingPort));

            Ptr<Packet> packet = Create<Packet>((const uint8_t*)message.c_str(), message.size());

            (packet->AddPacketTag(tags), ...);  // Add all provided tags to the packet

            this->socket->Send(packet);
        }


        void closeSocket() {
            this->socket->Close();
        }


        static TypeId GetTypeId (void) {
            static TypeId tid = TypeId ("CustomClient")
            .SetParent<Object> ()
            .SetGroupName ("Demo")
            .AddConstructor<CustomClient> ();
            return tid;
        }


    private:
    
        Ptr<Node> node;
        Ptr<Socket> socket; 

};

#endif 