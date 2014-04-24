#include "matrix.h"
#include "ns3/log.h"
#include <iostream>
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("Matrix");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (Matrix);

  TypeId Matrix::GetTypeId(void)
  {
    static TypeId tid = TypeId ("ns3::Matrix")
      .SetParent<Object> ()
      .AddConstructor<Matrix> ();
    return tid;
  }
  Matrix::Matrix ()
  {
    _m = 0;
    _n = 0;
  }

  Matrix::Matrix (uint32_t m, uint32_t n)
  {
    _m=m;
    _n=n;
    _matrix = NULL;
    _matrix = new double*[_m];
    for (uint32_t i = 0; i < _m; ++ i)
    {
      //std::cout<<" allocating array of one"<<" _m: "<<_m <<" _n: "<< _n<< " i: "<< i << std::endl;
      _matrix[i] = new double[_n];
      if (_matrix[i] == NULL)
      {
        std::cout<<"allocating memory failed"<< std::endl;
      }
    }

    for (uint32_t i = 0; i < _m ; ++ i)
    {
      for (uint32_t j = 0; j < _n; ++ j)
      {
        _matrix[i][j] = 0;
      }
    }

  }

  Matrix::~Matrix ()
  {
    for(uint32_t i = 0; i < _m; ++i) {
      delete [] _matrix[i];
    }
    delete [] _matrix;
  }

  void Matrix::SetM (uint32_t m)
  {
    _m = m;
  }
  void  Matrix::SetN (uint32_t n)
  {
    _n = n;
  }

  uint32_t Matrix::GetM ()
  {
    return _m;
  }
  uint32_t Matrix::GetN ()
  {
    return _n;
  }

  bool Matrix::SetValue (uint32_t i, uint32_t j, double value)
  {
    //std::cout<<" _matrix[i] = "<< _matrix[i] << std::endl;
    if (i > GetM () - 1 || j > GetN () )
    {
      return false;
    }
    _matrix[i][j] = value;
    return true;
  }
  bool Matrix::GetValue (uint32_t i, uint32_t j, double &value)
  {
    if (i > GetM () -1 || j > GetN () )
    {
      return false;
    }
    value = _matrix[i][j];
    return true;
  }

  double Matrix::GetValue (uint32_t i, uint32_t j)
  {
    return _matrix[i][j];
  }

  bool Matrix::Inverse (Matrix &inverseMatrix)
  {
    //this->ShowMatrix ();
    Matrix expandMatrix = Matrix (GetM (), GetM () * 2); // two times of the length
    //Matrix inverseMatrix = Matrix (GetM (), GetN ()); // m and n should be of the same value
    InitiateExpandMatrix ((*this), expandMatrix);
    bool canAdjust = expandMatrix.AdjustMatrix ();
    if (canAdjust == false)
      return false;
    expandMatrix.CalculateExpandMatrix ();

    GetInverseMatrix (expandMatrix, inverseMatrix);
    bool nanInfCheck = inverseMatrix.CheckNanAndInf ();
    //inverseMatrix.ShowMatrix ();
    return !nanInfCheck;
    //return true;

  }

  bool Matrix::CheckNanAndInf ()
  {
    int i,j;
    for ( i = 0; i < GetM (); ++ i)
    {
      for (j = 0; j < GetN (); ++ j)
      {
        double value;
        GetValue (i,j,value);
        if (isnan (value) == true || isinf (value) == true)
          return true;
      }
    }
    return false;
  }

  bool Matrix::AdjustMatrix ()
  {
    for (uint32_t i = 0;  i < GetM (); ++ i)
    {
      double value;
      GetValue (i,i,value);
      if (value == 0)
      {
        uint32_t j;
        for (j = 0; j < GetM (); ++ j)
        {
          double val;
          GetValue (j,i,val);
          if ( val != 0)
          {
            double* temp = _matrix[i]; 
            _matrix[i] = _matrix[j];
            _matrix[j] = temp;
            break;
          }
        }
        if ( j >= GetM ())
        {
          return false;
        }
      }
    }
    return true;
  }

  void Matrix::InitiateExpandMatrix (Matrix &sourceMatrix, Matrix &expandMatrix)
  {
    for (uint32_t i = 0; i < expandMatrix.GetM (); ++ i)
    {
      for (uint32_t j = 0; j < expandMatrix.GetN (); ++ j)
      {
        if (j < expandMatrix.GetM ())
        {
          double value;
          sourceMatrix.GetValue (i,j,value);
          expandMatrix.SetValue (i,j, value);
        }
        else 
        {
          if ( j == expandMatrix.GetM () + i)
          {
            expandMatrix.SetValue (i,j, 1);
          }
          else
          {
            expandMatrix.SetValue (i,j, 0);
          }
        }
      }
    }
    //expandMatrix.ShowMatrix ();
  }


  void Matrix::CalculateExpandMatrix ()
  {
    for (uint32_t i = 0; i < GetM (); ++ i)
    {
      double firstElement;
      GetValue (i,i,firstElement);

      for (uint32_t j = 0; j < GetN (); ++ j)
      {
        double targetElement;
        GetValue (i,j,targetElement);
        targetElement = targetElement / firstElement;
        SetValue (i,j,targetElement);
      }
      //this->ShowMatrix ();
      for (uint32_t m = 0; m < GetM (); ++ m)
      {
        if (m == i)
          continue;
        double times;
        GetValue (m,i,times);
        for (uint32_t n = 0; n < GetN (); ++ n)
        {
          double value;
          GetValue (i, n, value);
          double targetValue;
          GetValue (m, n, targetValue);
          //std::cout<<" target_value: "<< targetValue <<" value: "<< value <<" times: "<< times;
          double final= targetValue - value * times;
          //std::cout<<" final: "<< final << std::endl;


          SetValue (m, n, final);
        }
      }
    }
    //this->ShowMatrix ();
  }

  void Matrix::GetInverseMatrix (Matrix &expandMatrix, Matrix &inverseMatrix)
  {
    for (uint32_t i = 0; i < expandMatrix.GetM (); ++ i)
    {
      for (uint32_t j = 0; j < expandMatrix.GetN (); ++ j)
      {
        if (j >= expandMatrix.GetM ())
        {
          double value;
          expandMatrix.GetValue (i, j, value);
          inverseMatrix.SetValue (i, j - expandMatrix.GetM (), value);
        }
      }
    }
  }

  void Matrix::ShowMatrix ()
  {
    std::cout<<"======   SHOW MATRIX  ===== "<< std::endl;
    for (uint32_t i = 0; i < GetM (); ++ i)
    {
      for (uint32_t j = 0; j < GetN (); ++ j)
      {
        double value;
        GetValue (i,j,value);
        std::cout<<"\t"<<value;
      }
      std::cout<<std::endl;
    }
    std::cout<<std::endl;
  }

  void Matrix::Product (Matrix &inverseMatrix, Matrix &resultMatrix)
  {
    NS_ASSERT ( this->GetN () == inverseMatrix.GetM ());
    for (uint32_t i = 0; i < this->GetM (); ++ i)
    {
      for (uint32_t j = 0; j < inverseMatrix.GetN (); ++ j)
      {
        for (uint32_t k = 0; k < inverseMatrix.GetM(); ++ k)
        {
          double source, target, result;
          this->GetValue(i,k,source);
          inverseMatrix.GetValue (k,j,target);
          resultMatrix.GetValue (i,j,result);
          result += source * target;
          resultMatrix.SetValue (i,j,result);
        }
      }
    }
  }

  bool Matrix::Transpose (Matrix &transposeMatrix)
  {
    if ( GetM () == transposeMatrix.GetN () && GetN () == transposeMatrix.GetM ())
    {
      for (uint32_t i = 0; i < GetM (); ++ i)
      {
        for (uint32_t j = 0; j < GetN (); ++ j)
        {
          double _value;
          this->GetValue (i,j,_value);
          transposeMatrix.SetValue (j, i, _value);
        }
      }
      return true;
    }
    else
    {
      return false;
    }
  }

  void Matrix::ShowShape ()
  {
    std::cout<<"Row: "<<_m<<" Column: "<< _n<< std::endl;
  }
}
