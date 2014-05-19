#ifndef EXCLUSION_REGION_HELPER_H 
#define EXCLUSION_REGION_HELPER_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
*/

#include "ns3/core-module.h"
#include "signal-map.h"
#include <set>


namespace ns3{

  typedef struct LinkExclusionRegion
  {
    uint16_t sender;
    uint16_t receiver;
    double currentExclusionRegion;
    uint8_t version;
    double distance;
    double senderX;
    double senderY;
    double receiverX;
    double receiverY;
  }LinkExclusionRegion;


  typedef struct ParameterObservation
  {
    uint16_t sender;
    uint16_t receiver;
    double senderX;
    double senderY;
    double receiverX;
    double receiverY;
    double parameter;
  }ParameterObservation;
  typedef struct ReceiverObservations
  {
    uint16_t sender;
    uint16_t receiver;
    std::vector<ParameterObservation> observations;
  }ReceiverObservations;
  class ExclusionRegionHelper : public Object
  {
    public:
      static TypeId GetTypeId(void);
      ExclusionRegionHelper ();
      ~ExclusionRegionHelper ();
      double AdaptExclusionRegion (SignalMap signalMap, double deltaInterference, LinkExclusionRegion item, double txPower, double interferenceW, double ewmaPdr);
      double DbmToW (double dBm);
      double WToDbm (double w);
      void AddOrUpdateExclusionRegion (LinkExclusionRegion item);
      std::vector<LinkExclusionRegion> GetLatestUpdatedItems (uint32_t count, SignalMap signalMap);
      double GetExclusionRegion (uint16_t sender, uint16_t receiver);
      LinkExclusionRegion GetExclusionRegionRecord (uint16_t sender, uint16_t receiver);
      std::set<uint16_t> GetSenders ();
      std::vector<ParameterObservation> GetObservationsBySender (uint16_t sender, double senderX, double senderY, double receiverX, double receiverY);

    private:
      std::vector<LinkExclusionRegion> m_exclusionRegionCollection;
  };
}


#endif //EXCLUSION_REGION_HELPER_H
