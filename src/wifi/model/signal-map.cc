#include "signal-map.h"
#include "ns3/log.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("SignalMap");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (SignalMap);

  TypeId SignalMap::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::SignalMap")
      .SetParent<Object> ()
      .AddConstructor<SignalMap> ();
    return tid;
  }

  SignalMap::SignalMap ()
  {
  }

  SignalMap::~SignalMap ()
  {
  }
  void SignalMap::AddOrUpdate (SignalMapItem item)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (item.from == it->from && item.to == it->to )
      {
        it->attenuation = item.attenuation;
        it->timeStamp = item.timeStamp;
        it->angle = item.angle;
        it->x = item.x;
        it->y = item.y;
        it->edge = item.edge;
        return;
      }
    }

    // if no such from-to pair exist, create a new entry
    item.exclusionRegion = DEFAULT_EXCLUSION_REGION; //dbm;
    m_signalMap.push_back (item);
    NS_LOG_DEBUG ("New item added, signal map size: "<< m_signalMap.size ());
  }

  SignalMapItem SignalMap::FetchSignalMapItem (uint16_t from, uint16_t to)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (from == it->from && to == it->to )
      {
        return *it;
      }
    }
    // not found;
    SignalMapItem nullItem;
    nullItem.from = 0;
    nullItem.to = 0;
    return nullItem;
  }

  void SignalMap::SortAccordingToAttenuation ()
  {
    sort (m_signalMap.begin (), m_signalMap.end (), AttenCompare);
  }

  void SignalMap::SortAccordingToTimeStamp ()
  {
    sort (m_signalMap.begin (), m_signalMap.end (), TimeStampCompare);
  }


  //duration is the expiration time
  void SignalMap::RemoveExpiredItem (Time duration)
  {
    uint32_t removeIndex = m_signalMap.size ();
    for (uint32_t i = 0; i < m_signalMap.size (); ++ i)
    {
      if ( m_signalMap[i].timeStamp  < Simulator::Now () - duration) // expired
      {
        removeIndex -- ;
        // suppose the swap procedure works
        SignalMapItem item = m_signalMap[i];
        m_signalMap[i] = m_signalMap[removeIndex];
        m_signalMap[removeIndex] = item;
      }
    }
    if ( removeIndex < m_signalMap.size ())
    {
      m_signalMap.erase (m_signalMap.end () - removeIndex, m_signalMap.end ());
    }
  }

  void SignalMap::PrintSignalMap (uint16_t nodeId)
  {
    std::cout<<"-------------------signal map for node: "<< nodeId << "-------------------"<< std::endl;
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      std::cout<<"from: "<<it->from<<" to: "<<it->to <<" Atten: "<< it->attenuation <<" timestamp: "<< it->timeStamp 
        <<" angle: "<< it->angle <<" exclusionRegion: "<< it->exclusionRegion 
        <<" x: "<<it->x <<" y: "<< it->y <<" edge: "<< it->edge << std::endl;
    }
    std::cout<<"--------------------------------------------------"<< std::endl;
  }

  uint32_t SignalMap::GetSize ()
  {
    return m_signalMap.size ();
  }


  void SignalMap::GetConflictNodes (uint16_t slot, std::vector<uint16_t> &vec)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      /*
      if ( it->begin <= slot && it->end >= slot)
      {
        vec.push_back (it->from);
      }
      */
      return;
    }
  }


  void SignalMap::GetItemsToShare (std::vector<SignalMapItem> &vec, uint16_t count)
  {
    SortAccordingToTimeStamp ();
    for (uint32_t i = 0; i < count && i < m_signalMap.size (); ++ i)
    {
      vec.push_back (m_signalMap[i]);
    } 
    SortAccordingToAttenuation ();
  }

  void SignalMap::UpdateVehicleStatus (uint16_t from, double angle, double x, double y, std::string edge)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (it->from == from)
      {
        it->angle = angle;
        it->x = x;
        it->y = y;
        it->edge = edge;
        continue;
      }
    } 
  }

  double SignalMap::GetLinkExclusionRegionValue (uint16_t to, uint16_t from)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (from == it->from && to == it->to)
      {
        return it->exclusionRegion;
      }
    }
    return 0;
  }

  void SignalMap::GetNodesInExclusionRegion (uint16_t to, double exclusionRegion, std::set<uint16_t> &vec)
  {
    //need check expiration????
    SortAccordingToAttenuation ();
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (it->to == to)
      {
        if ( DEFAULT_POWER - it->attenuation <= exclusionRegion)
        {
          //vec.push_back (it->from);
          vec.insert (it->from);
        }
        else
        {
          break;
        }
      }
      else
      {
        break;
      }
    }
  }

  void SignalMap::GetOneHopNeighbors (double thresholdDbm, std::vector<uint16_t> &vec)
  {
    SortAccordingToAttenuation ();
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if ( DEFAULT_POWER - it->attenuation >= thresholdDbm)
      {
        vec.push_back (it->from);
      }
      else
      {
        break;
      }
    }
  }
  std::vector<SignalMapItem> SignalMap::GetSignalMap ()
  {
    return m_signalMap;
  }

  SignalMap::SignalMap (std::vector<SignalMapItem> vec)
  {
    for (std::vector<SignalMapItem>::iterator it = vec.begin (); it != vec.end (); ++ it)
    {
      AddOrUpdate (*it);
    }
    SortAccordingToAttenuation ();
  }

  std::vector<RoadVehicleItem> SignalMap::ComputeVehicleDirectionDistribution ()
  {
    std::vector<RoadVehicleItem> vec;
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      for (std::vector<RoadMapEdge>::iterator edgeIt = EdgeXmlParser::m_mapEdges.begin (); edgeIt != EdgeXmlParser::m_mapEdges.end (); ++ edgeIt)
      {
        //std::cout<<" edgeid: "<< edgeIt->edgeId << std::endl;
        double x1,x2,y1,y2;
        x1 = edgeIt->from.xCoordinate;
        x2 = edgeIt->to.xCoordinate;
        y1 = edgeIt->from.yCoordinate;
        y2 = edgeIt->to.yCoordinate;
        //double length = sqrt ( pow (it->x - m_x, 2) + pow (it->y - m_y, 2));
        // in this road segment.
        if ( it->edge == edgeIt->edgeId) 
        {
          bool found = false;
          for ( std::vector<RoadVehicleItem>::iterator vecIt = vec.begin (); vecIt != vec.end (); ++ vecIt)
          {
            if (vecIt->edge.edgeId == edgeIt->edgeId)
            {
              found = true;
              vecIt->vehicleCount += 1;
              if ( it->x != vecIt->minX)
              {
                if ( it->x < vecIt->minX)
                {
                  vecIt->minX = it->x;
                  vecIt->minY = it->y;
                }
                else if ( it->x > vecIt->maxX)
                {
                  vecIt->maxX = it->x;
                  vecIt->maxY = it->y;
                }
              }
              else if ( it->y != vecIt->minY)
              {
                if (it->y < vecIt->minY)
                {
                  vecIt->minX = it->x;
                  vecIt->minY = it->y;
                }
                else if (it->y > vecIt->maxY)
                {
                  vecIt->maxX = it->x;
                  vecIt->maxY = it->y;
                }
              }
              break;
            }
          }
          if ( found == false)
          {
            RoadVehicleItem item;
            item.edge = *edgeIt;
            item.roadLength = 0;
            item.minX = it->x;
            item.maxX = it->x;
            item.minY = it->y;
            item.maxY = it->y;
            item.vehicleCount = 1;
            item.angle = it->angle;
            vec.push_back (item);
          }
        } 
        //Not in this road segment
        else
        {
          //std::cout<<" not on road: "<< edgeIt->edgeId << std::endl;
          continue;
        }
      }
    }
    for ( std::vector<RoadVehicleItem>::iterator vecIt = vec.begin (); vecIt != vec.end (); ++ vecIt)
    {
      vecIt->roadLength = sqrt (pow (vecIt->minX - vecIt->maxX, 2) + pow (vecIt->minY - vecIt->maxY, 2));
      if ( vecIt->roadLength > 0)
      {
        vecIt->density = vecIt->vehicleCount * 100.0 / vecIt->roadLength; // again, unit length is 100 meters
      }
      else 
      {
        vecIt->density = 0;
      }
      //std::cout<<" edge: "<< vecIt->edge.edgeId <<" length: "<< vecIt->roadLength <<" count: "<< vecIt->vehicleCount <<" angle: "<< vecIt->angle<< " minX: "<< vecIt->minX <<" minY: "<< vecIt->minY <<" maxX: "<< vecIt->maxX <<" maxY: "<< vecIt->maxY << std::endl;
      
    }
    return vec;
  }

  void SignalMap::SetXY (double x, double y)
  {
    m_x = x;
    m_y = y;
  }

  void SignalMap::InsertDensityEstimation (uint16_t vehicleId, std::string edgeId, double density)
  {
    for (std::vector<DensityEstimation>::iterator it = m_densityEstimationVector.begin (); 
        it != m_densityEstimationVector.end (); ++ it)
    {
      if ( it->edgeId == edgeId )
      {
        it->density = it->density * EWMA_COEFFICIENT + (1 - EWMA_COEFFICIENT) * density;
        it->timeStamp = Simulator::Now ();
        return;
      }
    }
    DensityEstimation item;
    item.vehicleId = vehicleId;
    item.edgeId = edgeId;
    item.density = density;
    item.timeStamp = Simulator::Now ();
    m_densityEstimationVector.push_back (item);
  }

  void SignalMap::UpdateExclusionRegion (uint16_t sender, uint16_t receiver, double exclusionRegion)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (it->from == sender && it->to == receiver)
      {
        it->exclusionRegion = exclusionRegion;
        break;
      }
    }
  }
}
