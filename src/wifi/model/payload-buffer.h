#ifndef PAYLOAD_BUFFER_H 
#define PAYLOAD_BUFFER_H

#include "ns3/core-module.h"


namespace ns3{


  class PayloadBuffer: public Object
  {
    public:
      static TypeId GetTypeId(void);
      PayloadBuffer (uint8_t* buff);
      PayloadBuffer ();
      ~PayloadBuffer ();
      double ReadDouble ();
      void ReadDoubles (uint32_t times);
      uint8_t ReadU8 ();
      uint16_t ReadU16 ();
      uint32_t CheckRemainBytes (uint32_t bufferLength);

      void WriteU8 (uint8_t value);
      void WriteU16 (uint16_t value);
      void WriteDouble (double value);

      void ReSetPointer ();

    private:
      uint8_t* m_data;
      uint8_t* m_current;
  };


}


#endif //PAYLOAD_BUFFER_H
