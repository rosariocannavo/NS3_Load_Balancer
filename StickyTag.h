#ifndef STICKY_TAG_H
#define STICKY_TAG_H

#include "ns3/tag.h"

using namespace std;
using namespace ns3;

/**
 * Create a tag which indicates if the session is sticky
*/
class StickyTag : public Tag {

  public:

    StickyTag() : m_simpleValue(0) {}  

    static TypeId GetTypeId(void) {
        static TypeId tid = TypeId("ns3::StickyTag")
                                .SetParent<Tag>()
                                .AddConstructor<StickyTag>()
                                .AddAttribute("SimpleValue",
                                              "A simple value",
                                              EmptyAttributeValue(),
                                              MakeUintegerAccessor(&StickyTag::GetFlag),
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
        i.WriteU8(m_simpleValue);
    }


    virtual void Deserialize(TagBuffer i) {
        m_simpleValue = i.ReadU8();
    }


    virtual void Print(std::ostream& os) const {
        os << "v=" << (uint32_t)m_simpleValue;
    }


    void SetFlag(uint value) {
        m_simpleValue = value;
    }


    uint GetFlag(void) const {
        return m_simpleValue;
    }
    


  private:

    uint m_simpleValue;

};

#endif