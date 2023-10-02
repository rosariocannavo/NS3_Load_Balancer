#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/mobility-module.h"
#include <vector>

using namespace ns3;
using namespace std;


class LoadBalancer : public Object{
    public:

        LoadBalancer() {
            // Initialize member variables with default values if needed
            this->availableServers = NodeContainer();
        }


        LoadBalancer(NodeContainer availableServers) {
            this->availableServers = availableServers;
        }   


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
                    std::cout << "Selected RR Node address: " << RRaddr<<endl;  // << std::endl;

                    //
                    UdpEchoClientHelper test_client (RRaddr, 9);   
                    test_client.SetAttribute ("MaxPackets", UintegerValue (1));
                    test_client.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
                    test_client.SetAttribute ("PacketSize", UintegerValue (1024));

                    ApplicationContainer clientApps_aggr = test_client.Install(availableServers.Get(0)); //install the client on the first replicaserver = LB
                    clientApps_aggr.Start (Seconds (1.0));
                    clientApps_aggr.Stop (Seconds (20.0));
                    //

                    
                } else {
                    std::cout << "No available servers." << std::endl;
                }
            }    
        }
        


        static TypeId GetTypeId (void) {
            static TypeId tid = TypeId ("LoadBalancer")
            .SetParent<Object> ()
            .SetGroupName ("Demo")
            .AddConstructor<LoadBalancer> ();
            return tid;
         }

    private:
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


