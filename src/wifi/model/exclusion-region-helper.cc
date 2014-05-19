#include "exclusion-region-helper.h"
#include "observation.h"
#include "ns3/log.h"
#include "matrix.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include "signal-map.h"
#include "ns3/settings.h"

NS_LOG_COMPONENT_DEFINE ("ExclusionRegionHelper");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (ExclusionRegionHelper);

  TypeId ExclusionRegionHelper::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::ExclusionRegionHelper")
      .SetParent<Object> ()
      .AddConstructor<ExclusionRegionHelper> ();
    return tid;
  }
  ExclusionRegionHelper::ExclusionRegionHelper ()
  {
  }


  ExclusionRegionHelper::~ExclusionRegionHelper ()
  {
  }

  // do we need the return value?
  double ExclusionRegionHelper::AdaptExclusionRegion (SignalMap signalMap,  double deltaInterference,
      LinkExclusionRegion item, double txPower, double backgroundInterferenceW, double ewmaPdr)
  {

    //std::cout<<" m_exclusionRegionCollection.size (): "<< m_exclusionRegionCollection.size ()<< std::endl;
    //signalMap.PrintSignalMap (receiver);
    txPower = txPower + TX_GAIN;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if ( it->sender == item.sender && it->receiver == item.receiver)
      {
        it->version ++;
        //Change deltaInterference(dB) into deltaInterference(Watt)
        //deltaInterference = DbmToW (WToDbm (it->currentExclusionRegion) + deltaInterference) - it->currentExclusionRegion;
        deltaInterference = DbmToW (WToDbm (backgroundInterferenceW) + deltaInterference) - backgroundInterferenceW;
        //std::cout<<" delta interference: "<< deltaInterference<<" backgroundInterferenceW: "<< backgroundInterferenceW << std::endl;
        if (deltaInterference < 0) // expand exclusion region
        {

          std::cout<<" expand exclusion region deltainterference: "<< deltaInterference<< std::endl;
          signalMap.SortAccordingToAttenuation ();
          uint32_t offset = 0;
          for (SignalMap::Iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            //std::cout<<" currentExclusionRegion: "<< it->currentExclusionRegion << " interferenceW: " << interferenceW << " atten: "<< _it->attenuation << std::endl;
            offset = _it - signalMap.begin ();
            if ( interferenceW <= it->currentExclusionRegion) //find edge.
            {
              std::cout<<" find edge, interferencew: "<< interferenceW <<" current exclusion region: "<< it->currentExclusionRegion<< std::endl;
              break;
            }
          }

          /*
          if ( sender == 6 && receiver == 5)
            std::cout<<" offset: "<< offset <<" collectionsize: "<< signalMap.GetSignalMap ().size ()<< std::endl;
            */

          double interferenceSum = 0;
          for (SignalMap::Iterator _it = signalMap.begin () + offset + 1; _it != signalMap.end (); ++ _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            interferenceSum += interferenceW;
            if (interferenceSum >= fabs (deltaInterference))
            {
              std::cout<<" sum: "<< interferenceSum <<" deltaInterference: "<< fabs (deltaInterference) <<" target: sum interference greater than delta, then we can stop, current InterferenceW: " << interferenceW << std::endl;

              it->currentExclusionRegion = interferenceW;
              return interferenceW;
            }
          }
          // if we used all the signal map records, yet we still cannot satisfy the delta interference requirement,
          // we use the last item in the signal map as  current exlusion region
          double lastElementInterferenceW = DbmToW ( txPower - (signalMap.end () - 1) -> attenuation );
          if ( lastElementInterferenceW < it->currentExclusionRegion)
          {
            std::cout<<" used all, but cannot satisfy deltainterference requirement "<< std::endl;

            it -> currentExclusionRegion = lastElementInterferenceW;
          }
          return it->currentExclusionRegion;
        }
        else if (deltaInterference > 0) // shrink exclusion region
        {
          signalMap.SortAccordingToAttenuation ();
          uint32_t offset = 0;
          for (SignalMap::Iterator _it = signalMap.end () -1; _it != signalMap.begin () - 1; -- _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            offset = signalMap.end () - _it;
            if ( interferenceW >= it->currentExclusionRegion)
            {
              offset ++;
              std::cout<<" shrink, exclusion region, edge found  interferenceW: " << interferenceW <<" current exclusion region: "<< it->currentExclusionRegion << std::endl;
              break;
            }
          }

          double interferenceSum = 0;
          // when the last signal map record introduce larger interference than the exclusion region, the offset is 1. 
          // we set the offset as 1 so that in the for loop, we can simply substract offset.
          // again, if the offset is one, the for loop will not execute.
          for (SignalMap::Iterator _it = signalMap.end () - offset; _it != signalMap.begin () - 1; -- _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            interferenceSum += interferenceW;
            //std::cout<<" offset: "<< offset <<" it-begin :"<< _it - signalMap.begin ()<<" deltainterference: "<< deltaInterference <<" sum: "<< interferenceSum<< std::endl;
            if (interferenceSum > deltaInterference)
            {
              //std::cout<<" shrink, interference sum greater than delta interference" << std::endl;
              std::cout<<" sum: "<< interferenceSum <<" deltaInterference: "<< fabs (deltaInterference) <<" target: sum interference greater than delta, then we can stop" ;
              
              if ( signalMap.end () - _it > 1 && ewmaPdr - DESIRED_PDR < EWMA_PDR_PROTECTION_THRESHOLD)
              {
                it->currentExclusionRegion = DbmToW (txPower - (_it+1)->attenuation );
                //std::cout<<" txpower: "<< txPower <<" _it-1->from "<< (_it - 1)->from <<" atten: "<< (_it-1)->attenuation <<" er: "<< it->currentExclusionRegion << std::endl;
                std::cout<<" current InterferenceW: "<< DbmToW (txPower - (_it+1)->attenuation ) << std::endl;

                return it->currentExclusionRegion;
              }
              else
              {
                it->currentExclusionRegion = interferenceW;
                std::cout<<" current InterferenceW: "<< interferenceW << std::endl;
                return it->currentExclusionRegion;
              }
              //it->currentExclusionRegion = 
              // reverse 1
            }
            else if (interferenceSum == deltaInterference )
            {
              std::cout<<" shrink, interference sum equals to delta interference" << std::endl;
              it->currentExclusionRegion = interferenceW;
              return it->currentExclusionRegion;
              // do not need reverse
            }
          }
          //If we cannot locate the last exclusion region, we use the last signal map record as the exclusion region.
          //This is valid since our protocol requires us to shrink the exclusion region.
          if ( signalMap.GetSignalMap ().size () != 0)
          {
            double interferenceW = DbmToW (txPower - (signalMap.end ()-1)->attenuation);
            if ( interferenceW > it->currentExclusionRegion) // if the last item is greater than the exclusion region, this means the exclusion region is too large
              it->currentExclusionRegion = interferenceW;
            interferenceW = DbmToW (txPower - signalMap.begin ()->attenuation); 
            if ( interferenceW < it->currentExclusionRegion) // if the first item is smaller than the exclusion region,this means the exclusion region is too small.
              it->currentExclusionRegion = interferenceW;
            return it->currentExclusionRegion;
          }

          // if we used all the signal map records, yet we still cannot satisfy the delta interference requirement,
          // we use the first item in the signal map as  current exlusion region in this case
        }
        else if (deltaInterference == 0) //keep unchanged.
        {
          std::cout<<" delta Interference equals to zero, does not have to change exclusion region. " << std::endl;
          return it->currentExclusionRegion;
        }
      }
      //return it->currentExclusionRegion;
    }
    m_exclusionRegionCollection.push_back (item);
    return item.currentExclusionRegion;
    /*
    signalMap.SortAccordingToAttenuation ();
    for (SignalMap::Iterator it = signalMap.begin (); it != signalMap.end (); ++ it)
    {
      if (deltaInterference < 0) // expand the exclusion region
      {
      }
      else if (deltaInterference > 0) // shrink the exclusion region
      {

      }
    }
    */
  }
  double ExclusionRegionHelper::DbmToW (double dBm)
  {
    double mW = pow (10.0, dBm/10.0);
    return mW / 1000.0;
  }

  double ExclusionRegionHelper::WToDbm (double w)
  {
    return 10.0 * log10 (w * 1000.0);
  }

  void ExclusionRegionHelper::AddOrUpdateExclusionRegion (LinkExclusionRegion item)
  {

    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if ( it->sender == item.sender && it->receiver == item.receiver)
      {
        // first condition, normal comparison without overflow
        // second condition, overflow, it->version should be greater than 200, while version should be close to 0.
        if ( it->version < item.version || it->version - item.version > 100)
        {
          item.distance = it->distance;
          m_exclusionRegionCollection.erase (it);
          break;
        }
        else
        {
          return;
        }
      }
    }
    //no record exist regarding this link (sender ==> receiver);
    m_exclusionRegionCollection.insert ( m_exclusionRegionCollection.begin (), item);
    return;
  }

  std::vector<LinkExclusionRegion> ExclusionRegionHelper::GetLatestUpdatedItems (uint32_t count, SignalMap signalMap)
  {
    std::vector<LinkExclusionRegion> vec;
    uint32_t i = 0;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      for (SignalMap::Iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
      {
        //Check signal map, if the current node is in the receiver's exclusion region, the current node 
        //forward the receiver's exclusion region information. _it->from should be the receiver in order 
        //to locate the signal map record
        if ( _it->from == it->receiver )
        {
          double interferenceW = DbmToW (DEFAULT_POWER + TX_GAIN - _it->attenuation);
          // interferenceW is the supposed intereference if the current node transmits. if this value is
          // greater  or equal to the receiver's current exclusion region, the current node is in the 
          // receiver's exclusion region.
          if ( interferenceW >= it->currentExclusionRegion)
          {
            // we share such information with others.
            vec.push_back (*it);
            i ++;
            // if the maximum count is reached, break;
            if ( i == count)
            {
              break;
            }
          }
        }
      }

      // again, this is for breaking the loop.
      if ( i == count)
      {
        break;
      }
    }
    return vec;
  }

  double ExclusionRegionHelper::GetExclusionRegion (uint16_t sender, uint16_t receiver)
  {
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if (it->sender == sender && it->receiver == receiver)
      {
        return it->currentExclusionRegion;
      }
    }
    return DEFAULT_EXCLUSION_REGION_WATT;
  }

  LinkExclusionRegion ExclusionRegionHelper::GetExclusionRegionRecord (uint16_t sender, uint16_t receiver)
  {
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if (it->sender == sender && it->receiver == receiver)
      {
        return *it;
      }
    }
    //if not found
    LinkExclusionRegion item;
    item.sender = 0;
    item.receiver = 0;
    return item;
  }

  std::set<uint16_t> ExclusionRegionHelper::GetSenders ()
  {
    std::set<uint16_t> vec;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      vec.insert (it->sender);
    }
    return vec;
  }


  std::vector<ParameterObservation> ExclusionRegionHelper::GetObservationsBySender (uint16_t sender, double senderX, double senderY, double receiverX, double receiverY)
  {
    std::vector<ParameterObservation> vec;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      double x = 0, y = 0;
      if ( it->sender == sender)
      {
        x = it->senderX;
        y = it->senderY;
      }

      for (std::vector<LinkExclusionRegion>::iterator _it = m_exclusionRegionCollection.begin (); 
          _it != m_exclusionRegionCollection.end (); ++ _it)
      {
        if ( _it->sender == sender)
        {
          double senderXDifference = x - _it->senderX;
          double senderYDifference = y - _it->senderY;
          double dt = sqrt ( pow (senderX - _it->senderX, 2) + pow (senderY - _it->senderY, 2));
          double dr = sqrt ( pow (receiverX - _it->receiverX, 2) + pow (receiverY - _it->receiverY, 2));
          double dist = sqrt (dt*dt + dr*dr);
          if ( dist <= LINK_DISTANCE_THRESHOLD)
          {
            ParameterObservation obs;
            obs.sender = _it->sender;
            obs.receiver = _it->receiver;
            obs.parameter = _it->currentExclusionRegion;
            obs.senderX = _it->senderX + senderXDifference;
            obs.senderY = _it->senderY + senderYDifference;
            obs.receiverX = _it->receiverX + senderXDifference;
            obs.receiverY = _it->receiverY + senderYDifference;
            vec.push_back (obs);
          }
        }
      }
      if ( vec.size () >= 3)
        return vec;
      else
      {
        vec.clear ();
        continue;
      }
    }
    return vec;
  }
}
