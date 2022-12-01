#include "pch.h"
#include "Resampler.h"
#include <assert.h>

using namespace std;

static int calcOutputBufferSize(int maxInput, double ratio)
{
	return (int)ceil(maxInput * ratio) + 1;
}

BlockResampler::BlockResampler(int maxInput, int converter, double ratio)
	:maxInput(maxInput),
	 outputBuffer(calcOutputBufferSize(maxInput, ratio)),
	 src_state(nullptr)
{
	inputBuffer.reserve(maxInput);
	src_data.src_ratio = ratio;
	src_data.end_of_input = false;
	int error;
	src_state = src_new(converter, 2, &error);	//2 channel
	assert(src_state);
}

BlockResampler::~BlockResampler()
{
	src_delete(src_state);
}

bool BlockResampler::input(const std::vector<int16_t>& samples)
{
	if (inputBuffer.size() + samples.size() > maxInput)
		return false;
	std::vector<float> temp(samples.size());
	src_short_to_float_array(samples.data(), temp.data(), samples.size());
	inputBuffer.insert(inputBuffer.end(), temp.begin(), temp.end());
	return true;
}



bool BlockResampler::convert()
{
	src_data.data_in = inputBuffer.data();
	src_data.input_frames = inputBuffer.size() / 2;
	src_data.data_out = outputBuffer.data();
	src_data.output_frames = outputBuffer.size() / 2;
	auto error = src_process(src_state, &src_data);
	if (error)
		return false;
	return true;
}

std::vector<uint16_t> BlockResampler::output() const
{
	std::vector<uint16_t> result(src_data.output_frames_gen);
	for (int i = 0; i < src_data.output_frames_gen * 2; i += 2)
	{
		float mag = sqrt(outputBuffer[i] * outputBuffer[i] + outputBuffer[i + 1] * outputBuffer[i + 1]);
		mag *= 65535;
		if (mag > 65535.0)
			mag = 65535.0;
		result[i / 2] = (uint16_t)mag;
	}
	return result;
}

void BlockResampler::resetBuffer()
{
	inputBuffer.clear();
}

StreamResampler::StreamResampler(int maxInput, int converter, int numChannle, double ratio)
	:convBuffer(calcOutputBufferSize(maxInput, ratio)),
	src_state(nullptr),
	channels(numChannle)
{
	src_data.src_ratio = ratio;
	src_data.end_of_input = false;
	int error;
	src_state = src_new(converter, channels, &error);
	assert(src_state);
	remains.reserve(16384);
	outputBuffer.reserve(16384 * 4);
}

StreamResampler::~StreamResampler()
{
	src_delete(src_state);
}

bool StreamResampler::convert(const std::vector<int16_t>& input)
{
	vector<float> temp(input.size());
	src_short_to_float_array(input.data(), temp.data(), input.size());
	remains.insert(remains.end(), temp.begin(), temp.end());
	src_data.data_in = remains.data();
	src_data.input_frames = remains.size() / channels;
	src_data.data_out = convBuffer.data();
	src_data.output_frames = convBuffer.size() / channels;
	auto error = src_process(src_state, &src_data);
	if (error)
		return false;
	remains.erase(remains.begin(), remains.begin() + src_data.input_frames_used * channels);
	outputBuffer.insert(outputBuffer.end(), convBuffer.begin(), convBuffer.begin() + src_data.output_frames_gen * channels);
	return true;
}

std::unique_ptr<vector<int16_t>> StreamResampler::output(size_t size)
{
	if (outputBuffer.size() < size)
		return nullptr;
	std::unique_ptr<vector<int16_t>> result = make_unique<vector<int16_t>>(size);//outputBuffer.begin(), outputBuffer.begin() + size);
	src_float_to_short_array(outputBuffer.data(), result->data(), size);
	outputBuffer.erase(outputBuffer.begin(), outputBuffer.begin() + size);
	return result;
}

bool StreamResampler::output(std::vector<int16_t>& userBuffer)
{
	if (outputBuffer.size() < userBuffer.size())
		return false;
	src_float_to_short_array(outputBuffer.data(), userBuffer.data(), userBuffer.size());
	outputBuffer.erase(outputBuffer.begin(), outputBuffer.begin() + userBuffer.size());
	return true;
}

//32λ
bool StreamResampler::convert(const std::vector<int32_t>& input)
{
	vector<float> temp(input.size());
	src_int_to_float_array(input.data(), temp.data(), input.size());
	remains.insert(remains.end(), temp.begin(), temp.end());
	src_data.data_in = remains.data();
	src_data.input_frames = remains.size() / channels;
	src_data.data_out = convBuffer.data();
	src_data.output_frames = convBuffer.size() / channels;
	auto error = src_process(src_state, &src_data);
	if (error)
		return false;
	remains.erase(remains.begin(), remains.begin() + src_data.input_frames_used * channels);
	outputBuffer.insert(outputBuffer.end(), convBuffer.begin(), convBuffer.begin() + src_data.output_frames_gen * channels);
	return true;
}

std::unique_ptr<std::vector<int32_t>> StreamResampler::output32(size_t size)
{
	if (outputBuffer.size() < size)
		return nullptr;
	std::unique_ptr<vector<int32_t>> result = make_unique<vector<int32_t>>(size);//outputBuffer.begin(), outputBuffer.begin() + size);
	src_float_to_int_array(outputBuffer.data(), result->data(), size);
	outputBuffer.erase(outputBuffer.begin(), outputBuffer.begin() + size);
	return result;
}

bool StreamResampler::output(std::vector<int32_t>& userBuffer)
{
	if (outputBuffer.size() < userBuffer.size())
		return false;
	src_float_to_int_array(outputBuffer.data(), userBuffer.data(), userBuffer.size());
	outputBuffer.erase(outputBuffer.begin(), outputBuffer.begin() + userBuffer.size());
	return true;
}