#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <cassert>
#include <cmath>
#include <random>
#include <algorithm>

#include "mini_cnn.h"

using namespace std;
using namespace mini_cnn;

Network CreateFCN()
{
	Network nn;
	nn.AddLayer(new InputLayer(N_inputCount));
	nn.AddLayer(new FullyConnectedLayer(100, eActiveFunc::eRelu));
	nn.AddLayer(new FullyConnectedLayer(30, eActiveFunc::eRelu));
	nn.AddLayer(new OutputLayer(C_classCount, eLossFunc::eSoftMax_LogLikelihood, eActiveFunc::eSoftMax));
	return nn;
}

Network CreateCNN()
{
	Network nn;
	nn.AddLayer(new InputLayer(W_input, H_input, D_input));
	nn.AddLayer(new ConvolutionalLayer(4, new FilterDimension(3, 3, 1, 0, 1, 1), new Pooling(2, 2, 0, 2, 2), eActiveFunc::eRelu));
	nn.AddLayer(new ConvolutionalLayer(16, new FilterDimension(3, 3, 4, 0, 1, 1), new Pooling(2, 2, 0, 2, 2), eActiveFunc::eRelu));
	nn.AddLayer(new FullyConnectedLayer(30, eActiveFunc::eRelu));
	nn.AddLayer(new OutputLayer(C_classCount, eSoftMax_LogLikelihood, eActiveFunc::eSoftMax));
	//nn.AddLayer(new OutputLayer(C_classCount, eLossFunc::eMSE, eActiveFunc::eSigmod));
	return nn;
}

std::mt19937_64 CreateRandom()
{
	// random init
	uInt seed = GetNow();

	cout << "random seed:" << seed << endl;

	std::mt19937_64 generator(seed);

	return generator;
}

int main()
{
	std::vector<VectorN*> img_vec;
	std::vector<VectorN*> lab_vec;
	std::vector<VectorN*> test_img_vec;
	std::vector<int> test_lab_vec;
	ReadDataSet(img_vec, lab_vec, test_img_vec, test_lab_vec);

	int img_count = img_vec.size();
	int test_img_count = test_img_vec.size();

	long long t0 = GetNow();
	auto generator = CreateRandom();
	NormalRandom nrand(generator, 0, 1.0);

	// define neural network
	Network nn = CreateCNN();

	nn.Init(nrand);

	//Float lrs[] = { 0.0001, 0.0003, 0.001, 0.003, 0.01, 0.03, 0.1 };
	//Int l_idx = 0;
	Float init_learning_rate = 0.1;	
	int epoch = 100;
	int batch_size = 600;
	int batch = img_count / batch_size;
	std::vector<int> idx_vec(img_count);
	for (int k = 0; k < img_count; ++k)
	{
		idx_vec[k] = k;
	}

	float kef = 0.2f;
	float maxCorrectRate = 0;
	for (int c = 0; c < epoch; ++c)
	{
		Float learning_rate = init_learning_rate;
		std::shuffle(std::begin(idx_vec), std::end(idx_vec), generator);
		std::vector<VectorN*> batch_img_vec(batch_size);
		std::vector<VectorN*> batch_label_vec(batch_size);
		for (int i = 0; i <= batch; ++i)
		{
			for (int k = 0; k < batch_size; ++k)
			{
				int j = idx_vec[(i * batch_size + k) % img_count];
				batch_img_vec[k] = img_vec[j];
				batch_label_vec[k] = lab_vec[j];
			}

			//if (l_idx < sizeof(lrs) / sizeof(lrs[0]))
			//{
			//	learning_rate = lrs[l_idx++];
			//}

			learning_rate = init_learning_rate / (1.0 + kef * i);
			learning_rate = std::max(1e-4, learning_rate);

			nn.SGD(batch_img_vec, batch_label_vec, learning_rate);

			Float ca = nn.CalcCost(batch_img_vec, batch_label_vec);
			std::cout << "learning_rate:" << learning_rate << "\tcost: " << ca << endl;

			if (i % (batch / 5) == 0)
			{
				std::cout << "batch: " << i << "/" << batch << endl;
			}
		}

		uInt correct = nn.Test(test_img_vec, test_lab_vec);
		float correct_rate = (1.0f * correct / test_img_count);
		if (correct_rate > maxCorrectRate)
		{
			maxCorrectRate = correct_rate;
		}

		Float tot_cost = nn.CalcCost(img_vec, lab_vec);

		std::cout << "epoch " << c << ": " << correct_rate << " (" << correct << " / " << test_img_count << ")" << "\t tot_cost = " << tot_cost << endl;
	}

	cout << "Max CorrectRate: " << maxCorrectRate << endl;

	long long t1 = GetNow();

	float timeCost = (t1 - t0) * 0.001f;
	cout << "TimeCost: " << timeCost << "(s)" << endl;

	system("pause");
	return 0;

}

