/*
 * File:   approxmath.h
 * Author: estama
 *
 * Created on April 6, 2009, 2:57 AM
 */

#ifndef __APPROXMATH_H__
#define	__APPROXMATH_H__

// Calculates approximate e^x.
// Use it in code not requiring precision
inline float approx_exp (const float x)
{
	union
	{
	  float f;
	  int i;
	} _eco;

  if (x < -15)
  return 0. ;
   else if (x > 88)
     return 1e38 ;
   else
	_eco.i=12102203*x+1064652319;
	return _eco.f;
}

// Calculates approximate 2^x
// Use it in code not requiring precision
inline float approx_pow2(const float x)
{
	union
	{
	  float f;
	  int i;
	} _eco;

	 _eco.i=8388608*x+1065353216;
     return _eco.f;
}

// Calculates approximate x^y
// Use it in code not requiring precision
inline float approx_pow (const float x, const float y)
{
	union
	{
	  float f;
	  int i;
	} _eco;

	_eco.f=x;
	_eco.i=y*(_eco.i-1065353216) + 1065353216;
	return _eco.f;
}

// Calculates approximate square_root(x)
// Use it in code not requiring precision
inline float approx_sqrt(const float x)
{
    union
    {
      float f;
      int i;
    } _eco;

    _eco.f=x;
    _eco.i=((_eco.i-1065353216)>>1) + 1065353216;
    return _eco.f;
}

// Calculates approximate 1/square_root(x)
// it is faster than fast_invSqrt BUT
// use it in code not requiring precision
inline float approx_invsqrt(const float x)
{
  union // get bits for floating value
  {
    float x;
    int i;
  } u;
  u.x = x;
  u.i = 0x5f3759df - (u.i >> 1);  // gives initial guess y0
  return u.x;
}

// This function is a classic 1/square_root(x)code
// used by quake's game engine.
// It is very fast and has enough precision
// to drive a physics engine.
inline float fast_invSqrt(const float x)
{
  const float xhalf = 0.5f*x;

  union // get bits for floating value
  {
    float x;
    int i;
  } u;
  u.x = x;
  u.i = 0x5f3759df - (u.i >> 1);  // gives initial guess y0
  u.x=u.x*(1.5f - xhalf*u.x*u.x); // Newton step, repeating increases accuracy
  return u.x;
}

// It calculates a fast and accurate square_root(x)
inline float fast_sqrt(const float x)
{
  return x * fast_invSqrt(x);
}

inline float sign(const float x)
{
	return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f;
}


#endif	/* _APPROXMATH_H */
