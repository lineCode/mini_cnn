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
#include "fully_connected_layer.h"

namespace mini_cnn
{
class OutputLayer : public FullyConnectedLayer
{
protected:
	const VectorN *m_label;  // ѧϰ����ȷֵ
	eLossFunc m_loss_func_type;
public:
	OutputLayer(unsigned int neuralCount, eLossFunc lossFunc, eActiveFunc act) : FullyConnectedLayer(neuralCount, act)
	{
		assert(!(lossFunc == eLossFunc::eSigmod_CrossEntropy && act != eActiveFunc::eSigmod)
			&& !(lossFunc == eLossFunc::eSoftMax_LogLikelihood && act != eActiveFunc::eSoftMax)
			);
		m_loss_func_type = lossFunc;
	}

	void SetLabelValue(const VectorN &label)
	{
		m_label = &label;
	}

	virtual void BackProp()
	{
		switch (m_loss_func_type)
		{
		case eLossFunc::eMSE:
			{
				m_derived_func(*m_middle);
				*m_delta += MseDerive() ^ (*m_middle);
				*m_dw += *m_delta * GetInput();
			}
			break;
		case eLossFunc::eSigmod_CrossEntropy:
			{
				// J = -[t * ln(y) + (1 - t) * ln(1 - y)]      : t is the label; y is network output
				// ������CrossEntropy��ʧ������Sigmod���������ϣ�
				// ��ʧ�����������в��ƫ�����뼤����ĵ���ǡ���޹�
				// ref�� http://neuralnetworksanddeeplearning.com/chap3.html#introducing_the_cross-entropy_cost_function
				*m_delta += GetOutput() - *m_label;
				*m_dw += *m_delta * GetInput();
			}
			break;
		case eLossFunc::eSoftMax_LogLikelihood:
			{
				// J = -t * ln(y)     : t is the label; y is network output
				// LogLikelihood��ʧ������Softmax�����������£�
				// ��ʧ�����������в��ƫ�����뼤����ĵ���ǡ���޹�
				// delta(i) = output(k) - 1    (i==kʱ�� k��one-hot��ǩ��Ӧ��index)
				//          = 0                (i!=kʱ)
				// ref�� https://www.cnblogs.com/ZJUT-jiangnan/p/5489047.html
				int idx = m_label->ArgMax();
				const VectorN &output = GetOutput();
				Int len = static_cast<Int>(m_delta->GetSize());
				for (Int i = 0; i < len; ++i)
				{
					(*m_delta)[i] += (i == idx) ? (output[i] - (Float)(1.0)) : output[i];
				}
				*m_dw += *m_delta * GetInput();
			}
			break;
		default:
			assert(false);
			break;
		}
	}

	// ���������ʧ���������������ֵ��ƫ����
	VectorN MseDerive()
	{
		return GetOutput() - *m_label;
	}

	Float GetCost(bool check_gradient)
	{
		Float e = check_gradient ? 0 : cEPSILON;
		Float cost = 0;
		switch (m_loss_func_type)
		{
		case eLossFunc::eMSE:
			{
				VectorN diff = GetOutput() - *m_label;
				cost = (Float)(0.5) * diff.SquareMagnitude();
			}
			break;
		case eLossFunc::eSigmod_CrossEntropy:
			{							
				const VectorN &ov = GetOutput();
				int len = static_cast<int>(ov.GetSize());
				for (Int i = 0; i < len; ++i)
				{
					Float p = (*m_label)[i]; // p is only 0 or 1
					Float q = ov[i];
					Float c = p > 0 ? -log(q + e) : -log((Float)(1.0) - q + e);
					cost += c;
				}
			}
			break;
		case eLossFunc::eSoftMax_LogLikelihood:
			{
				const VectorN &ov = GetOutput();
				Int idx = m_label->ArgMax();
				cost = -log(ov[idx] + e);
			}
			break;
		default:
			assert(false);
			break;
		}
		return cost;
	}

};
}

#endif //__OUTPUT_LAYER_H__
