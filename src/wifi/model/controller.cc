/*
* created by Chuan
* this fine mainly implements various controllers for the PRK model.
*
*/


#include "controller.h"
#include "ns3/log.h"
#include "math.h"

NS_LOG_COMPONENT_DEFINE ("Controller");
namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (Controller);
  TypeId Controller::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::Controller")
      .SetParent<Object> ()
      .AddConstructor <Controller> ()
      ;
    return tid;
  }
  Controller::Controller ()
  {
    NS_LOG_FUNCTION (this);
  }
  Controller::~Controller ()
  {
    NS_LOG_FUNCTION (this);
  }
  const double Controller::m_pdrToSnr[446][2] = {
    {5.07629,1.11022e-16},
    {5.08629,3.33067e-16},
    {5.09629,1.44329e-15},
    {5.10629,4.996e-15},
    {5.11629,1.66533e-14},
    {5.12629,5.36238e-14},
    {5.13629,1.64868e-13},
    {5.14629,4.86056e-13},
    {5.15629,1.37534e-12},
    {5.16629,3.74178e-12},
    {5.17629,9.80171e-12},
    {5.18629,2.47578e-11},
    {5.19629,6.03851e-11},
    {5.20629,1.42411e-10},
    {5.21629,3.25178e-10},
    {5.22629,7.19789e-10},
    {5.23629,1.54639e-09},
    {5.24629,3.22821e-09},
    {5.25629,6.55569e-09},
    {5.26629,1.29643e-08},
    {5.27629,2.49921e-08},
    {5.28629,4.70115e-08},
    {5.29629,8.63701e-08},
    {5.30629,1.55123e-07},
    {5.31629,2.72595e-07},
    {5.32629,4.6909e-07},
    {5.33629,7.91112e-07},
    {5.34629,1.30858e-06},
    {5.35629,2.12455e-06},
    {5.36629,3.38801e-06},
    {5.37629,5.31048e-06},
    {5.38629,8.18689e-06},
    {5.39629,1.24216e-05},
    {5.40629,1.85596e-05},
    {5.41629,2.73244e-05},
    {5.42629,3.96613e-05},
    {5.43629,5.67872e-05},
    {5.44629,8.02465e-05},
    {5.45629,0.000111972},
    {5.46629,0.000154352},
    {5.47629,0.000210297},
    {5.48629,0.000283309},
    {5.49629,0.000377557},
    {5.50629,0.000497938},
    {5.51629,0.000650143},
    {5.52629,0.000840714},
    {5.53629,0.00107709},
    {5.54629,0.00136762},
    {5.55629,0.00172164},
    {5.56629,0.0021494},
    {5.57629,0.00266212},
    {5.58629,0.00327191},
    {5.59629,0.00399174},
    {5.60629,0.00483538},
    {5.61629,0.0058173},
    {5.62629,0.00695255},
    {5.63629,0.0082567},
    {5.64629,0.00974563},
    {5.65629,0.0114354},
    {5.66629,0.0133423},
    {5.67629,0.0154821},
    {5.68629,0.0178708},
    {5.69629,0.0205236},
    {5.70629,0.0234552},
    {5.71629,0.0266797},
    {5.72629,0.0302101},
    {5.73629,0.0340584},
    {5.74629,0.0382356},
    {5.75629,0.0427512},
    {5.76629,0.0476135},
    {5.77629,0.0528294},
    {5.78629,0.0584041},
    {5.79629,0.0643414},
    {5.80629,0.0706436},
    {5.81629,0.0773111},
    {5.82629,0.0843431},
    {5.83629,0.091737},
    {5.84629,0.0994887},
    {5.85629,0.107593},
    {5.86629,0.116042},
    {5.87629,0.124829},
    {5.88629,0.133943},
    {5.89629,0.143373},
    {5.90629,0.153109},
    {5.91629,0.163136},
    {5.92629,0.173443},
    {5.93629,0.184012},
    {5.94629,0.194831},
    {5.95629,0.205883},
    {5.96629,0.217151},
    {5.97629,0.22862},
    {5.98629,0.240271},
    {5.99629,0.252089},
    {6.00629,0.264056},
    {6.01629,0.276155},
    {6.02629,0.288369},
    {6.03629,0.300681},
    {6.04629,0.313074},
    {6.05629,0.325532},
    {6.06629,0.338038},
    {6.07629,0.350578},
    {6.08629,0.363135},
    {6.09629,0.375694},
    {6.10629,0.388243},
    {6.11629,0.400766},
    {6.12629,0.41325},
    {6.13629,0.425684},
    {6.14629,0.438054},
    {6.15629,0.450349},
    {6.16629,0.46256},
    {6.17629,0.474674},
    {6.18629,0.486683},
    {6.19629,0.498578},
    {6.20629,0.510351},
    {6.21629,0.521993},
    {6.22629,0.533497},
    {6.23629,0.544858},
    {6.24629,0.556068},
    {6.25629,0.567122},
    {6.26629,0.578016},
    {6.27629,0.588745},
    {6.28629,0.599304},
    {6.29629,0.609692},
    {6.30629,0.619903},
    {6.31629,0.629937},
    {6.32629,0.63979},
    {6.33629,0.649461},
    {6.34629,0.658949},
    {6.35629,0.668253},
    {6.36629,0.677371},
    {6.37629,0.686304},
    {6.38629,0.695052},
    {6.39629,0.703614},
    {6.40629,0.711992},
    {6.41629,0.720185},
    {6.42629,0.728196},
    {6.43629,0.736025},
    {6.44629,0.743673},
    {6.45629,0.751142},
    {6.46629,0.758434},
    {6.47629,0.765551},
    {6.48629,0.772494},
    {6.49629,0.779266},
    {6.50629,0.785869},
    {6.51629,0.792306},
    {6.52629,0.798578},
    {6.53629,0.804689},
    {6.54629,0.810641},
    {6.55629,0.816437},
    {6.56629,0.822079},
    {6.57629,0.82757},
    {6.58629,0.832913},
    {6.59629,0.83811},
    {6.60629,0.843165},
    {6.61629,0.848081},
    {6.62629,0.85286},
    {6.63629,0.857505},
    {6.64629,0.862019},
    {6.65629,0.866404},
    {6.66629,0.870665},
    {6.67629,0.874803},
    {6.68629,0.878821},
    {6.69629,0.882722},
    {6.70629,0.88651},
    {6.71629,0.890185},
    {6.72629,0.893753},
    {6.73629,0.897214},
    {6.74629,0.900572},
    {6.75629,0.903829},
    {6.76629,0.906988},
    {6.77629,0.910051},
    {6.78629,0.913021},
    {6.79629,0.915901},
    {6.80629,0.918692},
    {6.81629,0.921397},
    {6.82629,0.924019},
    {6.83629,0.926559},
    {6.84629,0.929021},
    {6.85629,0.931405},
    {6.86629,0.933714},
    {6.87629,0.935951},
    {6.88629,0.938117},
    {6.89629,0.940215},
    {6.90629,0.942246},
    {6.91629,0.944212},
    {6.92629,0.946115},
    {6.93629,0.947957},
    {6.94629,0.94974},
    {6.95629,0.951466},
    {6.96629,0.953135},
    {6.97629,0.954751},
    {6.98629,0.956313},
    {6.99629,0.957825},
    {7.00629,0.959288},
    {7.01629,0.960702},
    {7.02629,0.96207},
    {7.03629,0.963393},
    {7.04629,0.964672},
    {7.05629,0.965909},
    {7.06629,0.967104},
    {7.07629,0.96826},
    {7.08629,0.969378},
    {7.09629,0.970458},
    {7.10629,0.971501},
    {7.11629,0.97251},
    {7.12629,0.973485},
    {7.13629,0.974427},
    {7.14629,0.975337},
    {7.15629,0.976216},
    {7.16629,0.977066},
    {7.17629,0.977886},
    {7.18629,0.978679},
    {7.19629,0.979444},
    {7.20629,0.980183},
    {7.21629,0.980897},
    {7.22629,0.981587},
    {7.23629,0.982252},
    {7.24629,0.982895},
    {7.25629,0.983516},
    {7.26629,0.984115},
    {7.27629,0.984693},
    {7.28629,0.985251},
    {7.29629,0.98579},
    {7.30629,0.98631},
    {7.31629,0.986811},
    {7.32629,0.987296},
    {7.33629,0.987763},
    {7.34629,0.988214},
    {7.35629,0.988648},
    {7.36629,0.989068},
    {7.37629,0.989473},
    {7.38629,0.989863},
    {7.39629,0.99024},
    {7.40629,0.990603},
    {7.41629,0.990953},
    {7.42629,0.991291},
    {7.43629,0.991617},
    {7.44629,0.991931},
    {7.45629,0.992233},
    {7.46629,0.992525},
    {7.47629,0.992807},
    {7.48629,0.993078},
    {7.49629,0.99334},
    {7.50629,0.993592},
    {7.51629,0.993834},
    {7.52629,0.994069},
    {7.53629,0.994294},
    {7.54629,0.994511},
    {7.55629,0.994721},
    {7.56629,0.994923},
    {7.57629,0.995117},
    {7.58629,0.995304},
    {7.59629,0.995485},
    {7.60629,0.995659},
    {7.61629,0.995826},
    {7.62629,0.995987},
    {7.63629,0.996142},
    {7.64629,0.996292},
    {7.65629,0.996436},
    {7.66629,0.996574},
    {7.67629,0.996708},
    {7.68629,0.996836},
    {7.69629,0.99696},
    {7.70629,0.997079},
    {7.71629,0.997193},
    {7.72629,0.997304},
    {7.73629,0.99741},
    {7.74629,0.997512},
    {7.75629,0.99761},
    {7.76629,0.997704},
    {7.77629,0.997795},
    {7.78629,0.997883},
    {7.79629,0.997967},
    {7.80629,0.998048},
    {7.81629,0.998126},
    {7.82629,0.998201},
    {7.83629,0.998273},
    {7.84629,0.998342},
    {7.85629,0.998409},
    {7.86629,0.998473},
    {7.87629,0.998534},
    {7.88629,0.998593},
    {7.89629,0.99865},
    {7.90629,0.998705},
    {7.91629,0.998758},
    {7.92629,0.998808},
    {7.93629,0.998857},
    {7.94629,0.998903},
    {7.95629,0.998948},
    {7.96629,0.998991},
    {7.97629,0.999033},
    {7.98629,0.999072},
    {7.99629,0.999111},
    {8.00629,0.999147},
    {8.01629,0.999182},
    {8.02629,0.999216},
    {8.03629,0.999249},
    {8.04629,0.99928},
    {8.05629,0.99931},
    {8.06629,0.999339},
    {8.07629,0.999366},
    {8.08629,0.999393},
    {8.09629,0.999418},
    {8.10629,0.999443},
    {8.11629,0.999466},
    {8.12629,0.999489},
    {8.13629,0.99951},
    {8.14629,0.999531},
    {8.15629,0.999551},
    {8.16629,0.99957},
    {8.17629,0.999588},
    {8.18629,0.999606},
    {8.19629,0.999623},
    {8.20629,0.999639},
    {8.21629,0.999654},
    {8.22629,0.999669},
    {8.23629,0.999683},
    {8.24629,0.999697},
    {8.25629,0.99971},
    {8.26629,0.999723},
    {8.27629,0.999735},
    {8.28629,0.999746},
    {8.29629,0.999757},
    {8.30629,0.999768},
    {8.31629,0.999778},
    {8.32629,0.999788},
    {8.33629,0.999797},
    {8.34629,0.999806},
    {8.35629,0.999814},
    {8.36629,0.999822},
    {8.37629,0.99983},
    {8.38629,0.999838},
    {8.39629,0.999845},
    {8.40629,0.999852},
    {8.41629,0.999858},
    {8.42629,0.999865},
    {8.43629,0.999871},
    {8.44629,0.999877},
    {8.45629,0.999882},
    {8.46629,0.999887},
    {8.47629,0.999892},
    {8.48629,0.999897},
    {8.49629,0.999902},
    {8.50629,0.999906},
    {8.51629,0.999911},
    {8.52629,0.999915},
    {8.53629,0.999919},
    {8.54629,0.999922},
    {8.55629,0.999926},
    {8.56629,0.999929},
    {8.57629,0.999932},
    {8.58629,0.999936},
    {8.59629,0.999938},
    {8.60629,0.999941},
    {8.61629,0.999944},
    {8.62629,0.999947},
    {8.63629,0.999949},
    {8.64629,0.999951},
    {8.65629,0.999954},
    {8.66629,0.999956},
    {8.67629,0.999958},
    {8.68629,0.99996},
    {8.69629,0.999962},
    {8.70629,0.999964},
    {8.71629,0.999965},
    {8.72629,0.999967},
    {8.73629,0.999968},
    {8.74629,0.99997},
    {8.75629,0.999971},
    {8.76629,0.999973},
    {8.77629,0.999974},
    {8.78629,0.999975},
    {8.79629,0.999976},
    {8.80629,0.999978},
    {8.81629,0.999979},
    {8.82629,0.99998},
    {8.83629,0.999981},
    {8.84629,0.999982},
    {8.85629,0.999982},
    {8.86629,0.999983},
    {8.87629,0.999984},
    {8.88629,0.999985},
    {8.89629,0.999986},
    {8.90629,0.999986},
    {8.91629,0.999987},
    {8.92629,0.999988},
    {8.93629,0.999988},
    {8.94629,0.999989},
    {8.95629,0.999989},
    {8.96629,0.99999},
    {8.97629,0.99999},
    {8.98629,0.999991},
    {8.99629,0.999991},
    {9.00629,0.999992},
    {9.01629,0.999992},
    {9.02629,0.999993},
    {9.03629,0.999993},
    {9.04629,0.999993},
    {9.05629,0.999994},
    {9.06629,0.999994},
    {9.07629,0.999994},
    {9.08629,0.999995},
    {9.09629,0.999995},
    {9.10629,0.999995},
    {9.11629,0.999995},
    {9.12629,0.999996},
    {9.13629,0.999996},
    {9.14629,0.999996},
    {9.15629,0.999996},
    {9.16629,0.999996},
    {9.17629,0.999997},
    {9.18629,0.999997},
    {9.19629,0.999997},
    {9.20629,0.999997},
    {9.21629,0.999997},
    {9.22629,0.999997},
    {9.23629,0.999998},
    {9.24629,0.999998},
    {9.25629,0.999998},
    {9.26629,0.999998},
    {9.27629,0.999998},
    {9.28629,0.999998},
    {9.29629,0.999998},
    {9.30629,0.999998},
    {9.31629,0.999998},
    {9.32629,0.999998},
    {9.33629,0.999999},
    {9.34629,0.999999},
    {9.35629,0.999999},
    {9.36629,0.999999},
    {9.37629,0.999999},
    {9.38629,0.999999},
    {9.39629,0.999999},
    {9.40629,0.999999},
    {9.41629,0.999999},
    {9.42629,0.999999},
    {9.43629,0.999999},
    {9.44629,0.999999},
    {9.45629,0.999999},
    {9.46629,0.999999},
    {9.47629,0.999999},
    {9.48629,0.999999},
    {9.49629,0.999999},
    {9.50629,0.999999},
    {9.51629,0.999999},
    {9.52629,1}
  };
  double Controller::ComputeSlope (double currentPdr)
  {
    if (currentPdr < 0.002)
    {
      currentPdr = 0.002;
    }
    uint32_t targetIndex = 0;
    for (uint32_t i = 0; i < 446; ++ i)
    {
      if ( currentPdr >= Controller::m_pdrToSnr[i][1])
      {
        targetIndex = i;
        break;
      }
    }
    double deltaSnr = 0;
    double deltaPdr = 0;
    if ( targetIndex == 0)
    {
      deltaSnr = m_pdrToSnr[targetIndex + 1][0] - m_pdrToSnr[targetIndex][0];
      deltaPdr = m_pdrToSnr[targetIndex + 1][1] - m_pdrToSnr[targetIndex][1];
    }
    if ( targetIndex > 0)
    {
      deltaSnr = m_pdrToSnr[targetIndex][0] - m_pdrToSnr[targetIndex - 1][0];
      deltaPdr = m_pdrToSnr[targetIndex][1] - m_pdrToSnr[targetIndex - 1][1];
    }

    double slope = 0.0;
    slope = deltaPdr/deltaSnr; // the slope at the point PDR=currentPdr in the SNR-to-PDR curve.
    return slope;
  }

  double Controller::GetSnrByPdr (double pdr)
  {
    if (pdr < 0.002)
    {
      pdr = 0.002;
    }

    uint32_t targetIndex = 0;
    for (uint32_t i = 0; i < 446; ++ i)
    {
      if ( pdr >= Controller::m_pdrToSnr[i][1])
      {
        targetIndex = i;
        break;
      }
    }
    return Controller::m_pdrToSnr[targetIndex][0];
  }


  NS_OBJECT_ENSURE_REGISTERED (PControllerWithReferenceInterference);

  TypeId PControllerWithReferenceInterference::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PControllerWithReferenceInterference")
      .SetParent<Controller> ()
      .AddConstructor<PControllerWithReferenceInterference> ()
      ;
    return tid;
  }

  PControllerWithReferenceInterference::PControllerWithReferenceInterference ()
  {
    NS_LOG_FUNCTION (this);
  }

  PControllerWithReferenceInterference::~PControllerWithReferenceInterference ()
  {
    NS_LOG_FUNCTION (this);
  }

  double PControllerWithReferenceInterference::GetDeltaInterference(double rxPowerDbm, double expectedPdr, double currentNplusIDbm)
  {
#ifdef ENABLE_RID
    return 0;
#endif
    double a_0 = Controller::ComputeSlope (expectedPdr);
    double b_0 = expectedPdr - Controller::GetSnrByPdr (expectedPdr) * a_0;
    double I_r = rxPowerDbm + (b_0 - expectedPdr)/ a_0;
    double K_p = 1;
    double deltaInterferenceDb = K_p * (I_r - currentNplusIDbm);
    return deltaInterferenceDb;
  }




  NS_OBJECT_ENSURE_REGISTERED (PControllerWithDesiredPdr);

  TypeId PControllerWithDesiredPdr::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PControllerWithDesiredPdr")
      .SetParent<Controller> ()
      .AddConstructor <PControllerWithDesiredPdr> ()
      ;
    return tid;
  }

  PControllerWithDesiredPdr::PControllerWithDesiredPdr ()
  {
    m_deltaY = 0;
    m_ewmaCoefficient = 0.4;
    m_E0 = 0.04;
    NS_LOG_FUNCTION (this);
  }

  PControllerWithDesiredPdr::~PControllerWithDesiredPdr ()
  {
    NS_LOG_FUNCTION (this);
  }


  double PControllerWithDesiredPdr::GetDeltaInterference(double desiredPdr, double currentPdr)
  {
#ifdef ENABLE_RID
    return 0;
#endif
    if (currentPdr == 0) // we suppose the minimum current Pdr is 0.001 according to the current accuracy of the array @m_pdrToSnr;
    {
      currentPdr = 0.002; // the minimum pdr in the simulation;
    }
    double slope = 0.0;

#ifndef NO_PROTECTION // Use protection, E_0 is enabled.
    if (fabs (currentPdr - desiredPdr ) > m_E0 )
    {
      slope = Controller::ComputeSlope (desiredPdr);
    }
    else if (fabs (currentPdr - desiredPdr ) <= m_E0 )
    {
      slope = Controller::ComputeSlope (currentPdr);
    }
#endif

    double deltaSnr = 0.001/slope;
    double pParameter = -1.0/slope;
    double deltaInterferenceDb = pParameter * (desiredPdr - currentPdr);
    std::cout<<"p_controller: slope: "<<slope<<"\tpParameter: "<<pParameter<<"\tdeltaSnr: "<<deltaSnr<<"\tdesiredPdr: "<< desiredPdr <<"\tcurrentPdr: "<< currentPdr<<"\tdeltaInterference: "<<deltaInterferenceDb<<std::endl;
    /*
       if (desiredPdr == currentPdr)
       {
       return 0;
       }
       */
    return deltaInterferenceDb;
  }

  NS_OBJECT_ENSURE_REGISTERED (MinimumVarianceController);
  TypeId MinimumVarianceController::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::MinimumVarianceController")
      .SetParent<Controller> ()
      .AddConstructor <MinimumVarianceController> ()
      ;
    return tid;
  }
  MinimumVarianceController::MinimumVarianceController ()
  {
    m_deltaY = 0;
    m_ewmaCoefficient = 0.4;
    m_E0 = 0.04;
    NS_LOG_FUNCTION (this);
  }
  MinimumVarianceController::~MinimumVarianceController ()
  {
    NS_LOG_FUNCTION (this);
  }
  double MinimumVarianceController::GetDeltaInterference (double desiredPdr, double ewmaCurrentPdr, double estimatedCurrentPdr, bool &conditionTwoMeet)
  {
#ifdef ENABLE_RID
    return 0;
#endif
    double slope = 0;
#ifndef NO_PROTECTION // Use protection, m_E0 is enabled.
    if (fabs (estimatedCurrentPdr - desiredPdr ) > m_E0 )
    {
      conditionTwoMeet = true;
      //slope = Controller::ComputeSlope (ewmaCurrentPdr) > Controller::ComputeSlope (desiredPdr) ? Controller::ComputeSlope (ewmaCurrentPdr) :Controller::ComputeSlope (desiredPdr);
      slope = Controller::ComputeSlope (desiredPdr);
    }
    else if (fabs (estimatedCurrentPdr - desiredPdr ) <= m_E0 )
    {
      slope = Controller::ComputeSlope (ewmaCurrentPdr);
    }
#endif
#ifdef NO_PROTECTION // Do not use protection, always use the current EWMA PDR.
    slope = Controller::ComputeSlope (ewmaCurrentPdr);
#endif
    double deltaInterferenceDb = (m_ewmaCoefficient * ewmaCurrentPdr + (1 - m_ewmaCoefficient) * estimatedCurrentPdr - desiredPdr - m_deltaY ) /((1 - m_ewmaCoefficient) * slope);
    std::cout<<" controller: desired.pdr: "<<desiredPdr <<" ewma.current.pdr: "<< ewmaCurrentPdr <<" estimated.current.pdr: "<< estimatedCurrentPdr <<" delta.interference.db: "<< deltaInterferenceDb <<" 1/((1-m_ewmaCoefficient)*slope): "<< 1 / ((1- m_ewmaCoefficient) * slope )<<" slope: "<< slope << std::endl;
    return deltaInterferenceDb;
  }
}
