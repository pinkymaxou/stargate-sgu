#ifndef _HELPERMACRO_H_
#define _HELPERMACRO_H_

#include <math.h>

#define HELPERMACRO_LEDLOGADJ(_valuePerc,_maxPerc) ((pow(10.0f, _valuePerc) - 1.0f)/9.0f*_maxPerc)

#define HELPERMACRO_MIN(a,b) (((a)<(b))?(a):(b))
#define HELPERMACRO_MAX(a,b) (((a)>(b))?(a):(b))

#endif