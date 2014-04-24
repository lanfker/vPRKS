
#ifndef OBSERVATION_H 
#define OBSERVATION_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
 */

#include "ns3/core-module.h"
#include <set>


namespace ns3{

  typedef struct ObservationItem 
  {
    uint16_t sender;
    uint16_t receiver;
    double senderX;
    double senderY;
    double receiverX;
    double receiverY;
    double averageAttenuation;
    Time timeStamp;
  } ObservationItem;
  typedef struct LinkObservations
  {
    uint16_t sender;
    uint16_t receiver;
    std::vector< ObservationItem > observations;
  }LinkObservations;


  class Observation : public Object
  {
    public:
      static TypeId GetTypeId(void);
      Observation ();
      void AppendObservation (uint16_t sender, uint16_t receiver, ObservationItem obs);
      ~Observation ();
      // find the minimum number such that we can know what is the shape of the matrix we can get from this observation
      uint32_t FindMinimumObservationLength ();
      std::set<uint16_t> FetchSenders ();
      void PrintObservations ();
      // The should at most maxCount items stored for each link
      void RemoveExpireItems (Time duration, uint32_t maxCount);
      std::vector<ObservationItem> FetchLinkObservationByReceiver (uint16_t receiver);
      std::vector<ObservationItem> FetchLinkObservationBySender (uint16_t sender, double senderX, double senderY, double receiverX, double receiverY);
      std::vector <LinkObservations> m_observations; // a vector of vectors
    private:
  };
}


#endif //OBSERVATION_H
