#include "link-estimator.h"
#include "observation.h"
#include "ns3/log.h"
#include "matrix.h"
#include <iostream>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("LinkEstimator");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (LinkEstimator);

  TypeId LinkEstimator::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::LinkEstimator")
      .SetParent<Object> ()
      .AddConstructor<LinkEstimator> ();
    return tid;
  }
  LinkEstimator::LinkEstimator ()
  {
    m_coefficient = 0.8;
    m_maxExpireTime = Seconds (100.0);
  }


  LinkEstimator::~LinkEstimator ()
  {
  }

  /* After calling method, we should call method to check if PDR is updated.
   */
  void LinkEstimator::AddSequenceNumber (uint32_t seq, uint16_t sender, uint16_t receiver, Time timeStamp)
  {
    for (std::vector<LinkEstimationItem>::iterator it = m_estimations.begin (); it != m_estimations.end (); ++ it)
    {
      if (it->sender == sender && it->receiver == receiver)
      {
        // if it has been too long since the last time this link receives a packet, erase all sequence numbers.
        if ( timeStamp - it->timeStamp > m_maxExpireTime)
        {
          it->receivedSequenceNumbers.clear ();
        }
        it->timeStamp = timeStamp;
        InsertSeqNumber (it->receivedSequenceNumbers, seq);
        return;
      }
    }
    // if no entry found
    LinkEstimationItem item;
    item.sender = sender;
    item.receiver = receiver;
    item.timeStamp = timeStamp;
    item.instantPdr = 0;
    item.ewmaPdr = 0;
    item.receivedSequenceNumbers.push_back (seq);
    m_estimations.push_back (item);

  }

  void LinkEstimator::InsertSeqNumber (std::vector<uint32_t> &vec, uint32_t seq)
  {
    std::sort (vec.begin (), vec.end ()); // use < as default comparator.
    for (std::vector<uint32_t>::iterator it = vec.begin (); it != vec.end (); ++ it)
    {
      if (seq == *it)
      {
        return;
      }
      if (seq< *it)
      {
        vec.insert (it, seq);
        return;
      }
    }
    vec.push_back (seq);
  }

  bool LinkEstimator::IsPdrUpdated (uint16_t sender, uint16_t receiver, uint32_t window)
  {
    for (std::vector<LinkEstimationItem>::iterator it = m_estimations.begin (); it != m_estimations.end (); ++ it)
    {
      if (it->sender == sender && it->receiver == receiver)
      {

        uint32_t _first = it->receivedSequenceNumbers[0];
        uint32_t _last = it->receivedSequenceNumbers[ it->receivedSequenceNumbers.size () - 1];
        if (_last - _first > 2000 ) //4096 overflow
        {
          for (std::vector<uint32_t>::iterator _it = it->receivedSequenceNumbers.begin (); 
              _it != it->receivedSequenceNumbers.end (); ++ _it)
          {
            if ( *_it < 2000)
            {
              *_it = *_it + 4096;
            }
          }
        }
        sort (it->receivedSequenceNumbers.begin (), it->receivedSequenceNumbers.end ());
        if (_last - _first + 1 >= window)
        {
          it->instantPdr = (double)(it->receivedSequenceNumbers.size ()) / (_last - _first + 1);
          it->ewmaPdr = (1-m_coefficient) * it->ewmaPdr + m_coefficient * it->instantPdr;
          it->receivedSequenceNumbers.clear ();
          return true;
        }
        else
        {
          return false;
        }
      }
    }
  }

  LinkEstimationItem LinkEstimator::GetLinkEstimationItem (uint16_t sender, uint16_t receiver)
  {
    for (std::vector<LinkEstimationItem>::iterator it = m_estimations.begin (); it != m_estimations.end (); ++ it)
    {
      if ( it->sender == sender && it->receiver == receiver)
      {
        return *it;
      }
    }
  }
}
