#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-echo-server.h"

#include <vector>

using namespace ns3;
using namespace std;

class ReplicaServer : public Object {
  public:
    ReplicaServer() {}

    ~ReplicaServer() {}

    static TypeId GetTypeId(void) {
        static TypeId tid = TypeId("ReplicaServer")
                                .SetParent<Object>()
                                .SetGroupName("Demo")
                                .AddConstructor<ReplicaServer>();
        return tid;
    }

    static void ReplicaReceivePacket(Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from))) {
            InetSocketAddress fromAddr = InetSocketAddress::ConvertFrom(from);
            Ipv4Address fromIpv4 = fromAddr.GetIpv4();

            // each packet has a tag and it is used for managing the responses once the replica
            // servers responde to the load balancer

            CustomTag tag;
            tag.SetData(static_cast<uint>(packet->GetUid()));
            packet->AddPacketTag(tag);

            CustomTag retriviedTag;
            packet->PeekPacketTag(retriviedTag);
            cout << "Retrivied packetTag: " << retriviedTag.GetData() << endl;

            // there you should manage the response
            uint32_t dataSize = packet->GetSize();
            
            Ipv4Address receiver = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

            std::cout<<"REPLICA: I am: "<<receiver<< " I Received a packet of size " << dataSize << " bytes from " << fromIpv4<< std::endl;
        }
    }

  private:

};