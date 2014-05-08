#include "exclusion-region-helper.h"
#include "observation.h"
#include "ns3/log.h"
#include "matrix.h"
#include <iostream>
#include <algorithm>
#include <cmath>
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
      uint16_t sender, uint16_t receiver, double txPower, double backgroundInterferenceW)
  {

    //std::cout<<" m_exclusionRegionCollection.size (): "<< m_exclusionRegionCollection.size ()<< std::endl;
    //signalMap.PrintSignalMap (receiver);
    txPower = txPower + TX_GAIN;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if ( it->sender == sender && it->receiver == receiver)
      {
        //Change deltaInterference(dB) into deltaInterference(Watt)
        //deltaInterference = DbmToW (WToDbm (it->currentExclusionRegion) + deltaInterference) - it->currentExclusionRegion;
        deltaInterference = DbmToW (WToDbm (backgroundInterferenceW) + deltaInterference) - it->currentExclusionRegion;
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
              
              if ( signalMap.end () - _it > 1)
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
    LinkExclusionRegion linkExclusionRegion;
    linkExclusionRegion.sender = sender;
    linkExclusionRegion.receiver = receiver;
    linkExclusionRegion.currentExclusionRegion = DEFAULT_EXCLUSION_REGION_WATT; //watt  Default exclusion region, will change later 
    m_exclusionRegionCollection.push_back (linkExclusionRegion);
    return linkExclusionRegion.currentExclusionRegion;
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

}
