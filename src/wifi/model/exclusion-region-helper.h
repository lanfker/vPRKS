#ifndef EXCLUSION_REGION_HELPER_H 
#define EXCLUSION_REGION_HELPER_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
*/

#include "ns3/core-module.h"
#include "signal-map.h"


namespace ns3{


  class ExclusionRegionHelper : public Object
  {
    public:
      static TypeId GetTypeId(void);
      ExclusionRegionHelper ();
      ~ExclusionRegionHelper ();
      double AdaptExclusionRegion (SignalMap signalMap, double currentExlusionRegion, double deltaInterference);

    private:
  };
}


#endif //EXCLUSION_REGION_HELPER_H
