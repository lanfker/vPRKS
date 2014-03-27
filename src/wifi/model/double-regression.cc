#include "double-regression.h"
#include "observation.h"
#include "ns3/log.h"
#include "matrix.h"
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

  void DoubleRegression::Initialize (Observation obs, Matrix &phi, Matrix &pathLoss)
  {
    std::vector <LinkObservations> obsVector = obs.m_observations;
    //uint32_t n = obs.FindMinimumObservationLength ();
    //---------Initialize matrix \Phi and L
    for (uint32_t i = 0; i < phi.GetM (); ++ i)
    {
      //std::cout<<" m_phi.GetM () "<<phi.GetM () << " m_phi.GetN ()" <<phi.GetN () << std::endl;
      phi.SetValue (i,0, obsVector[i].observations[0].senderX);
      phi.SetValue (i,1, obsVector[i].observations[0].senderY);
      phi.SetValue (i,2, obsVector[i].observations[0].receiverX);
      phi.SetValue (i,3, obsVector[i].observations[0].receiverY);
      pathLoss.SetValue (i, 0, obsVector[i].observations[0].averageAttenuation);
    }
  }

  /* Please check Equation 18 in vPRKS.pdf
   */
  void DoubleRegression::GetCoefficientBeta (Matrix &betaMatrix, Matrix &phi, Matrix &pathLoss)
  {
    Matrix phiTranspose = Matrix (phi.GetN (), phi.GetM ());
    Matrix phiDotPhiTranspose = Matrix ( phiTranspose.GetM (), phi.GetN ());

    phi.Transpose (phiTranspose); // \Phi^T * \Phi

    phiTranspose.Product (phi, phiDotPhiTranspose); // (\Phi^T \Phi)^{-1}
    //std::cout<<" phi^t * phi"<< std::endl;
    //phiDotPhiTranspose.ShowMatrix ();
    
    Matrix inv = Matrix (phiDotPhiTranspose.GetM (), phiDotPhiTranspose.GetN ());
    phiDotPhiTranspose.Inverse (inv);
    //std::cout<<" (phi^t * phi)^{-1}"<< std::endl;
    //inv.ShowMatrix ();

    Matrix inverseDotTranspose = Matrix (inv.GetM (), phiTranspose.GetN ());
    inv.Product (phiTranspose, inverseDotTranspose);//(\Phi^T \Phi)^{-1} * \Phi^T
    //std::cout<<" (phi^t * phi)^{-1} * phi^t"<< std::endl;
    //inverseDotTranspose.ShowMatrix (); 

    //betaMatrix = Matrix (inverseDotTranspose.GetM (), pathLoss.GetN ());
    //std::cout<<" (phi^t * phi)^{-1} * phi^t * L"<< std::endl;
    //pathLoss.ShowMatrix ();
    inverseDotTranspose.Product (pathLoss, betaMatrix); // (\Phi^T \Phi)^{-1} * \Phi^T * L
  }

}
