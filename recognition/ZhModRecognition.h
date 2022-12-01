#pragma once
#include "ZhSensorDef.h"
#include "ModulationRecognition.h"
#include "node/modulation.pb.h"
#include "node/zczh/ZhIFAnalysis.pb.h"

class ZhModRecognition
{
public:
	ZhModRecognition();
	~ZhModRecognition();
public:
	bool Recognise(const senIQSegmentData& header, const std::vector<float> iqData);
	void fillResult(zczh::zhIFAnalysis::Result& result);
	void resetTimes() { m_times = 0; }
private:
	void createHandle(double centFreq, double sampleRate);
	void destroy();
private:
	uint32_t m_handle;
	double m_centFreq;
	double m_sampleRate;
	std::vector<float> m_iqDataBuffer;
	//std::unique_ptr<zb::dcts::node::modulation::RecognizeResult> m_recRes;
	zb::dcts::node::modulation::RecognizeResult m_recRes;
	bool m_isRecognizeFinish;
	int m_times;
private:
	const int IQ_PAIR_COUNT = 30;
};