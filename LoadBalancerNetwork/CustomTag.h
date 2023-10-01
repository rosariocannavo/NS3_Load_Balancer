#include "ns3/tag.h"
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

using namespace ns3;
using namespace CryptoPP;


class CustomTag : public Tag {
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize(TagBuffer i);
    virtual void Print(std::ostream& os) const;

    // these are our accessors to our tag structure
    void SetData(uint value);
    uint GetData(void) const;

  private:
    uint m_simpleValue;
};

TypeId CustomTag::GetTypeId(void) {
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

TypeId CustomTag::GetInstanceTypeId(void) const {
    return GetTypeId();
}

uint32_t CustomTag::GetSerializedSize(void) const {
    return 1;
}

void CustomTag::Serialize(TagBuffer i) const {
    i.WriteU8(m_simpleValue);
}

void CustomTag::Deserialize(TagBuffer i) {
    m_simpleValue = i.ReadU8();
}

void CustomTag::Print(std::ostream& os) const {
    os << "v=" << (uint32_t)m_simpleValue;
}

void CustomTag::SetData(uint value) {
    m_simpleValue = value;
}

uint CustomTag::GetData(void) const {
    return m_simpleValue;
}
