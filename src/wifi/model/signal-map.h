#ifndef SIGNAL_MAP_H 
#define SIGNAL_MAP_H

#include "ns3/core-module.h"
#include <set>


namespace ns3{


  typedef struct DirectionDistribution
  {
    double ratio[4];
    uint32_t selfSector;
  }DirectionDistribution;
  typedef struct SignalMapItem
  {
    uint16_t from; // from is the neighbor.
    uint16_t to; // to is m_self
    double attenuation;
    Time timeStamp;
    double angle;
    uint16_t begin; // record active slot for 'from'
    uint16_t end; // record active slot fro 'from'
    double exclusionRegion;// dBm
  }SignalMapItem;
  class SignalMap: public Object
  {
    public:
      static TypeId GetTypeId(void);
      SignalMap ();
      ~SignalMap ();
      void AddOrUpdate (SignalMapItem item);
      SignalMapItem FetchSignalMapItem (uint16_t from, uint16_t to);
      void SortAccordingToAttenuation ();
      void SortAccordingToTimeStamp ();
      void RemoveExpiredItem (Time duration);
      void PrintSignalMap (uint16_t nodeId);
      //void UpdateActiveSlots (uint16_t nodeId, uint16_t begin, uint16_t end);
      DirectionDistribution GetDirectionDistribution (double angle);
      void GetConflictNodes (uint16_t slot, std::vector<uint16_t> &vec);
      void GetItemsToShare (std::vector<SignalMapItem> &vec, uint16_t count);
      uint32_t GetSize ();
      typedef std::vector<SignalMapItem>::iterator Iterator;
      Iterator begin () { return m_signalMap.begin ();}
      Iterator end () {return m_signalMap.end ();}
      void UpdateVehicleStatus (uint16_t from, double angle, uint16_t beign, uint16_t end);
      double GetLinkExclusionRegionValue (uint16_t to, uint16_t from); // to is the receiver, from is the sender
      // additively add neighbors to the set @vec since we are using STL set
      void GetNodesInExclusionRegion (uint16_t to, double exclusionRegion, std::set<uint16_t> &vec);
      void GetOneHopNeighbors (double thresholdDbm, std::vector<uint16_t> &vec);
    private:
      std::vector<SignalMapItem> m_signalMap;

      struct AttenCompare {
        bool operator () (SignalMapItem a, SignalMapItem b)
        {
          return a.attenuation< b.attenuation;
        }
      }AttenCompare;

      struct TimeStampCompare {
        bool operator () (SignalMapItem a, SignalMapItem b)
        {
          return a.timeStamp > b.timeStamp;
        }
      }TimeStampCompare;
  };


}


#endif //SIGNAL_MAP_H
