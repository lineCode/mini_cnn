#ifndef __FULLYCONNECTED_LAYER_H__
#define __FULLYCONNECTED_LAYER_H__

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

class FullyConnectedLayer : public LayerBase
{
public:
	MatrixMN *m_weight;  // m_weight[i][j] : ǰһ��ĵ�j����Ԫ����ǰ��ĵ�i����Ԫ������Ȩ��
	VectorN *m_bias;     // m_bias[i] : ��ǰ��ĵ�i����Ԫ��ƫ��
	VectorN *m_middle;	// middle value


	VectorN *m_delta;	// equal to dJ/d(bias)
	MatrixMN *m_dw;		// equal to dJ/d(w)

	VectorN *m_sum_delta;
	MatrixMN *m_sum_dw;

protected:
	eActiveFunc m_activeFuncType;
	ActiveFunc m_activeFunc;
	ActiveFunc m_activePrimeFunc;

public:
	FullyConnectedLayer(uint32_t neuralCount, eActiveFunc act) : LayerBase(neuralCount)
	{
		m_bias = new VectorN(neuralCount);
		m_middle = new VectorN(neuralCount);
		m_output = new VectorN(neuralCount);

		m_activeFuncType = act;
		switch (act)
		{
		case eActiveFunc::eSigmod:
			m_activeFunc = Sigmoid;
			m_activePrimeFunc = SigmoidPrime;
			break;
		case eActiveFunc::eRelu:
			m_activeFunc = Relu;
			m_activePrimeFunc = ReluPrime;
			break;
		case eActiveFunc::eSoftMax:
			m_activeFunc = Softmax;
			break;
		default:
			break;
		}
	}

	virtual void Connect(LayerBase *prev)
	{

		m_weight = new MatrixMN(this->Size(), prev->Size());

		m_delta = new VectorN(this->Size());
		m_dw = new MatrixMN(this->Size(), prev->Size());

		m_sum_delta = new VectorN(this->Size());
		m_sum_dw = new MatrixMN(this->Size(), prev->Size());

		this->m_input = prev->m_output;

	}

	virtual void Init(NormalRandom nrand)
	{
		for (unsigned int i = 0; i < m_weight->GetRowCount(); ++i)
		{
			for (unsigned int j = 0; j < m_weight->GetColCount(); ++j)
			{
				(*m_weight)(i, j) = nrand.GetRandom();
			}
		}

		for (unsigned int i = 0; i < m_bias->GetSize(); ++i)
		{
			(*m_bias)[i] = nrand.GetRandom();
		}	
	}

	virtual void Forward()
	{
		m_middle->Copy(*m_weight * *m_input + *m_bias);
		m_activeFunc(*m_middle, *m_output);
	}

	virtual void BackProp(LayerBase *next)
	{
		FullyConnectedLayer *fc = dynamic_cast<FullyConnectedLayer*>(next);
		m_activePrimeFunc(*m_middle, *m_outputPrime);
		m_delta->Copy((fc->m_weight->Transpose() * (*fc->m_delta)) ^ (*m_outputPrime));
		m_dw->Copy(*m_delta * *m_input);
	}

	virtual void PreTrain()
	{
		m_sum_delta->MakeZero();
		m_sum_dw->MakeZero();
	}

	virtual void SumGradient()
	{
		m_sum_delta->Copy(*m_sum_delta + *m_delta);
		m_sum_dw->Copy(*m_sum_dw + *m_dw);
	}

	virtual void UpdateWeightBias(float eff)
	{	
		*m_weight -= *m_sum_dw * eff;
		*m_bias -= *m_sum_delta * eff;
	}
};

#endif //__FULLYCONNECTED_LAYER_H__
