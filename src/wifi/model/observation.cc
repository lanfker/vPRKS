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

  void Observation::PrintObservations ()
  {
    for (std::vector<LinkObservations>::iterator it = m_observations.begin (); it != m_observations.end ();
        ++ it)
    {
      std::cout<<"For sender: "<< it->sender << " --> "<< it->receiver << std::endl;
      for (std::vector< ObservationItem >::iterator sub_it = it->observations.begin ();
          sub_it != it->observations.end (); ++ sub_it)
      {
        std::cout<<"\t\t\t" <<" sender: ("<<sub_it->senderX<<", "<<sub_it->senderY<<") receiver: ("<<sub_it->receiverX<<", "<<
          sub_it->receiverY <<") avg_atten: "<<sub_it->averageAttenuation <<" timestamp: "<< sub_it->timeStamp << std::endl;
      }
    }
  }

  void Observation::RemoveExpireItems (Time duration, uint32_t maxCount)
  {
    for (std::vector<LinkObservations>::iterator it = m_observations.begin (); it != m_observations.end ();
        ++ it)
    {
      for (std::vector< ObservationItem >::iterator sub_it = it->observations.begin ();
          sub_it != it->observations.end (); ++ sub_it)
      {
        //std::cout<<"timestamp: "<< sub_it->timeStamp <<" duration: "<< duration << " now: "<< Simulator::Now () ;
        if (sub_it->timeStamp < Simulator::Now () - duration )
        {
          it->observations.erase (sub_it, it->observations.end ());
          break;
        }
        if (sub_it - it->observations.begin () > maxCount - 1)
        {
          it->observations.erase (sub_it, it->observations.end ());
          break;
        }
      }
    }
  }
}
