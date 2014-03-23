//The code of this file is based on code from (http://blog.csdn.net/zmazon/article/details/8241348)
/* This file is build for testing TraCI such that NS-3 may be work with SUMO cooperatively. 
 * 2013-12-02 Mon 10:20 AM
 */

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
      uint8_t ReadU8 ();

      void WriteU8 (uint8_t value);
      void WriteDouble (double value);

    private:
      uint8_t* m_data;
      uint8_t* m_current;
  };


}


#endif //PAYLOAD_BUFFER_H
