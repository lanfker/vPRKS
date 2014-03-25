#include "double-regression.h"
#include "observation.h"
#include "ns3/log.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("DoubleRegression");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (DoubleRegression);

  TypeId DoubleRegression::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::DoubleRegression")
      .SetParent<Object> ()
      .AddConstructor<DoubleRegression> ();
    return tid;
  }
  DoubleRegression::DoubleRegression ()
  {
  }


  DoubleRegression::~DoubleRegression ()
  {
  }
  /* Here, we assume the length of the \Phi vector is 4.
   */
  void DoubleRegression::Initialize (Observation obs)
  {
    std::vector <LinkObservations> obsVector = obs.m_observations;
    uint32_t m = obs.FindLinkCount ();
    //uint32_t n = obs.FindMinimumObservationLength ();
    //---------Initialize matrix \Phi and L
    m_phi = Matrix (m, 4);
    m_pathloss = Matrix (m, 1);
    for (uint32_t i = 0; i < m; ++ i)
    {
      m_phi.SetValue (i,0, obsVector[i].observations[0].senderX);
      m_phi.SetValue (i,1, obsVector[i].observations[0].senderY);
      m_phi.SetValue (i,2, obsVector[i].observations[0].receiverX);
      m_phi.SetValue (i,3, obsVector[i].observations[0].receiverY);
      m_pathloss.SetValue (i, 0, obsVector[i].observations[0].averageAttenuation);
    }
  }

  /* Please check Equation 18 in vPRKS.pdf
   */
  void DoubleRegression::GetCoefficientBeta (Matrix &betaMatrix)
  {
    Matrix phiTranspose = Matrix (m_phi.GetN (), m_phi.GetM ());
    Matrix phiDotPhiTranspose = Matrix (m_phi.GetN (), phiTranspose.GetM ());

    m_phi.Transpose (phiTranspose); // \Phi^T * \Phi
    std::cout<<" original: "<<m_phi.GetM () <<"x"<<m_phi.GetN () << std::endl;
    std::cout<<" transpose: "<<phiTranspose.GetM () <<"x"<<phiTranspose.GetN () << std::endl;

    phiTranspose.Product (m_phi, phiDotPhiTranspose); // (\Phi^T \Phi)^{-1}
    std::cout<<" traspose*original: "<<phiTranspose.GetM () <<"x"<<phiTranspose.GetN () << std::endl;
    
    Matrix inv = Matrix (phiDotPhiTranspose.GetM (), phiDotPhiTranspose.GetN ());

    Matrix inverseDotTranspose = Matrix (inv.GetM (), phiTranspose.GetN ());
    inv.Product (phiTranspose, inverseDotTranspose);//(\Phi^T \Phi)^{-1} * \Phi^T

    betaMatrix = Matrix (inverseDotTranspose.GetM (), m_pathloss.GetN ());
    inverseDotTranspose.Product (m_pathloss, betaMatrix); // (\Phi^T \Phi)^{-1} * \Phi^T * L


  }

}
