#ifndef __CONVOLUTIONAL_LAYER_H__
#define __CONVOLUTIONAL_LAYER_H__

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
#include "math/matrix3d.h"
#include "math/mathdef.h"
#include "layer.h"

class FilterDimension
{
public:
	int32_t m_width;
	int32_t m_height;
	int32_t m_channels;
	int32_t m_padding;
	int32_t m_stride_w;
	int32_t m_stride_h;
	FilterDimension(int32_t w, int32_t h, int32_t c, int32_t padding, int32_t stride_w, int32_t stride_h) :
		m_width(w), m_height(h), m_channels(c), m_padding(padding), m_stride_w(stride_w), m_stride_h(stride_h)
	{
	}
	FilterDimension(const FilterDimension &filterDim) :
		m_width(filterDim.m_width), m_height(filterDim.m_height), m_channels(filterDim.m_channels)
		, m_padding(filterDim.m_padding), m_stride_w(filterDim.m_stride_w), m_stride_h(filterDim.m_stride_h)
	{
	}
};

class Pooling
{
public:
	int32_t m_width;
	int32_t m_height;
	int32_t m_padding;
	int32_t m_stride_w;
	int32_t m_stride_h;
	Pooling(int32_t w, int32_t h, int32_t padding, int32_t stride_w, int32_t stride_h) :
		m_width(w), m_height(h), m_padding(padding), m_stride_w(stride_w), m_stride_h(stride_h)
	{
	}
	Pooling(const Pooling &pooling) :
		m_width(pooling.m_width), m_height(pooling.m_height)
		, m_padding(pooling.m_padding), m_stride_w(pooling.m_stride_w), m_stride_h(pooling.m_stride_h)
	{
	}
};


/*
for the l-th Conv layer:
	(l)     (l-1)      (l)    (l)
   Z	=  X       *  W    + B
    (l)      (l)
   X    = f(Z   )
*/
class ConvolutionalLayer : public LayerBase
{
public:
	std::vector<Matrix3D*> m_filters;
	VectorN *m_bias;

	VectorN *m_db;	// dJ/d(bias)
	std::vector<Matrix3D*> m_dw;	// dJ/d(w)
	Matrix3D *m_delta;		// dJ/d(z)

protected:
	Matrix3D *m_middle;	// middle value

	Matrix3D *m_input_img;
	Matrix3D *m_pre_pool_img;
	Matrix3D *m_output_img;
	Matrix3D *m_middle_prime;
	std::vector<int32_t> *m_idx_map;

	Matrix3D *m_pre_unpool_delta;

	// mini-batch��ÿ��batch��Ķ��ѵ���������ݶ�֮��
	VectorN *m_sum_db;
	std::vector<Matrix3D*> m_sum_dw;

protected:
	FilterDimension m_filterDim;
	Pooling *m_pooling;
	eActiveFunc m_activeFuncType;
	MatActiveFunc m_activeFunc;
	MatActiveFunc m_activePrimeFunc;

public:
	ConvolutionalLayer(int32_t filterCount
		, FilterDimension *filterDim
		, Pooling *pooling
		, eActiveFunc act)
		: LayerBase(0, new MatrixInOut(), new MatrixInOut())
		, m_filterDim(*filterDim)
		, m_pooling(pooling)
		, m_activeFuncType(act)
	{

		for (int32_t i = 0; i < filterCount; ++i)
		{
			m_filters.push_back(new Matrix3D(m_filterDim.m_width, m_filterDim.m_height, m_filterDim.m_channels));
		}
		m_bias = new VectorN(filterCount);

		for (int32_t i = 0; i < filterCount; ++i)
		{
			m_dw.push_back(new Matrix3D(m_filterDim.m_width, m_filterDim.m_height, m_filterDim.m_channels));
		}
		m_db = new VectorN(filterCount);

		for (int32_t i = 0; i < filterCount; ++i)
		{
			m_sum_dw.push_back(new Matrix3D(m_filterDim.m_width, m_filterDim.m_height, m_filterDim.m_channels));
		}
		m_sum_db = new VectorN(filterCount);

		switch (act)
		{
		case eActiveFunc::eSigmod:
			m_activeFunc = Sigmoid;
			m_activePrimeFunc = SigmoidPrime;
			break;
		case eActiveFunc::eTanh:
			m_activeFunc = Tanh;
			m_activePrimeFunc = TanhPrime;
			break;
		case eActiveFunc::eRelu:
			m_activeFunc = Relu;
			m_activePrimeFunc = ReluPrime;
			break;
		default:
			break;
		}
	}

	const Matrix3D& GetInput() const
	{
		MatrixInOut* mat_in = dynamic_cast<MatrixInOut*>(m_input);
		return *(mat_in->m_value);
	}

	Matrix3D& GetOutput() const
	{
		MatrixInOut* mat_out = dynamic_cast<MatrixInOut*>(m_output);
		return *(mat_out->m_value);
	}

	virtual void Connect(LayerBase *next)
	{
		LayerBase::Connect(next);

		MatrixInOut* pre_out = dynamic_cast<MatrixInOut*>(m_prev->m_output);
		auto input_img = pre_out->m_value;

		int32_t nd = m_filters.size();

		// calc output size
		// Padding::Valid
		int32_t input_w = input_img->Width();
		int32_t input_h = input_img->Height();
		int32_t nw = static_cast<int32_t>(floorf(1.0f * (input_w - m_filterDim.m_width) / m_filterDim.m_stride_w)) + 1;
		int32_t nh = static_cast<int32_t>(floorf(1.0f * (input_h - m_filterDim.m_height) / m_filterDim.m_stride_h)) + 1;

		m_middle = new Matrix3D(nw, nh, nd);
		m_middle_prime = new Matrix3D(nw, nh, nd);

		m_delta = new Matrix3D(nw, nh, nd);

		if (m_pooling != nullptr)
		{
			m_pre_pool_img = new Matrix3D(nw, nh, nd);

			m_idx_map = new std::vector<int32_t>(nw * nh);

			nw = static_cast<int32_t>(floorf(1.0f * (nw - m_pooling->m_width) / m_pooling->m_stride_w)) + 1;
			nh = static_cast<int32_t>(floorf(1.0f * (nh - m_pooling->m_height) / m_pooling->m_stride_h)) + 1;

			m_pre_unpool_delta = new Matrix3D(nw, nh, nd);

		}

		m_neuralCount = nw * nh * nd;

		MatrixInOut* this_out = dynamic_cast<MatrixInOut*>(m_output);
		this_out->m_value = new Matrix3D(nw, nh, nd);
		m_output_img = this_out->m_value;

	}

	virtual void Init(NormalRandom nrand)
	{
		for (int32_t k = 0; k < m_filters.size(); ++k)
		{
			auto filter = m_filters[k];
			int32_t w = filter->Width();
			int32_t h = filter->Height();
			int32_t d = filter->Depth();
			for (int32_t i = 0; i < w; ++i)
			{
				for (int32_t j = 0; j < h; ++j)
				{
					for (int32_t c = 0; c < d; ++c)
					{
						(*filter)(i, j, c) = nrand.GetRandom();
					}
				}
			}
		}

		for (int32_t i = 0; i < m_bias->GetSize(); ++i)
		{
			(*m_bias)[i] = nrand.GetRandom();
		}

	}

	virtual void Forward()
	{
		MatrixInOut* pre_out = dynamic_cast<MatrixInOut*>(m_prev->m_output);
		m_input_img = pre_out->m_value;

		m_input_img->Conv(m_middle, m_filters, m_filterDim.m_stride_w, m_filterDim.m_stride_h, Padding::Valid);
		m_middle->AddBias(*m_bias);

		if (m_pooling != nullptr)
		{
			m_activeFunc(*m_middle, *m_pre_pool_img);
			m_pre_pool_img->DownSample(m_output_img, *m_idx_map, m_pooling->m_width, m_pooling->m_height, m_pooling->m_stride_w, m_pooling->m_stride_h);
		}
		else
		{
			m_activeFunc(*m_middle, *m_output_img);
		}

		assert(m_next != nullptr);
		if (m_next->m_input->m_type != m_output->m_type)
		{
			VectorInOut* next_vec_in = dynamic_cast<VectorInOut*>(m_next->m_input);
			next_vec_in->m_value = m_output_img->Flatten();
		}

	}

	virtual void BackProp()
	{
		FullyConnectedLayer *fully_layer = dynamic_cast<FullyConnectedLayer*>(m_next);
		ConvolutionalLayer *conv_layer = dynamic_cast<ConvolutionalLayer*>(m_next);

		m_activePrimeFunc(*m_middle, *m_middle_prime);

		if (fully_layer != nullptr)
		{
			if (m_pooling != nullptr)
			{
				VectorN flatten_delta = fully_layer->m_weight->Transpose() * (*fully_layer->m_delta);
				m_pre_unpool_delta->Copy(*flatten_delta.Unflatten(m_pre_unpool_delta->Width(), m_pre_unpool_delta->Height(), m_pre_unpool_delta->Depth()));
				m_pre_unpool_delta->UpSample(m_delta, *m_idx_map, m_pooling->m_width, m_pooling->m_height, m_pooling->m_stride_w, m_pooling->m_stride_h);
				(*m_delta) ^= (*m_middle_prime);
			}
			else
			{
				VectorN flatten_delta = (fully_layer->m_weight->Transpose() * (*fully_layer->m_delta)) ^ (*(m_middle_prime->Flatten()));
				m_delta->Copy(*flatten_delta.Unflatten(m_delta->Width(), m_delta->Height(), m_delta->Depth()));
			}
		}
		else if (conv_layer != nullptr)
		{
			if (m_pooling != nullptr)
			{
				conv_layer->m_delta->ConvDepthWise(m_pre_unpool_delta, conv_layer->m_filters,
					conv_layer->m_filterDim.m_stride_w, conv_layer->m_filterDim.m_stride_h,
					Padding::Valid);
				m_pre_unpool_delta->UpSample(m_delta, *m_idx_map, m_pooling->m_width, m_pooling->m_height, m_pooling->m_stride_w, m_pooling->m_stride_h);
			}
			else
			{
				conv_layer->m_delta->ConvDepthWise(m_delta, conv_layer->m_filters,
					conv_layer->m_filterDim.m_stride_w, conv_layer->m_filterDim.m_stride_h,
					Padding::Valid);
			}
			(*m_delta) ^= (*m_middle_prime);
		}
		else
		{
			throw new std::exception("no implement!");
		}

		m_input_img->ConvDepthWise(m_dw, *m_delta, m_filterDim.m_stride_w, m_filterDim.m_stride_h, Padding::Valid);

		assert(m_filters.size() == m_bias->GetSize() &&
			m_bias->GetSize() == m_delta->Depth());

		int32_t nFilter = m_filters.size();

		for (int i = 0; i < nFilter; ++i)
		{
			(*m_bias)[i] = m_delta->SumByDepthWise(i);
		}
	}

	virtual void PreTrain()
	{
		m_sum_db->MakeZero();
		for (int32_t i = 0; i < m_filters.size(); ++i)
		{
			m_sum_dw[i]->MakeZero();
		}
	}

	virtual void SumGradient()
	{
		m_sum_db->Copy(*m_sum_db + *m_db);
		for (int32_t i = 0; i < m_filters.size(); ++i)
		{
			*m_sum_dw[i] += *m_dw[i];
		}
	}

	virtual void UpdateWeightBias(float eff)
	{
		*m_bias -= *m_sum_db * eff;
		for (int32_t i = 0; i < m_filters.size(); ++i)
		{
			*m_sum_dw[i] *= eff;
			*m_filters[i] -= *m_sum_dw[i];
		}
	}

#ifndef NDEBUG
	virtual void CheckGradient()
	{

	}
#endif

};

#endif //__CONVOLUTIONAL_LAYER_H__

