#include "observation.h"
#include "ns3/log.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("Observation");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (Observation);

  TypeId Observation::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::Observation")
      .SetParent<Object> ()
      .AddConstructor<Observation> ();
    return tid;
  }
  Observation::Observation ()
  {
  }


  Observation::~Observation ()
  {
  }
  void Observation::AppendObservation (uint16_t sender, uint16_t receiver, ObservationItem obs)
  {
    for (std::vector<LinkObservations>::iterator it = m_observations.begin (); it != m_observations.end ();
        ++ it)
    {
      if ( it->sender == sender && it->receiver == receiver)
      {
        it->observations.insert ( it->observations.begin (),obs); // the latest observation should be inserted in the front
        return;
      }
    }
    // not found in existed record, then we create a new one for this sender-receiver pair
    LinkObservations linkObservations;
    linkObservations.sender = sender;
    linkObservations.receiver = receiver;
    // the latest observation should be inserted in the front
    linkObservations.observations.insert (linkObservations.observations.begin (), obs);
    m_observations.push_back (linkObservations);
  }

  uint32_t Observation::FindMinimumObservationLength ()
  {
    uint32_t min = 10000;
    for (std::vector<LinkObservations>::iterator it = m_observations.begin (); it != m_observations.end ();
        ++ it)
    {
      if (it->observations.size () < min)
      {
        min = it->observations.size ();
      }
    }
    
    return min;
  }

  uint32_t Observation::FindLinkCount ()
  {
    return m_observations.size ();
  }
}
