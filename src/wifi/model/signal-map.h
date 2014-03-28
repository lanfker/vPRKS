#ifndef SIGNAL_MAP_H 
#define SIGNAL_MAP_H

#include "ns3/core-module.h"


namespace ns3{


  typedef struct SignalMapItem
  {
    uint16_t from;
    uint16_t to;
    double attenuation;
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
      void PrintSignalMap (uint16_t nodeId);
      uint32_t GetSize ();
      struct Compare {
        bool operator () (SignalMapItem a, SignalMapItem b)
        {
          return a.attenuation< b.attenuation;
        }
      }InComingAttenCompare;
      typedef std::vector<SignalMapItem>::iterator Iterator;
      //typedef std::vector<SignalMapItem>::const_iterator ConstIterator;
      Iterator begin () { return m_signalMap.begin ();}
      //ConstIterator begin () {return m_signalMap.begin (); }
      Iterator end () {return m_signalMap.end ();}
      //ConstIterator end () {return m_signalMap.end (); }
    private:
      std::vector<SignalMapItem> m_signalMap;
  };


}


#endif //SIGNAL_MAP_H
