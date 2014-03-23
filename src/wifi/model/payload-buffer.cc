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

  PayloadBuffer::PayloadBuffer ()
  {
  }

  uint8_t PayloadBuffer::ReadU8 ()
  {
    m_current ++;
    return *(m_current - 1);
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

  PayloadBuffer::~PayloadBuffer ()
  {
  }

  void PayloadBuffer::WriteU8 (uint8_t value)
  {
    *m_current = value;
    m_current ++;
  }

  void PayloadBuffer::WriteDouble (double value)
  {
    uint8_t* buff = (uint8_t *)& value;
    for (uint32_t i = 0; i < sizeof (double); ++i, ++ buff)
    {
      WriteU8 (*buff);
    }
  }

}
