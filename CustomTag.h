#include "ns3/tag.h"
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

using namespace ns3;
using namespace CryptoPP;

class CustomTag : public Tag {
  public:
    CustomTag() : m_simpleValue(0) {}  

    static TypeId GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CustomTag")
                                .SetParent<Tag>()
                                .AddConstructor<CustomTag>()
                                .AddAttribute("SimpleValue",
                                              "A simple value",
                                              EmptyAttributeValue(),
                                              MakeUintegerAccessor(&CustomTag::GetData),
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


    // Accessors
    void SetData(uint value) {
        m_simpleValue = value;
    }


    uint GetData(void) const {
        return m_simpleValue;
    }
    

  private:
    uint m_simpleValue;
};
