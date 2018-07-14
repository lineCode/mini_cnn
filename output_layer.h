#ifndef __OUTPUT_LAYER_H__
#define __OUTPUT_LAYER_H__

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
#include "fullyconnected_layer.h"


class OutputLayer : public FullyConnectedLayer
{
protected:
	const VectorN *m_label;  // ѧϰ����ȷֵ
	eLossFunc m_lossFuncType;
public:
	OutputLayer(unsigned int neuralCount, eLossFunc lossFunc, eActiveFunc act) : FullyConnectedLayer(neuralCount, act)
	{
		assert(!(lossFunc == eLossFunc::eSigmod_CrossEntropy && act != eActiveFunc::eSigmod)
			&& !(lossFunc == eLossFunc::eSoftMax_LogLikelihood && act != eActiveFunc::eSoftMax)
			);
		m_lossFuncType = lossFunc;
	}

	void SetLabelValue(const VectorN &label)
	{
		m_label = &label;
	}

	virtual void BackProp()
	{
		switch (m_lossFuncType)
		{
		case eLossFunc::eMSE:
		{
			m_activePrimeFunc(*m_middle, *m_outputPrime);
			m_delta->Copy(MseDerive() ^ (*m_outputPrime));
			m_dw->Copy(*m_delta * *m_input);
		}
			break;
		case eLossFunc::eSigmod_CrossEntropy:
		case eLossFunc::eSoftMax_LogLikelihood:
			// ������CrossEntropy��ʧ������Sigmod���������� ���� LogLikelihood��ʧ������Softmax�����������£�
			// ��ʧ�����������в��ƫ�����뼤����ĵ���ǡ���޹�
			// ref�� http://neuralnetworksanddeeplearning.com/chap3.html#introducing_the_cross-entropy_cost_function
			m_delta->Copy(*m_output - *m_label);
			m_dw->Copy(*m_delta * *m_input);
			break;
		default:
			assert(false);
			break;
		}
	}

	// ���������ʧ���������������ֵ��ƫ����
	VectorN MseDerive()
	{
		return *m_output - *m_label;
	}

};

#endif //__OUTPUT_LAYER_H__
