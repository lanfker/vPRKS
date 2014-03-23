#ifndef SIGNAL_MAP_H 
#define SIGNAL_MAP_H

#include "ns3/core-module.h"


namespace ns3{


  typedef struct SignalMapItem
  {
    uint16_t from;
    uint16_t to;
    double inComingAttenuation;
    double outGoingAttenuation;
    Time timeStamp;
  }SignalMapItem;
  class SignalMap: public Object
  {
    public:
      static TypeId GetTypeId(void);
      SignalMap ();
      ~SignalMap ();
      void AddOrUpdate (SignalMapItem item);
      SignalMapItem FetchSignalMapItem (uint16_t from, uint16_t to);
      void SortAccordingToInComingAttenuation ();
      void RemoveExpiredItem (Time duration);
      struct Compare {
        bool operator () (SignalMapItem a, SignalMapItem b)
        {
          return a.inComingAttenuation < b.inComingAttenuation;
        }
      }InComingAttenCompare;
    private:
      std::vector<SignalMapItem> m_signalMap;
  };


}


#endif //SIGNAL_MAP_H
