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

using namespace ns3;
using namespace std;


class LoadBalancer : public Object{
    public:
        /*at allocation time load balancer expose a udp server and knows all the replica servers available*/
        LoadBalancer(Ptr<Node> lbNode, uint exposingPort, NodeContainer availableServers) {
            this->lbNode = lbNode;
            this->exposingPort = exposingPort;
            this->availableServers = availableServers;

            this->socket = Socket::CreateSocket(this->lbNode, UdpSocketFactory::GetTypeId());
            this->lbAddr = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            
            this->socket->Bind(InetSocketAddress(lbAddr, exposingPort));
            this->socket->SetRecvCallback(MakeCallback(&LoadBalancer::ReceivePacket));
        }


        LoadBalancer() {}


        ~LoadBalancer() {}

     
        static void ReceivePacket(Ptr<Socket> socket) {
            Ptr<Packet> packet;
            Address from;

            while ((packet = socket->RecvFrom(from))) {
                InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
                Ipv4Address fromIpv4 = fromAddr.GetIpv4();
                    
                uint32_t dataSize = packet->GetSize();

                Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
                std::cout <<"LB: I am: "<<receiver<< "I Received a packet of size " << dataSize << " bytes from " << fromIpv4 << std::endl;


                Ptr<Node> selectedNode = RoundRobinSelection();

                if (selectedNode != nullptr) {

                    Ipv4Address RRaddr = selectedNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
                    std::cout << "LB: selected RR Node address: " << RRaddr<<endl;  

                    CustomTag tag;
                    cout<<"LB: adding tag "<<packet->GetUid()<<" to packet"<<endl; 

                    //convert ipv4addr to string
                    std::ostringstream oss;
                    oss << fromIpv4;

                    CustomClient lbClient = CustomClient(availableServers.Get(0));
                    lbClient.sendTo(RRaddr, 9, oss.str() , tag);
                
                } else {
                    std::cout << "No available servers." << std::endl;
                }
            }    
        }


        Ipv4Address getAddress() {
            return this->lbAddr;
        }


        uint getPort() {
            return this->exposingPort;
        }
        

        static TypeId GetTypeId (void) {
            static TypeId tid = TypeId ("LoadBalancer")
            .SetParent<Object> ()
            .SetGroupName ("Demo")
            .AddConstructor<LoadBalancer> ();
            return tid;
         }

    private:

        uint exposingPort;
        Ptr<Node> lbNode;
        Ipv4Address lbAddr; 
        Ptr<Socket> socket; 
        static uint32_t currentRRIndex;
        static NodeContainer availableServers;


        static Ptr<Node> RoundRobinSelection() {
                if (availableServers.GetN() == 0) {
                    return nullptr;  // No available servers
                }
        
                currentRRIndex = (currentRRIndex + 1) % availableServers.GetN();
        
            if (currentRRIndex == 0) {
                // Skip index 0, move to the next index
                currentRRIndex = 1;
            }
            
            Ptr<Node> selectedNode = availableServers.Get(currentRRIndex);
            return selectedNode;
        }


};

uint32_t LoadBalancer::currentRRIndex = 0;
NodeContainer LoadBalancer::availableServers;


