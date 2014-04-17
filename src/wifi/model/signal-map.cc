#include "signal-map.h"
#include "ns3/log.h"
#include <iostream>
#include <algorithm>
#include <set>

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
        it->begin = item.begin;
        it->end = item.end;
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
        <<" angle: "<< it->angle <<" begin: "<< it->begin <<" end: "<< it->end<<" exclusionRegion: "<< it->exclusionRegion << std::endl;
    }
    std::cout<<"--------------------------------------------------"<< std::endl;
  }

  uint32_t SignalMap::GetSize ()
  {
    return m_signalMap.size ();
  }

  DirectionDistribution SignalMap::GetDirectionDistribution (double angle)
  {
    DirectionDistribution directionDistribution;
    directionDistribution.ratio[0] = 0;
    directionDistribution.ratio[1] = 0;
    directionDistribution.ratio[2] = 0;
    directionDistribution.ratio[3] = 0;
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if ( it->angle < -90 && it->angle >= -180)
      {
        directionDistribution.ratio[2] += 1;
      }
      else if (it->angle >= -90 && it->angle < 0)
      {
        directionDistribution.ratio[3] += 1;
      }
      if (it->angle > 90 && it->angle >= 180)
      {
        directionDistribution.ratio[1] += 1;
      }
      else if (it->angle <= 90 && it->angle >= 0)
      {
        directionDistribution.ratio[0] += 1;
      }
    }
    double signalMapSize = GetSize ();

    directionDistribution.ratio[0] /= signalMapSize;
    directionDistribution.ratio[1] /= signalMapSize;
    directionDistribution.ratio[2] /= signalMapSize;
    directionDistribution.ratio[3] /= signalMapSize;
    if ( angle >= 0 && angle <= 90)
    {
      directionDistribution.selfSector = 0;
    }
    else if ( angle > 90 && angle <= 180)
    {
      directionDistribution.selfSector = 1;
    }
    else if ( angle >= -90 && angle < 0)
    {
      directionDistribution.selfSector = 3;   
    }
    else if (angle < -90 && angle >= -180)
    {
      directionDistribution.selfSector = 2;
    }
    //std::cout<<" angle: "<< angle <<" sector: "<< directionDistribution.selfSector << std::endl;
    return directionDistribution;
  }

  void SignalMap::GetConflictNodes (uint16_t slot, std::vector<uint16_t> &vec)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if ( it->begin <= slot && it->end >= slot)
      {
        vec.push_back (it->from);
      }
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

  void SignalMap::UpdateVehicleStatus (uint16_t from, double angle, uint16_t begin, uint16_t end)
  {
    for (std::vector<SignalMapItem>::iterator it = m_signalMap.begin (); it != m_signalMap.end (); ++ it)
    {
      if (it->from == from)
      {
        it->angle = angle;
        it->begin = begin;
        it->end = end;
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

}
