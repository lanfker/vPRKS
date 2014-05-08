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
    m_coefficient = EWMA_COEFFICIENT;
    m_maxExpireTime = Seconds (OBSERVATION_EXPIRATION_TIME);
  }


  LinkEstimator::~LinkEstimator ()
  {
  }

  /* After calling method, we should call method to check if PDR is updated.
   */
  void LinkEstimator::AddSequenceNumber (uint32_t seq, uint16_t sender, uint16_t receiver, Time timeStamp)
  {
    //std::cout<<" m_estimations.size (): "<< m_estimations.size () << std::endl;
    //std::cout<<" seq: "<< seq <<" seq % 4096: "<< seq % 4096 << std::endl;
    seq = seq % 4096;
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
    item.estimationCount = 0;
    item.receivedSequenceNumbers.push_back (seq);
    m_estimations.push_back (item);

  }

  void LinkEstimator::PrintSeqNumbers (std::vector<uint32_t> &vec)
  {
    std::cout<<" print out received sequence number" << std::endl;
    for (std::vector<uint32_t>::iterator it = vec.begin (); it != vec.end (); ++ it)
    {
      std::cout<<*it<<" ";
    }
    std::cout<<std::endl;
  }
  void LinkEstimator::InsertSeqNumber (std::vector<uint32_t> &vec, uint32_t seq)
  {
    //std::sort (vec.begin (), vec.end ()); // use < as default comparator.
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

  //over flow taken care of.
  bool LinkEstimator::IsPdrUpdated (uint16_t sender, uint16_t receiver, uint32_t window)
  {
    for (std::vector<LinkEstimationItem>::iterator it = m_estimations.begin (); it != m_estimations.end (); ++ it)
    {
      if (it->sender == sender && it->receiver == receiver)
      {

        uint32_t _first = it->receivedSequenceNumbers[0];
        uint32_t _last = it->receivedSequenceNumbers[it->receivedSequenceNumbers.size () - 1];
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
        _first = it->receivedSequenceNumbers[0];
        _last = it->receivedSequenceNumbers[it->receivedSequenceNumbers.size () - 1];
        /*
        if ( sender == 6 && receiver == 5)
        {
          PrintSeqNumbers (it->receivedSequenceNumbers);
          std::cout<<"_last: "<< _last <<" _first: "<< _first << std::endl;
        }
        */
        if (_last - _first + 1 >= window)
        {
          it->instantPdr = (double)(it->receivedSequenceNumbers.size ()) / (_last - _first + 1);
          it->ewmaPdr = (m_coefficient) * it->ewmaPdr + (1- m_coefficient) * it->instantPdr;
          /*
          if ( sender == 15)
          {
            std::cout<<"sender: "<< sender <<" receiver: "<< receiver <<" instantPdr: "<< it->instantPdr <<" ewmaPdr: "<< it->ewmaPdr << std::endl;
          }
          */
          it->receivedSequenceNumbers.clear ();
          it->estimationCount ++;
          return true;
        }
        else
        {
          return false;
        }
      }
    }
    return false;
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
    LinkEstimationItem item;
    item.sender = 0;
    item.receiver = 0;
    return item;
  }

  void LinkEstimator::ClearSequenceNumbers (uint16_t sender, uint16_t receiver)
  {
    for (std::vector<LinkEstimationItem>::iterator it = m_estimations.begin (); it != m_estimations.end (); ++ it)
    {
      if ( it->sender == sender && it->receiver == receiver)
      {
        it->receivedSequenceNumbers.clear ();
        return;
      }
    }
  }
}
