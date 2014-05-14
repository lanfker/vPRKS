#include "payload-buffer.h"
#include "ns3/log.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("PayloadBuffer");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (PayloadBuffer);

  TypeId PayloadBuffer::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::PayloadBuffer")
      .SetParent<Object> ()
      .AddConstructor<PayloadBuffer> ();
    return tid;
  }
  PayloadBuffer::PayloadBuffer (uint8_t* buff)
  {
    m_data = buff;
    m_current = m_data;
  }

  uint32_t PayloadBuffer::CheckRemainBytes (uint32_t bufferLength)
  {
    uint32_t offset = m_current - m_data;
    return bufferLength - offset - 1;
  }

  PayloadBuffer::PayloadBuffer ()
  {
  }

  uint8_t PayloadBuffer::ReadU8 ()
  {
    m_current ++;
    return *(m_current - 1);
  }
  uint16_t PayloadBuffer::ReadU16 ()
  {
    uint8_t byte0 = ReadU8 ();
    uint8_t byte1 = ReadU8 ();
    uint16_t value = byte1;
    value <<= 8;
    value |= byte0;

    return value;
  }


  PayloadBuffer::~PayloadBuffer ()
  {
  }

  void PayloadBuffer::WriteU8 (uint8_t value)
  {
    *m_current = value;
    m_current ++;
  }

  void PayloadBuffer::WriteU16 (uint16_t value)
  {
    WriteU8 (value & 0xff);
    value >>= 8;
    WriteU8 (value & 0xff);
  }

  void PayloadBuffer::WriteU32 (uint32_t value)
  {
    WriteU8 ((value >> 0) & 0xff);
    WriteU8 ((value >> 8) & 0xff);
    WriteU8 ((value >> 16) & 0xff);
    WriteU8 ((value >> 24) & 0xff);
  }
  void PayloadBuffer::WriteU64 (uint64_t value)
  {
    WriteU8 ((value >> 0) & 0xff);
    WriteU8 ((value >> 8) & 0xff);
    WriteU8 ((value >> 16) & 0xff);
    WriteU8 ((value >> 24) & 0xff);
    WriteU8 ((value >> 32) & 0xff);
    WriteU8 ((value >> 40) & 0xff);
    WriteU8 ((value >> 48) & 0xff);
    WriteU8 ((value >> 56) & 0xff);
  }

  uint32_t PayloadBuffer::ReadU32()
  {
    uint8_t byte0 = ReadU8 ();
    uint8_t byte1 = ReadU8 ();
    uint8_t byte2 = ReadU8 ();
    uint8_t byte3 = ReadU8 ();
    uint32_t value = byte3;
    value <<= 8;
    value |= byte2;
    value <<= 8;
    value |= byte1;
    value <<= 8;
    value |= byte0;

    return value;
  }

  uint64_t PayloadBuffer::ReadU64 ()
  {
    uint8_t byte0 = ReadU8 ();
    uint8_t byte1 = ReadU8 ();
    uint8_t byte2 = ReadU8 ();
    uint8_t byte3 = ReadU8 ();
    uint8_t byte4 = ReadU8 ();
    uint8_t byte5 = ReadU8 ();
    uint8_t byte6 = ReadU8 ();
    uint8_t byte7 = ReadU8 ();
    uint64_t value = byte7;
    value <<= 8;
    value |= byte6;
    value <<= 8;
    value |= byte5;
    value <<= 8;
    value |= byte4;
    value <<= 8;
    value |= byte3;
    value <<= 8;
    value |= byte2;
    value <<= 8;
    value |= byte1;
    value <<= 8;
    value |= byte0;

    return value;
  }

  void PayloadBuffer::WriteDouble (double value)
  {
    uint8_t* buff = (uint8_t *)& value;
    for (uint32_t i = 0; i < sizeof (double); ++i, ++ buff)
    {
      WriteU8 (*buff);
    }
  }

  double PayloadBuffer::ReadDouble ()
  {
    double value = 0;
    uint8_t* buff = (uint8_t *)&value;

    for (uint32_t i = 0; i < sizeof (double); ++ i, ++ buff)
    {
      *buff = ReadU8 ();
    }
    return value;
  }

  void PayloadBuffer::ReSetPointer ()
  {
    m_current = m_data;
  }


  void PayloadBuffer::ReadDoubles (uint32_t times)
  {
    for (uint32_t i = 0; i < times; ++ i)
    {
      ReadDouble ();
    }
  }

  uint32_t PayloadBuffer::WriteString (std::string str)
  {
    //std::cout<<" write string: "<< str << std::endl;
    uint32_t size = str.size ();
    uint32_t writeCount = 0;
    uint8_t* ptr = (uint8_t *) str.c_str ();
    for (uint32_t i = 0; i < size; ++ i)
    {
      WriteU8 (*(ptr + i));
      writeCount ++ ;
    }
    return writeCount;
  }

  std::string PayloadBuffer::ReadString (uint32_t length)
  {
    std::string str;
    for (uint32_t i = 0; i < length; ++ i)
    {
      uint8_t ch = ReadU8 ();
      str.push_back ((char)ch);
    }
    //std::cout<<" read string: "<< str << std::endl;
    return str;
  }

  void PayloadBuffer::Rewind (uint32_t offset)
  {
    m_current -= offset;
  }

  uint32_t PayloadBuffer::GetNumberOfUsedBytes ()
  {
    return m_current - m_data;
  }
}
