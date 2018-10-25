#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#include <vector>
#include <cassert>

namespace mini_cnn
{

	typedef int					nn_int;
	typedef unsigned int		nn_uint;

	typedef double				nn_float;

	typedef std::vector<nn_int>	index_vec;

	const nn_float cEPSILON = 1e-4f;

#ifndef PI
	const nn_float cPI = 3.141592653589793f;
#endif

	const nn_float cZero = (nn_float)0.0;
	const nn_float cOne = (nn_float)1.0;

	const nn_float cMIN_FLOAT = std::numeric_limits<nn_float>::min();
	const nn_float cMAX_FLOAT = std::numeric_limits<nn_float>::max();

#ifdef _DEBUG
	#define nn_assert(cond) (void)(assert_break(cond))
	void assert_break(bool cond)
	{
		if (!cond)
		{
			std::cout << _CRT_WIDE(__FILE__) << __LINE__ << std::endl;
		}
		assert(cond);
	}
#else
	#define nn_assert(cond) ((void)0)
#endif

// memory alignment
#ifndef ALIGN
#if defined(__GNUC__)    // GCC
#    define ALIGN(n)    __attribute__((aligned(n)))
#elif defined( _MSC_VER ) // VC
#    define ALIGN(n)    __declspec(align(n))
#  endif
#endif // ALIGN

#define ALIGN_SIZE 32

#define USE_AVX
}
#endif // __COMMON_DEF_H__
