#include "double-regression.h"
#include "exclusion-region-helper.h"
#include "observation.h"
#include "ns3/log.h"
#include "matrix.h"
#include <iostream>
#include "ns3/simulator.h"
#include <cmath>

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
  void DoubleRegression::Initialize (std::vector<ObservationItem> vec, Matrix &phi, Matrix &pathLoss, bool senderCoordinates)
  {
    for (uint32_t i = 0; i < phi.GetM (); ++ i)
    {
      phi.SetValue (i,0, 1);
      if ( senderCoordinates ==  true)
      {
        phi.SetValue (i,1, vec[i].senderX);
        phi.SetValue (i,2, vec[i].senderY);
      }
      else
      {
        phi.SetValue (i,1, vec[i].receiverX);
        phi.SetValue (i,2, vec[i].receiverY);
      }
      pathLoss.SetValue (i, 0, vec[i].averageAttenuation);
    }
  }

  void DoubleRegression::Initialize (std::vector<ParameterObservation> vec, Matrix &phi, Matrix &pathLoss, bool senderCoordinates)
  {
    for (uint32_t i = 0; i < phi.GetM (); ++ i)
    {
      phi.SetValue (i,0, 1);
      if ( senderCoordinates ==  true)
      {
        phi.SetValue (i,1, vec[i].senderX);
        phi.SetValue (i,2, vec[i].senderY);
      }
      else
      {
        phi.SetValue (i,1, vec[i].receiverX);
        phi.SetValue (i,2, vec[i].receiverY);
      }
      pathLoss.SetValue (i, 0, vec[i].parameter);
    }
  }

  void DoubleRegression::Initialize (Observation obs, Matrix &phi, Matrix &pathLoss)
  {
    std::vector <LinkObservations> obsVector = obs.m_observations;
    for (uint32_t i = 0; i < phi.GetM (); ++ i)
    {
      //std::cout<<" m_phi.GetM () "<<phi.GetM () << " m_phi.GetN ()" <<phi.GetN () << std::endl;
      phi.SetValue (i,0, 1);
      phi.SetValue (i,1, obsVector[i].observations[0].receiverX);
      phi.SetValue (i,2, obsVector[i].observations[0].receiverY);
      pathLoss.SetValue (i, 0, obsVector[i].observations[0].averageAttenuation);
    }
  }

  /* Please check Equation 18 in vPRKS.pdf
   */
  bool DoubleRegression::GetCoefficientBeta (Matrix &betaMatrix, Matrix &phi, Matrix &pathLoss)
  {
    bool betaExist = false;
    Matrix phiTranspose = Matrix (phi.GetN (), phi.GetM ());
    Matrix phiDotPhiTranspose = Matrix ( phiTranspose.GetM (), phi.GetN ());

    phi.Transpose (phiTranspose); // \Phi^T * \Phi

    phiTranspose.Product (phi, phiDotPhiTranspose); // (\Phi^T \Phi)^{-1}
    //std::cout<<" phi^t * phi"<< std::endl;
    //phiDotPhiTranspose.ShowMatrix ();
    
    Matrix inv = Matrix (phiDotPhiTranspose.GetM (), phiDotPhiTranspose.GetN ());
    betaExist = phiDotPhiTranspose.Inverse (inv);
    if (betaExist == false)
    {
      std::cout<<" inverse does  not exits "<< std::endl;
      return betaExist;
    }
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
    return betaExist;
  }

  double DoubleRegression::ParameterEstimation (uint16_t sender, uint16_t receiver, double senderX, double senderY, double receiverX, double receiverY, ExclusionRegionHelper ers)
  {
    std::vector<ParameterObservation> _vec;
    std::set<uint16_t> senders = ers.GetSenders ();
    for (std::set<uint16_t>::iterator it = senders.begin (); it != senders.end (); ++ it)
    {
      if ( sender == *it || receiver == *it)
        continue;
      std::vector<ParameterObservation> vec = ers.GetObservationsBySender (*it, senderX, senderY, receiverX, receiverY);
      uint32_t obsCount = vec.size ();

      if ( obsCount > 5)
      {
        Matrix phi = Matrix(obsCount, 3);
        Matrix pathLoss = Matrix (obsCount, 1);

        //std::cout<<" for sender: " << *it << " obsCount: "<< obsCount << std::endl;
        Initialize (vec, phi, pathLoss, false);
        //phi.ShowMatrix ();
        //pathLoss.ShowMatrix ();
        //phi.ShowMatrix ();
        Matrix  betaMatrix = Matrix (3,1);
        bool betaExist = false; 
        betaExist = GetCoefficientBeta (betaMatrix, phi, pathLoss);
        //betaMatrix.ShowMatrix ();
        //std::cout<<" beta exist: "<< betaExist << std::endl;
        //phi.ShowMatrix ();
        if ( betaExist ==  true)
        {
          NodeStatus status = Simulator::GetNodeStatus (*it);
          //std::cout<<" coefficients are: "<< std::endl;
          //betaMatrix.ShowMatrix ();
          double parameter= betaMatrix.GetValue (0,0) + receiverX * betaMatrix.GetValue (1,0) + receiverY * betaMatrix.GetValue (2,0);
          double dist = sqrt ( pow(status.x - receiverX, 2) + pow ( status.y - receiverY, 2 ));
          if ( dist == 0)// || dist > LINK_DISTANCE_THRESHOLD)
            continue;

          ParameterObservation item;
          item.senderX = status.x;
          item.senderY = status.y;
          item.receiverX = receiverX;
          item.receiverY = receiverY;
          item.parameter = parameter;
          _vec.push_back (item);
        }
      }
    }

    uint32_t obsCount = _vec.size ();
    std::cout<<" obsCount: "<< _vec.size () << std::endl;
    if ( obsCount > 5)
    {
        Matrix phi = Matrix(obsCount, 3);
        Matrix pathLoss = Matrix (obsCount, 1);
        Initialize (_vec, phi, pathLoss, true);
        Matrix  betaMatrix = Matrix (3,1);
        bool betaExist = false; 
        betaExist = GetCoefficientBeta (betaMatrix, phi, pathLoss); 
        if (betaExist == true)
        {
          double parameter= betaMatrix.GetValue (0,0) + senderX * betaMatrix.GetValue (1,0) + senderY * betaMatrix.GetValue (2,0);
          return parameter;
        }
        else
        {
          return 0;
        }
    }
    else
    {
      return 0;
    }
  }

  double DoubleRegression::AttenuationEstimation (uint16_t sender, uint16_t receiver, double senderX, double senderY, double receiverX, double receiverY, Observation obs)
  {
    //std::cout<<" senderx: "<< senderX <<" sendery: "<< senderY <<" recieverx: "<< receiverX <<" receivery: "<< receiverY << std::endl;
    std::vector<ObservationItem> _vec;
    std::set<uint16_t> senders = obs.FetchSenders ();
    for (std::set<uint16_t>::iterator it = senders.begin (); it != senders.end (); ++ it)
    {
      if ( sender == *it || receiver == *it)
        continue;
      std::vector<ObservationItem> vec = obs.FetchLinkObservationBySender (*it, senderX, senderY, receiverX, receiverY);
      uint32_t obsCount = vec.size ();
      if ( obsCount > 5)
      {
        Matrix phi = Matrix(obsCount, 3);
        Matrix pathLoss = Matrix (obsCount, 1);

        //std::cout<<" for sender: " << *it << " obsCount: "<< obsCount << std::endl;
        Initialize (vec, phi, pathLoss, false);
        //phi.ShowMatrix ();
        //pathLoss.ShowMatrix ();
        //phi.ShowMatrix ();
        Matrix  betaMatrix = Matrix (3,1);
        bool betaExist = false; 
        betaExist = GetCoefficientBeta (betaMatrix, phi, pathLoss);
        //betaMatrix.ShowMatrix ();
        //std::cout<<" beta exist: "<< betaExist << std::endl;
        //phi.ShowMatrix ();
        if ( betaExist ==  true)
        {
          NodeStatus status = Simulator::GetNodeStatus (*it);
          //std::cout<<" coefficients are: "<< std::endl;
          //betaMatrix.ShowMatrix ();
          double atten = betaMatrix.GetValue (0,0) + receiverX * betaMatrix.GetValue (1,0) + receiverY * betaMatrix.GetValue (2,0);
          double dist = sqrt ( pow(status.x - receiverX, 2) + pow ( status.y - receiverY, 2 ));
          if ( dist == 0)// || dist > LINK_DISTANCE_THRESHOLD)
            continue;
          std::cout<<" for sender: "<< *it <<" dist: "<< dist << std::endl;

          //std::cout<<" for sender: "<< *it <<" (" << status.x <<", "<< status.y <<") receiver: "<< receiver <<" (" << receiverX <<", "<<receiverY<<") atten: "<< atten << std::endl;
          ObservationItem item;
          item.senderX = status.x;
          item.senderY = status.y;
          item.receiverX = receiverX;
          item.receiverY = receiverY;
          item.averageAttenuation = atten;
          _vec.push_back (item);
        }
      }
    }

    uint32_t obsCount = _vec.size ();
    std::cout<<" obsCount: "<< _vec.size () << std::endl;
    if ( obsCount > 5)
    {
        Matrix phi = Matrix(obsCount, 3);
        Matrix pathLoss = Matrix (obsCount, 1);
        Initialize (_vec, phi, pathLoss, true);
        //phi.ShowMatrix ();
        //pathLoss.ShowMatrix ();
        Matrix  betaMatrix = Matrix (3,1);
        bool betaExist = false; 
        betaExist = GetCoefficientBeta (betaMatrix, phi, pathLoss); 
        //std::cout<<" final result exits: "<< betaExist << std::endl;
        if (betaExist == true)
        {
          //double atten = betaMatrix.GetValue (0,0) + senderX * betaMatrix.GetValue (1,0) + senderY * betaMatrix.GetValue (2,0)
            //+ receiverX * betaMatrix.GetValue (3,0) + receiverY * betaMatrix.GetValue (4,0);
          double atten = betaMatrix.GetValue (0,0) + senderX * betaMatrix.GetValue (1,0) + senderY * betaMatrix.GetValue (2,0);
          return atten;
        }
        else
        {
          return 0;
        }
    }
    else
    {
      return 0;
    }

  }

}
