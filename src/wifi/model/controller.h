/*
 * Created by Chuan
 */
#ifndef PRK_CONTROLLER_H
#define PRK_CONTROLLER_H
#include "ns3/object.h"
#include "math-helper.h"
namespace ns3
{
  const uint32_t ARRAY_LENGTH = 446;
  class Controller : public Object
  {
    public:
      static TypeId GetTypeId (void);
      Controller ();
      ~Controller ();

      static const double m_pdrToSnr[ARRAY_LENGTH][2];
      virtual double ComputeSlope (double currentPdr);
      virtual double GetSnrByPdr (double pdr);
    private:

  };
  class PControllerWithReferenceInterference : public Controller
  {
    public:
      static TypeId GetTypeId (void);
      PControllerWithReferenceInterference ();
      ~PControllerWithReferenceInterference ();

      double GetDeltaInterference(double rxPowerDbm, double expectedPdr, double currentNplusIDbm);
    private:
  };

  class PControllerWithDesiredPdr : public Controller
  {
    public:
      static TypeId GetTypeId (void);
      PControllerWithDesiredPdr ();
      ~PControllerWithDesiredPdr ();

      /*
       * \param desiredPdr The desired link reliability of the current link.
       * \param current estimated expected link reliability of the current link
       *
       * This method use the P-Controller to control the delta Interference a node should remove
       * More detailly, the controller says as follows: deltaI = Kp * (Ts.r - Ys.r(t)) where Kp = (-1)/a(t)
       * and the Ts.r is the minimun link reliability of the link from s to r assuming s and r represent two nodes
       */
      double GetDeltaInterference(double desiredPdr, double currentPdr);

    private:
      double m_deltaY;
      double m_ewmaCoefficient;
      double m_E0;
  };
  class MinimumVarianceController : public Controller
  {
    public:
      static TypeId GetTypeId (void);
      MinimumVarianceController();
      ~MinimumVarianceController();
      double GetDeltaInterference (double desiredPdr, double ewmaCurrentPdr, double estimatedCurrentPdr, bool &conditionTwoMeet);
    private:
      double m_deltaY;
      double m_ewmaCoefficient;
      double m_E0;
  };
} // namespace ns3
#endif //PRK_CONTROLLER_H
