
#ifndef DOUBLE_REGRESSION_H 
#define DOUBLE_REGRESSION_H

/* There will be M reference links for us to use to predict the signal power attenuation from S to R. 
 */

#include "ns3/core-module.h"
#include "matrix.h"
#include "observation.h"
#include "exclusion-region-helper.h"


namespace ns3{



  class DoubleRegression : public Object
  {
    public:
      static TypeId GetTypeId(void);
      DoubleRegression ();
      ~DoubleRegression ();
      void Initialize (Observation obs, Matrix &phi, Matrix &pathLoss);
      void Initialize (std::vector<ObservationItem> vec, Matrix &phi, Matrix &pathLoss, bool senderCoordinates);
      void Initialize (std::vector<ParameterObservation> vec, Matrix &phi, Matrix &pathLoss, bool senderCoordinates);
      bool GetCoefficientBeta (Matrix &betaMatrix, Matrix &phi, Matrix &pathLoss);
      double AttenuationEstimation (uint16_t sender, uint16_t receiver, double senderX, double senderY, double receiverX, double receiverY, Observation obs);
      double ParameterEstimation (uint16_t sender, uint16_t receiver, double senderX, double senderY, double receiverX, double receiverY, ExclusionRegionHelper ers);
  double DbmToW (double dBm);

  double WToDbm (double w);
    private:
  };
}


#endif //DOUBLE_REGRESSION_H
