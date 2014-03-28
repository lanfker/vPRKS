#ifndef MATH_HELPER_H
#define MATH_HELPER_H

#include "ns3/object.h"

#define m_2_SQRTPI 1.12837916709551257389615890312
#define m_SQRT1_2 0.70710678118654752440084436210
#ifndef m_1_SQRT2PI
#define m_1_SQRT2PI (m_2_SQRTPI * m_SQRT1_2 / 2.0)
#endif

#define gauss_EPSILON (gsl_DBL_EPSILON / 2)
#define gsl_DBL_EPSILON 2.2204460492503131e-16
#define m_SQRT2 1.41421356237309504880168872421
#define m_SQRT32 (4.0 * m_SQRT2)
#define gauss_XUPPER (8.572)
#define gauss_XLOWER (-37.519)
#define gauss_SCALE (16.0)


#ifdef INFINITY
# define GSL_POSINF INFINITY
# define GSL_NEGINF (-INFINITY)
#elif defined(HUGE_VAL)
# define GSL_POSINF HUGE_VAL
# define GSL_NEGINF (-HUGE_VAL)
#else
# define GSL_POSINF (gsl_posinf())
# define GSL_NEGINF (gsl_neginf())
#endif

// created by Chuan
namespace ns3
{
class MathHelper : public Object
{
public:
  static TypeId GetTypeId (void);
  MathHelper ();
  ~MathHelper ();
  /*
* given the parameters of a normal distribution,
* calculate the pdf value of this distribution
*/
  double NormPdf (double gamma, double mu, double sigma);
  /*
* calculate the cdf value for standard gaussian distribution
*/
  double NormCdf ( const double x);
  
  /* params length the length of a packet segment. the default 132 means the length of the whole packet.
* find the specific sinr for a given PRR (packet reception rate)
* param pdr a certain packet deliver rate
* return gamma (in dB): snr threshold in terms of dB
* others B_N = 30; kHz; noise bandwidth
* R = 19.2 kbps; radio
* f = 39 frame size in bytes
* l = 8 preamble size in bytes
*
*/
  double PdrToGamma (double pdr, uint32_t length = 132);
  
  /*
* calculate the cdf value of the inverse of te P(x) in standard gaussian distribution
*/
  double NormInv (const double p);

  double
  gsl_cdf_ugaussian_P (const double x);

  double
  gsl_cdf_ugaussian_Q(const double x);

  double
  gauss_small (const double x);

  /*
* Normal cdf for 0.66291 < fabs(x) < sqrt(32).
*/
  double
  gauss_medium (const double x);


  /*
* Normal cdf for
* {sqrt(32) < x < GAUSS_XUPPER} union { GAUSS_XLOWER < x < -sqrt(32) }.
*/
  double
  gauss_large (const double x);

  double
  get_del (double x, double rational);

  double
  gsl_cdf_ugaussian_Pinv (const double P);


  double
  small (double q);

  double
  intermediate (double r);


  double
  tail (double r);


  double
  rat_eval (const double a[], const size_t na,
            const double b[], const size_t nb, const double x);
  double gsl_posinf (void);
  double gsl_neginf (void);
  double gsl_fdiv (double x, double y);
private:
  // these variables are needed when calculating cdf and pdf of the guassian (normal) distribution
  // these variables are not in use anymore since we can use the GUN Scientific Library (GSL) in ns-3
  double m_2_SqrtPi;
  double m_Sqrt1_2;
  double m_1_Sqrt2Pi;
  double m_Gsl_Dbl_Epsilon ;
  double m_Sqrt2 ;
  double m_Gauss_Epsilon ;
  double m_Gauss_XUpper ;
  double m_Gauss_XLower;
  double m_Gauss_Scale ;
  double m_Sqrt32 ;

  //other values
  double m_P_t; // tx power in dBm
  double m_d0; // reference distance, unit is meter
  double m_PL_d0; // PL(d0) path loss at a reference distance
  double m_sigma; // standard deviation of the channel noise
  double m_B_N; // noise/ channel/receiver bandwidth: 30 kHz for CC1000, 2Mhz for Zigbee
                  //
  double m_R; // CC2420: R = 250kbps on 2450Mhz R=20kbps on 868Mhz BPSK;
                  // R = 40 on 915Mhz BPSK; CC1000 R = 19.2kbps
  double m_f; // frame size in bytes. CC1000: 39. CC2420: radio support a maximum packet of 128 bytes
  double m_l; // Phy preamble size of a frame in bytes. 5 for 802.15.4. 8 for mica2 platform.
};
}

#endif
