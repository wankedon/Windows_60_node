#pragma once
#include "ZhSensorDef.h"
#include "TransmissionSystemRecognition.h"
#include "node/modulation.pb.h"
#include "node/zczh/ZhIFAnalysis.pb.h"

using namespace zb::dcts::node::modulation;

class ZhTransRecognition
{
public:
	ZhTransRecognition();
	~ZhTransRecognition();
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
	//std::unique_ptr<zczh::zhIFAnalysis::TransmissionResult> m_transRes;
	zczh::zhIFAnalysis::TransmissionResult m_transRes;
	bool m_isRecognizeFinish;
	std::vector<float> m_iqDataBuffer;
	int m_times;
private:
	const int IQ_PAIR_COUNT = 10;

};