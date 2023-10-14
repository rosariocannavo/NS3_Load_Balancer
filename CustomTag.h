#ifndef CUSTOM_TAG_H
#define CUSTOM_TAG_H

#include "ns3/tag.h"

using namespace ns3;

/**
 * 
 * Create a custom tag that could be attached to a packet
 * 
 */
class CustomTag : public Tag {

    public:

        CustomTag() : m_numberValue(0) {}  // Initialize m_numberValue to 0


        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("ns3::CustomTag")
                .SetParent<Tag>()
                .AddConstructor<CustomTag>()
                .AddAttribute("NumberValue",  // Rename the attribute
                    "A numeric value",        // Update the description
                    EmptyAttributeValue(),
                    MakeUintegerAccessor(&CustomTag::GetData),  // Rename the accessors
                    MakeUintegerChecker<uint32_t>());  // Change the data type
            return tid;
        }


        virtual TypeId GetInstanceTypeId(void) const {
            return GetTypeId();
        }


        virtual uint32_t GetSerializedSize(void) const {
            return 4;  // Serialized size for a uint32_t
        }


        virtual void Serialize(TagBuffer i) const {
            i.WriteU32(m_numberValue);  // Use WriteU32 to serialize a uint32_t
        }


        virtual void Deserialize(TagBuffer i) {
            m_numberValue = i.ReadU32();  // Use ReadU32 to deserialize a uint32_t
        }


        virtual void Print(std::ostream& os) const {
            os << "Value=" << m_numberValue;  // Update the print format
        }


        void SetData(uint32_t value) {  // Update the setter and getter names
            m_numberValue = value;
        }


        uint32_t GetData(void) const {
            return m_numberValue;
        }
        

    private:

        uint32_t m_numberValue;  // Change the data type to uint32_t
};

#endif
