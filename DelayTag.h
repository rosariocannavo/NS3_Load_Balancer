#ifndef DELAY_TAG_H
#define DELAY_TAG_H

#include "ns3/tag.h"

using namespace std;
using namespace ns3;

/**
 * Create a tag which indicates the replica delay
*/
class DelayTag : public Tag {

    public:

        DelayTag() : m_delay(0) {}  


        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("ns3::DelayTag")
                                    .SetParent<Tag>()
                                    .AddConstructor<DelayTag>()
                                    .AddAttribute("SimpleValue",
                                                "A simple value",
                                                EmptyAttributeValue(),
                                                MakeUintegerAccessor(&DelayTag::GetDelay),
                                                MakeUintegerChecker<uint8_t>());
            return tid;
        }


        virtual TypeId GetInstanceTypeId(void) const {
            return GetTypeId();
        }


        virtual uint32_t GetSerializedSize(void) const {
            return 1;
        }


        virtual void Serialize(TagBuffer i) const {
            i.WriteU8(m_delay);
        }


        virtual void Deserialize(TagBuffer i) {
            m_delay = i.ReadU8();
        }


        virtual void Print(std::ostream& os) const {
            os << "v=" << (uint32_t)m_delay;
        }


        void SetDelay(double value) {
            m_delay = value;
        }


        double GetDelay(void) const {
            return m_delay;
        }
        

    private:

        double m_delay;

};

#endif