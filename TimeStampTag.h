#ifndef TIMESTAMP_TAG_H
#define TIMESTAMP_TAG_H

#include "ns3/tag.h"
#include "ns3/simulator.h"

using namespace ns3;


//TimestampTag timestampTag;
//    timestampTag.SetTimestamp(Simulator::Now());
/**
 * Use this class to attach a custom tag to a packet and analyze the time difference
*/
class TimestampTag : public Tag {
        
    public:

        TimestampTag()  {
            this->m_timestamp = 0;
        }

        TimestampTag() {}


        ~TimestampTag() {}
    

        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("ns3::TimestampTag")
                .SetParent<Tag>()
                .SetGroupName ("Demo")
                .AddConstructor<TimestampTag>();
            return tid;
        }


        virtual TypeId GetInstanceTypeId(void) const {
            return GetTypeId();
        }


        virtual uint32_t GetSerializedSize(void) const {
            return sizeof(Time);
        }


        virtual void Serialize(TagBuffer i) const {
            i.Write(m_timestamp);
        }


        virtual void Deserialize(TagBuffer i) {
            i.Read(m_timestamp);
        }


        virtual void Print(std::ostream& os) const {
            os << "Timestamp: " << m_timestamp;
        }


        void SetTimestamp(Time timestamp) {
            m_timestamp = timestamp;
        }


        Time GetTimestamp(void) const {
            return m_timestamp;
        }


    private:

        Time m_timestamp;

};

#endif



