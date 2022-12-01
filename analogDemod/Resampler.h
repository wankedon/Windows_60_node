#pragma once

#include <samplerate.h>
#include <vector>
#include <memory>

class BlockResampler
{
public:
	BlockResampler(int maxInput, int converter, double ratio);
	~BlockResampler();
public:
	bool input(const std::vector<int16_t>& samples);
	bool isInputFull() const { return inputBuffer.size() == maxInput; }
	bool convert();
	std::vector<uint16_t> output() const;
	void resetBuffer();
private:
	const int maxInput;
	std::vector<float> inputBuffer;
	std::vector<float> outputBuffer;
	SRC_STATE* src_state;
	SRC_DATA src_data;
};

class StreamResampler
{
public:
	StreamResampler(int maxInput, int converter, int numChannle, double ratio);
	~StreamResampler();
public:
	bool convert(const std::vector<int16_t>& input);
	std::unique_ptr<std::vector<int16_t>> output(size_t size);
	bool output(std::vector<int16_t>& userBuffer);
public:
	bool convert(const std::vector<int32_t>& input);
	std::unique_ptr<std::vector<int32_t>> output32(size_t size);
	bool output(std::vector<int32_t>& userBuffer);
private:
	SRC_STATE* src_state;
	SRC_DATA src_data;
	const int channels;
	std::vector<float> remains;
	std::vector<float> convBuffer;
	std::vector<float> outputBuffer;
};