#pragma once
#include "ZhSensorDef.h"
#include "AnalogDemod.h"
#include "node/zczh/ZhIFAnalysis.pb.h"

enum ZhDemodType
{
	Demod_none = 0,
	Demod_AM = 1,
	Demod_FM = 2,
	Demod_CW = 3,
	Demod_LSB = 4,
	Demod_USB = 5,
};

class StreamResampler;
class ZhAnalogDemod
{
public:
	ZhAnalogDemod();
	~ZhAnalogDemod();
public:
	void resetDemodType(ZhDemodType type);
	bool Demod(const senIQSegmentData& header, const std::vector<float> iqData);
	void fillResult(zczh::zhIFAnalysis::Result& result);
	void resetTimes() { m_times = 0; }
private:
	void createDemod(float sampleRate, int16_t numTransferSamples);
	void createResample(float sampleRate, int16_t iqDataSize);
	bool convertResult(std::vector<int16_t>& demodRawData, std::vector<int16_t>& demodAudioData);
	void destroy();

private:
	int m_times;
	double m_sampleRate;
	int16_t m_iqPairSize;
	bool m_isDemodFinish;
	DemodType m_demodType;
	std::unique_ptr<AnalogDemod> m_demod;
	std::unique_ptr<zczh::zhIFAnalysis::AudioResult> m_audioResult;
	std::unique_ptr<StreamResampler> m_resampler;
	std::vector<int16_t> m_audioData;
private:
	const float SAMPLE_RATE = 28e6;
};