#ifndef SIGNAL_MAP_H 
#define SIGNAL_MAP_H

#include "ns3/core-module.h"
#include <set>
#include "ns3/road-map-module.h"


namespace ns3{


  typedef struct DirectionDistribution
  {
    double ratio[4];
    uint32_t selfSector;
  }DirectionDistribution;

#ifndef SIGNAL_MAP_ITEM
#define SIGNAL_MAP_ITEM
  typedef struct SignalMapItem
  {
    double selfx;
    double selfy;
    uint16_t from; // from is the neighbor.
    uint16_t to; // to is m_self
    double attenuation;
    Time timeStamp;
    double angle;
    double exclusionRegion;// dBm
    double x;
    double y;
    double speed;
    std::string edge;
  }SignalMapItem;
#endif 
  typedef struct RoadVehicleItem
  {
    RoadMapEdge edge;
    double roadLength;
    double minX;
    double minY;
    double maxX;
    double maxY;
    uint32_t vehicleCount;
    double angle; // assume road segments are straight segments, the angle of a road segment is fixed.
    double density; // # of vehicles within unit length road segment. (unit length:100 meters)
  }RoadVehicleItem;
  typedef struct DensityEstimation
  {
    double density;
    uint16_t vehicleId;
    std::string edgeId;
    Time timeStamp;
  }DensityEstimation;

  class SignalMap: public Object
  {
    public:
      static TypeId GetTypeId(void);
      SignalMap ();
      SignalMap (std::vector<SignalMapItem> vec);
      ~SignalMap ();
      void AddOrUpdate (SignalMapItem item, bool self);
      SignalMapItem FetchSignalMapItem (uint16_t from, uint16_t to);
      void SortAccordingToAttenuation ();
      void SortAccordingToTimeStamp ();
      void RemoveExpiredItem (Time duration);
      void PrintSignalMap (uint16_t nodeId);
      //void UpdateActiveSlots (uint16_t nodeId, uint16_t begin, uint16_t end);
      void GetConflictNodes (uint16_t slot, std::vector<uint16_t> &vec);
      void GetItemsToShare (std::vector<SignalMapItem> &vec, uint16_t count);
      uint32_t GetSize ();
      typedef std::vector<SignalMapItem>::iterator Iterator;
      Iterator begin () { return m_signalMap.begin ();}
      Iterator end () {return m_signalMap.end ();}
      void UpdateVehicleStatus (uint16_t from, double angle, double x, double y, std::string edge);
      double GetLinkExclusionRegionValue (uint16_t to, uint16_t from); // to is the receiver, from is the sender
      // additively add neighbors to the set @vec since we are using STL set
      void GetNodesInExclusionRegion (uint16_t to, double exclusionRegion, std::set<uint16_t> &vec);
      void GetOneHopNeighbors (double thresholdDbm, std::vector<uint16_t> &vec);
      std::vector<SignalMapItem> GetSignalMap ();
      std::vector<RoadVehicleItem> ComputeVehicleDirectionDistribution ();
      void SetXY (double x, double y);
      void InsertDensityEstimation (uint16_t vehicleId, std::string edgeId, double density);
      void UpdateExclusionRegion (uint16_t sender, uint16_t receiver, double exclusionRegion);
      double DistanceToNeighbor (uint16_t neighbor, double x, double y);
    private:
      std::vector<SignalMapItem> m_signalMap;
      std::vector<DensityEstimation> m_densityEstimationVector;
      double m_x;
      double m_y;

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
