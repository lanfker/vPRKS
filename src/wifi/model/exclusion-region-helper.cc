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
      uint16_t sender, uint16_t receiver, double txPower)
  {

    txPower = txPower + TX_GAIN;
    for (std::vector<LinkExclusionRegion>::iterator it = m_exclusionRegionCollection.begin (); 
        it != m_exclusionRegionCollection.end (); ++ it)
    {
      if ( it->sender == sender && it->receiver == receiver)
      {
        if (deltaInterference < 0)
        {
          signalMap.SortAccordingToAttenuation ();
          uint32_t offset = 0;
          for (SignalMap::Iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            if ( interferenceW <= it->currentExclusionRegion) //find edge.
            {
              offset = _it - signalMap.begin ();
              break;
            }
          }

          double interferenceSum = 0;
          for (SignalMap::Iterator _it = signalMap.begin () + offset; _it != signalMap.end (); ++ _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            interferenceSum += interferenceW;
            if (interferenceSum >= fabs (deltaInterference))
            {
              it->currentExclusionRegion = interferenceW;
              return interferenceW;
            }
          }
          // if we used all the signal map records, yet we still cannot satisfy the delta interference requirement,
          // we use the last item in the signal map as  current exlusion region
          it -> currentExclusionRegion = DbmToW ( txPower - (signalMap.end () - 1) -> attenuation );
          return it->currentExclusionRegion;
        }
        else if (deltaInterference > 0)
        {
          signalMap.SortAccordingToAttenuation ();
          uint32_t offset = 0;
          for (SignalMap::Iterator _it = signalMap.end () -1; _it != signalMap.begin () - 1; -- _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            if ( interferenceW >= it->currentExclusionRegion)
            {
              offset = signalMap.end () - _it;
              break;
            }
          }

          double interferenceSum = 0;
          for (SignalMap::Iterator _it = signalMap.end () - offset; _it != signalMap.end () - 1; -- _it)
          {
            double interferenceW = DbmToW (txPower - _it->attenuation );
            interferenceSum += interferenceW;
            if (interferenceSum > deltaInterference)
            {
              if ( signalMap.end () - _it > 1)
              {
                it->currentExclusionRegion = DbmToW (txPower - (_it-1)->attenuation );
                return it->currentExclusionRegion;
              }
              else
              {
                it->currentExclusionRegion = interferenceW;
                return it->currentExclusionRegion;
              }
              //it->currentExclusionRegion = 
              // reverse 1
            }
            else if (interferenceSum == deltaInterference )
            {
              it->currentExclusionRegion = interferenceW;
              return it->currentExclusionRegion;
              // do not need reverse
            }
          }
          // if we used all the signal map records, yet we still cannot satisfy the delta interference requirement,
          // we use the first item in the signal map as  current exlusion region in this case
        }
        else if (deltaInterference == 0)
        {
          return it->currentExclusionRegion;
        }
      }
      return 0;
    }
    LinkExclusionRegion linkExclusionRegion;
    linkExclusionRegion.sender = sender;
    linkExclusionRegion.receiver = receiver;
    linkExclusionRegion.currentExclusionRegion = 2.01237e-13; //watt  Default exclusion region, will change later 
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
