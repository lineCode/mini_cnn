#ifndef __INPUT_LAYER_H__
#define __INPUT_LAYER_H__

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

#include "types.h"
#include "utils.h"
#include "math/vectorn.h"
#include "math/matrixmxn.h"
#include "math/mathdef.h"
#include "layer.h"

class InputLayer : public LayerBase
{
protected:
	int32_t m_width;
	int32_t m_height;
	int32_t m_depth;
public:
	InputLayer(uint32_t neuralCount) : LayerBase(neuralCount, new VectorInOut(), new VectorInOut())
	{
		m_width = 0;
		m_height = 0;
		m_depth = 0;
		VectorInOut* vec_out = dynamic_cast<VectorInOut*>(m_output);
		vec_out->m_value = new VectorN(neuralCount);
	}

	InputLayer(int32_t width, int32_t height, int32_t depth) : LayerBase(width * height * depth, new VectorInOut(), new MatrixInOut())
	{
		m_width = width;
		m_height = height;
		m_depth = depth;
		MatrixInOut* mat_out = dynamic_cast<MatrixInOut*>(m_output);
		mat_out->m_value = new Matrix3D(width, height, depth);
	}
	
	void SetInputData(const VectorN &input)
	{
		if (m_output->m_type == InOutType::IO_Vector)
		{
			VectorInOut* vec_out = dynamic_cast<VectorInOut*>(m_output);
			vec_out->m_value->Copy(input);			
		}
		else
		{
			MatrixInOut* mat_out = dynamic_cast<MatrixInOut*>(m_output);
			mat_out->m_value = input.Unflatten(m_width, m_height, m_depth);
		}
		m_next->m_input = m_output;
	}

};

#endif //__INPUT_LAYER_H__
