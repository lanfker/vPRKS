#ifndef LINK_ESTIMATOR_H 
#define LINK_ESTIMATOR_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
*/

#include "ns3/core-module.h"


namespace ns3{

  typedef struct LinkEstimationItem
  {
    uint16_t sender;
    uint16_t receiver;
    std::vector<uint32_t> receivedSequenceNumbers;
    double instantPdr;
    double ewmaPdr;
    Time timeStamp;
    uint32_t estimationCount;
  }LinkEstimationItem;

  class LinkEstimator : public Object
  {
    public:
      static TypeId GetTypeId(void);
      LinkEstimator ();
      ~LinkEstimator ();
      void AddSequenceNumber (uint32_t seq, uint16_t sender, uint16_t receiver, Time timeStamp);
      void InsertSeqNumber (std::vector<uint32_t> &vec, uint32_t seq);
      void PrintSeqNumbers (std::vector<uint32_t> &vec);
      bool IsPdrUpdated (uint16_t sender, uint16_t receiver, uint32_t window);
      LinkEstimationItem GetLinkEstimationItem (uint16_t sender, uint16_t receiver);
      void ClearSequenceNumbers (uint16_t sender, uint16_t receiver);
    private:
      std::vector<LinkEstimationItem> m_estimations;
      double m_coefficient;
      Time m_maxExpireTime;
  };
}


#endif //LINK_ESTIMATOR_H
