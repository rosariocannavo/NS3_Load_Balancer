#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"

class CustomServer : public Object {
    public: 

        CustomServer(Ptr<Node> node, Ipv4Address address, uint port) {
            this->node = node;
            this->address = address;
            this->port = port;

            this->socket = Socket::CreateSocket(this->node, UdpSocketFactory::GetTypeId());
            this->socket->Bind(InetSocketAddress(this->address, this->port));
        }


        CustomServer() {}


        ~CustomServer() {}

        template <typename T>
        void startServer(void (T::*callbackFunction)(Ptr<Socket>), T* instance) {

            this->socket->SetRecvCallback(MakeCallback(callbackFunction, instance));

        }


        static TypeId GetTypeId (void) {
            static TypeId tid = TypeId ("CustomServer")
            .SetParent<Object> ()
            .SetGroupName ("Demo")
            .AddConstructor<CustomServer> ();
            return tid;
        }


        Ipv4Address getServerAddress() {
            return this->address;
        }

        uint getServerPort() {
            return this->port;
        }



    private:
        Ptr<Node> node;
        Ptr<Socket> socket;
        Ipv4Address address;
        uint port;
};