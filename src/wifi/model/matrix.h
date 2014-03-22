//The code of this file is based on code from (http://blog.csdn.net/zmazon/article/details/8241348)
/* This file is build for testing TraCI such that NS-3 may be work with SUMO cooperatively. 
 * 2013-12-02 Mon 10:20 AM
 */

#ifndef MATRIX_H 
#define MATRIX_H

#include "ns3/core-module.h"


namespace ns3{


  class Matrix : public Object
  {
    public:
      static TypeId GetTypeId(void);
      Matrix ();
      Matrix (uint32_t m, uint32_t n);
      ~Matrix ();
      void SetM (uint32_t m);
      void SetN (uint32_t n);
      uint32_t GetM ();
      uint32_t GetN ();
      bool SetValue (uint32_t i, uint32_t j, double value);
      bool GetValue (uint32_t i, uint32_t j, double &value);
      bool Inverse (Matrix &inverseMatrix);
      void InitiateExpandMatrix (Matrix &sourceMatrix, Matrix &expandMatrix);
      bool AdjustMatrix ();
      void CalculateExpandMatrix ();
      void GetInverseMatrix (Matrix &expandMatrix, Matrix &inverseMatrix);
      void ShowMatrix ();
      void Product (Matrix &inverseMatrix, Matrix &resultMatrix);
    private:
      // In our case, _m and _n should be equal
      uint32_t _m;
      uint32_t _n;
      double **_matrix;
  };


}


#endif //MATRIX_H
