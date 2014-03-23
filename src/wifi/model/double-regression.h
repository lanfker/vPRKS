
#ifndef DOUBLE_REGRESSION_H 
#define DOUBLE_REGRESSION_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
 */

#include "ns3/core-module.h"
#include "matrix.h"
#include "observation.h"


namespace ns3{



  class DoubleRegression : public Object
  {
    public:
      static TypeId GetTypeId(void);
      DoubleRegression ();
      ~DoubleRegression ();
      void Initialize (Observation obs);
      void GetCoefficientBeta (Matrix &betaMatrix);
    private:
      uint32_t m_referenceLinkCount;
      Matrix m_phi;
      Matrix m_pathloss;
  };
}


#endif //DOUBLE_REGRESSION_H
