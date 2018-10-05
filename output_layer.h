#ifndef __OUTPUT_LAYER_H__
#define __OUTPUT_LAYER_H__

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

namespace mini_cnn
{
class output_layer : public fully_connected_layer
{
protected:
	lossfunc_type m_lossfunc_type;

public:
	output_layer(int_t neural_count, lossfunc_type lf_type, activation_type ac_type) : fully_connected_layer(neural_count, ac_type)
	{
		nn_assert(!(lf_type == lossfunc_type::eSigmod_CrossEntropy && ac_type != activation_type::eSigmod)
			&& !(lf_type == lossfunc_type::eSoftMax_LogLikelihood && ac_type != activation_type::eSoftMax)
			);
		m_lossfunc_type = lf_type;
	}

	void backward(const varray &label, int_t task_idx)
	{
		calc_delta(label, task_idx);

		layer_base::task_storage &ts = m_task_storage[task_idx];
		const varray &input = m_prev->output(task_idx);
		int_t out_sz = ts.m_a.size();
		int_t in_sz = input.size();

		nn_assert(m_w.dim() == 2);
		nn_assert(in_sz == m_w.width());
		nn_assert(out_sz == m_w.height());

		/*
			dw = db * input
		*/
		for (int_t i = 0; i < out_sz; ++i)
		{
			ts.m_db(i) += ts.m_delta(i);
			for (int_t j = 0; j < in_sz; ++j)
			{
				ts.m_dw(j, i) += ts.m_delta(i) * input(j);
			}
		}

		/*
			m_w : out_sz X in_sz
		*/
		for (int_t i = 0; i < in_sz; ++i)
		{
			float_t dot = 0;
			for (int_t j = 0; j < out_sz; ++j)
			{
				dot += m_w(i, j) * ts.m_delta(j);
			}
			ts.m_wd(i) = dot;
		}

		m_prev->back_prop(ts.m_wd, task_idx);

	}

	float_t calc_cost(bool check_gradient, const varray &label, int_t task_idx) const
	{
		const layer_base::task_storage &ts = m_task_storage[task_idx];
		const varray &output = ts.m_a;

		int_t out_sz = output.size();
		nn_assert(out_sz == output.size());

		float_t e = check_gradient ? 0 : cEPSILON;
		float_t cost = 0;
		switch (m_lossfunc_type)
		{
			case lossfunc_type::eMSE:
			{
				for (int_t i = 0; i < out_sz; ++i)
				{
					float_t s = output(i) - label(i);
					cost += s * s;
				}
				cost *= (float_t)(0.5);
			}
			break;
		case lossfunc_type::eSigmod_CrossEntropy:
			{
				for (int_t i = 0; i < out_sz; ++i)
				{
					float_t p = label(i); // p is only 0 or 1
					float_t q = output(i);
					float_t c = p > 0 ? -log(q + e) : -log((float_t)(1.0) - q + e);
					cost += c;
				}
			}
			break;
		case lossfunc_type::eSoftMax_LogLikelihood:
			{
				int_t idx = label.arg_max();
				cost = -log(output(idx) + e);
			}
			break;
		default:
			nn_assert(false);
			break;
		}
		return cost;
	}

private:
	const varray& calc_delta(const varray &label, int_t task_idx)
	{
		layer_base::task_storage &ts = m_task_storage[task_idx];

		int_t out_sz = label.size();
		nn_assert(out_sz == ts.m_z.size());

		switch (m_lossfunc_type)
		{
		case lossfunc_type::eMSE:
			{
				m_df(ts.m_z, ts.m_delta);
				for (int_t i = 0; i < out_sz; ++i)
				{
					ts.m_delta(i) *= ts.m_a(i) - label(i); // ���������ʧ���������������ֵ��ƫ����
				}
			}
			break;
		case lossfunc_type::eSigmod_CrossEntropy:
			{
				// J = -[t * ln(y) + (1 - t) * ln(1 - y)]      : t is the label; y is network output
				// ������CrossEntropy��ʧ������Sigmod���������ϣ�
				// ��ʧ�����������в��ƫ�����뼤����ĵ���ǡ���޹�
				// ref�� http://neuralnetworksanddeeplearning.com/chap3.html#introducing_the_cross-entropy_cost_function
				for (int_t i = 0; i < out_sz; ++i)
				{
					ts.m_delta(i) = ts.m_a(i) - label(i);
				}
			}
			break;
		case lossfunc_type::eSoftMax_LogLikelihood:
			{
				// J = -t * ln(y)     : t is the label; y is network output
				// LogLikelihood��ʧ������Softmax�����������£�
				// ��ʧ�����������в��ƫ�����뼤����ĵ���ǡ���޹�
				// delta(i) = output(k) - 1    (i==kʱ�� k��one-hot��ǩ��Ӧ��index)
				//          = 0                (i!=kʱ)
				// ref�� https://www.cnblogs.com/ZJUT-jiangnan/p/5489047.html
				int_t idx = label.arg_max();
				for (int_t i = 0; i < out_sz; ++i)
				{
					ts.m_delta(i) = ts.m_a(i);
				}
				ts.m_delta(idx) = ts.m_a(idx) - cOne;
			}
			break;
		default:
			nn_assert(false);
			break;
		}
		return ts.m_delta;
	}

};
}

#endif //__OUTPUT_LAYER_H__
