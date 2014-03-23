#include "signal-map.h"
#include "ns3/log.h"
#include <iostream>
#include <algorithm>

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
        it->inComingAttenuation = item.inComingAttenuation;
        it->outGoingAttenuation = item.outGoingAttenuation;
        it->timeStamp = item.timeStamp;
        return;
      }
    }
    // if no such from-to pair exist, create a new entry
    m_signalMap.push_back (item);
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

  void SignalMap::SortAccordingToInComingAttenuation ()
  {
    sort (m_signalMap.begin (), m_signalMap.end (), InComingAttenCompare);
  }


  //duration is the expiration time
  void SignalMap::RemoveExpiredItem (Time duration)
  {
    uint32_t removeIndex = m_signalMap.size ();
    for (uint32_t i = 0; i < m_signalMap.size (); ++ i)
    {
      if ( m_signalMap[i].timeStamp + duration < Simulator::Now ()) // expired
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
}
