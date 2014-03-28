#include "math-helper.h"
#include <cmath>
#include "ns3/log.h"
//#include <gsl/gsl_cdf.h>
#include <cstddef>

NS_LOG_COMPONENT_DEFINE ("MathHelper");
namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (MathHelper);

TypeId MathHelper::GetTypeId(void)
{
  static TypeId tid = TypeId ("ns3::MathHelper")
    .SetParent<Object> ()
    .AddConstructor<MathHelper> ()
    ;
  return tid;
}

MathHelper::MathHelper ()
{

  //m_2_SqrtPi = 1.12837916709551257389615890312;
  
  //m_Sqrt1_2 = 0.70710678118654752440084436210;
  //m_1_Sqrt2Pi = (m_2_SqrtPi * m_Sqrt1_2 / 2.0);
  //m_Gsl_Dbl_Epsilon = 2.2204460492503131e-16;
  //m_Sqrt2 = 1.41421356237309504880168872421;
  //m_Gauss_Epsilon = m_Gsl_Dbl_Epsilon / 2;
  //m_Gauss_XUpper = 8.572;
  //m_Gauss_XLower = -37.519;
  //m_Gauss_Scale = 16.0;
  m_P_t = -15;
  m_d0 = 1;
  m_PL_d0 = 55;
  m_sigma = 3;
  m_B_N = 2000;
  m_R = 250 * 8;
  //m_Sqrt32 = 4.0 * m_Sqrt2;
  m_f = 132;
  m_l = 5;
  NS_LOG_FUNCTION (this);
}



double MathHelper::PdrToGamma (double pdr, uint32_t length)
{
  // if pdr =1, the value of gamma will be infinity.
  // in this case, we should make pdr slightly less than 1;
  m_f = (double) length;
  if ( pdr == 1.0)
  {
    pdr = 0.9999999;
  }
  double gaussX = 1 - pow (pdr, 1/(64*m_f) );
  double gammaDs = m_R * pow (2 * m_B_N, -1) * pow (NormInv (gaussX), 2);

  double gamma = gammaDs / 4.5;
  double gammaDB = 10 * log10 (gamma); //change into dB form
  
  return gammaDB;
}


double MathHelper::NormInv (const double p)
{
  //std::cout<<"gsl_cdf_ugaussian_Pinv (0.15): "<<gsl_cdf_ugaussian_Pinv (0.15)<<std::endl;
  return gsl_cdf_ugaussian_Pinv (p);
}

MathHelper::~MathHelper () { NS_LOG_FUNCTION (this);
}

double
MathHelper::NormPdf (double gamma, double mu, double sigma)
{
  const double PI = 3.1415926535;
  return (1/(sigma * sqrt (2*PI))) * exp( (-1)* pow(gamma-mu, 2) / (2 * pow (sigma, 2)));
}


double MathHelper::NormCdf ( const double x)
{
  //std::cout<<"gsl_cdf_ugaussian_P (1.0): "<<gsl_cdf_ugaussian_P (1.0)<<std::endl;
  return gsl_cdf_ugaussian_P (x);
}

double
MathHelper::gsl_cdf_ugaussian_P (const double x)
{
  double result;
  double absx = fabs (x);

  if (absx < gauss_EPSILON)
    {
      result = 0.5;
      return result;
    }
  else if (absx < 0.66291)
    {
      result = 0.5 + gauss_small (x);
      return result;
    }
  else if (absx < m_SQRT32)
    {
      result = gauss_medium (x);

      if (x > 0.0)
        {
          result = 1.0 - result;
        }

      return result;
    }
  else if (x > gauss_XUPPER)
    {
      result = 1.0;
      return result;
    }
  else if (x < gauss_XLOWER)
    {
      result = 0.0;
      return result;
    }
  else
    {
      result = gauss_large (x);

      if (x > 0.0)
        {
          result = 1.0 - result;
        }
    }

  return result;
}

double
MathHelper::gsl_cdf_ugaussian_Q(const double x)
{
  double result;
  double absx = fabs (x);

  if (absx < gauss_EPSILON)
    {
      result = 0.5;
      return result;
    }
  else if (absx < 0.66291)
    {
      result = gauss_small (x);

      if (x < 0.0)
        {
          result = fabs (result) + 0.5;
        }
      else
        {
          result = 0.5 - result;
        }

      return result;
    }
  else if (absx < m_SQRT32)
    {
      result = gauss_medium (x);

      if (x < 0.0)
        {
          result = 1.0 - result;
        }

      return result;
    }
  else if (x > -(gauss_XLOWER))
    {
      result = 0.0;
      return result;
    }
  else if (x < -(gauss_XUPPER))
    {
      result = 1.0;
      return result;
    }
  else
    {
      result = gauss_large (x);

      if (x < 0.0)
        {
          result = 1.0 - result;
        }

    }

  return result;
}
double
MathHelper::gauss_small (const double x)
{
  unsigned int i;
  double result = 0.0;
  double xsq;
  double xnum;
  double xden;

  const double a[5] = {
    2.2352520354606839287,
    161.02823106855587881,
    1067.6894854603709582,
    18154.981253343561249,
    0.065682337918207449113
  };
  const double b[4] = {
    47.20258190468824187,
    976.09855173777669322,
    10260.932208618978205,
    45507.789335026729956
  };

  xsq = x * x;
  xnum = a[4] * xsq;
  xden = xsq;

  for (i = 0; i < 3; i++)
    {
      xnum = (xnum + a[i]) * xsq;
      xden = (xden + b[i]) * xsq;
    }

  result = x * (xnum + a[3]) / (xden + b[3]);

  return result;
}

/*
* Normal cdf for 0.66291 < fabs(x) < sqrt(32).
*/
double
 MathHelper::gauss_medium (const double x)
{
  unsigned int i;
  double temp = 0.0;
  double result = 0.0;
  double xnum;
  double xden;
  double absx;

  const double c[9] = {
    0.39894151208813466764,
    8.8831497943883759412,
    93.506656132177855979,
    597.27027639480026226,
    2494.5375852903726711,
    6848.1904505362823326,
    11602.651437647350124,
    9842.7148383839780218,
    1.0765576773720192317e-8
  };
  const double d[8] = {
    22.266688044328115691,
    235.38790178262499861,
    1519.377599407554805,
    6485.558298266760755,
    18615.571640885098091,
    34900.952721145977266,
    38912.003286093271411,
    19685.429676859990727
  };

  absx = fabs (x);

  xnum = c[8] * absx;
  xden = absx;

  for (i = 0; i < 7; i++)
    {
      xnum = (xnum + c[i]) * absx;
      xden = (xden + d[i]) * absx;
    }

  temp = (xnum + c[7]) / (xden + d[7]);

  result = get_del (x, temp);

  return result;
}

/*
* Normal cdf for
* {sqrt(32) < x < gauss_XUPPER} union { gauss_XLOWER < x < -sqrt(32) }.
*/
double
 MathHelper::gauss_large (const double x)
{
  int i;
  double result;
  double xsq;
  double temp;
  double xnum;
  double xden;
  double absx;

  const double p[6] = {
    0.21589853405795699,
    0.1274011611602473639,
    0.022235277870649807,
    0.001421619193227893466,
    2.9112874951168792e-5,
    0.02307344176494017303
  };
  const double q[5] = {
    1.28426009614491121,
    0.468238212480865118,
    0.0659881378689285515,
    0.00378239633202758244,
    7.29751555083966205e-5
  };

  absx = fabs (x);
  xsq = 1.0 / (x * x);
  xnum = p[5] * xsq;
  xden = xsq;

  for (i = 0; i < 4; i++)
    {
      xnum = (xnum + p[i]) * xsq;
      xden = (xden + q[i]) * xsq;
    }

  temp = xsq * (xnum + p[4]) / (xden + q[4]);
  temp = (m_1_SQRT2PI - temp) / absx;

  result = get_del (x, temp);

  return result;
}

double
 MathHelper::get_del (double x, double rational)
{
  double xsq = 0.0;
  double del = 0.0;
  double result = 0.0;

  xsq = floor (x * gauss_SCALE) / gauss_SCALE;
  del = (x - xsq) * (x + xsq);
  del *= 0.5;

  result = exp (-0.5 * xsq * xsq) * exp (-1.0 * del) * rational;

  return result;
}
/**********************************************************************************************************************/

double
 MathHelper::gsl_cdf_ugaussian_Pinv (const double P)
{
  double r, x, pp;

  double dP = P - 0.5;

  if (P == 1.0)
    {
      return GSL_POSINF;
    }
  else if (P == 0.0)
    {
      return GSL_NEGINF;
    }

  if (fabs (dP) <= 0.425)
    {
      x = small (dP);

      return x;
    }

  pp = (P < 0.5) ? P : 1.0 - P;

  r = sqrt (-log (pp));

  if (r <= 5.0)
    {
      x = intermediate (r);
    }
  else
    {
      x = tail (r);
    }

  if (P < 0.5)
    {
      return -x;
    }
  else
    {
      return x;
    }

}

double
 MathHelper::small (double q)
{
  const double a[8] = { 3.387132872796366608, 133.14166789178437745,
    1971.5909503065514427, 13731.693765509461125,
    45921.953931549871457, 67265.770927008700853,
    33430.575583588128105, 2509.0809287301226727
  };

  const double b[8] = { 1.0, 42.313330701600911252,
    687.1870074920579083, 5394.1960214247511077,
    21213.794301586595867, 39307.89580009271061,
    28729.085735721942674, 5226.495278852854561
  };

  double r = 0.180625 - q * q;

  double x = q * rat_eval (a, 8, b, 8, r);

  return x;
}

double
 MathHelper::intermediate (double r)
{
  const double a[] = { 1.42343711074968357734, 4.6303378461565452959,
    5.7694972214606914055, 3.64784832476320460504,
    1.27045825245236838258, 0.24178072517745061177,
    0.0227238449892691845833, 7.7454501427834140764e-4
  };

  const double b[] = { 1.0, 2.05319162663775882187,
    1.6763848301838038494, 0.68976733498510000455,
    0.14810397642748007459, 0.0151986665636164571966,
    5.475938084995344946e-4, 1.05075007164441684324e-9
  };

  double x = rat_eval (a, 8, b, 8, (r - 1.6));

  return x;
}

double
 MathHelper::tail (double r)
{
  const double a[] = { 6.6579046435011037772, 5.4637849111641143699,
    1.7848265399172913358, 0.29656057182850489123,
    0.026532189526576123093, 0.0012426609473880784386,
    2.71155556874348757815e-5, 2.01033439929228813265e-7
  };

  const double b[] = { 1.0, 0.59983220655588793769,
    0.13692988092273580531, 0.0148753612908506148525,
    7.868691311456132591e-4, 1.8463183175100546818e-5,
    1.4215117583164458887e-7, 2.04426310338993978564e-15
  };

  double x = rat_eval (a, 8, b, 8, (r - 5.0));

  return x;
}

double
 MathHelper::rat_eval (const double a[], const size_t na,
          const double b[], const size_t nb, const double x)
{
  size_t i, j;
  double u, v, r;

  u = a[na - 1];

  for (i = na - 1; i > 0; i--)
    {
      u = x * u + a[i - 1];
    }

  v = b[nb - 1];

  for (j = nb - 1; j > 0; j--)
    {
      v = x * v + b[j - 1];
    }

  r = u / v;

  return r;
}

double MathHelper::gsl_posinf (void)
{
  return gsl_fdiv (+1.0, 0.0);
}
double MathHelper::gsl_neginf (void)
{
  return gsl_fdiv (-1.0, 0.0);
}
double MathHelper::gsl_fdiv (double x, double y)
{
  return x / y;
}
}
