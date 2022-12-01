#include "pch.h"
#include "ZhTransRecognition.h"
#include "Logger.h"

ZhTransRecognition::ZhTransRecognition()
	:m_handle(0),
	m_centFreq(0),
    m_sampleRate(0),
	m_isRecognizeFinish(true),
	m_times(0)
{

}

ZhTransRecognition::~ZhTransRecognition()
{
	destroy();
}

bool ZhTransRecognition::Recognise(const senIQSegmentData& header, const std::vector<float> iqData)
{
	m_iqDataBuffer.insert(m_iqDataBuffer.end(), iqData.begin(), iqData.end());
	if (m_iqDataBuffer.size() < IQ_PAIR_COUNT * 4096*2)
	{
		return false;
	}
	if (!m_isRecognizeFinish)
		return false;
	createHandle(header.centerFrequency, header.sampleRate);
	if (m_handle == 0)
		return false;
	m_isRecognizeFinish = false;
	auto transType = (zczh::zhIFAnalysis::TransmissionType)TransmissionSystemRecognition::recognize(m_handle, m_iqDataBuffer.data(), m_iqDataBuffer.size());
	//if (m_transRes == nullptr)
	//{
		//m_transRes = std::make_unique<zczh::zhIFAnalysis::TransmissionResult>();
		m_transRes.set_trans_type(transType);
	//}
	m_iqDataBuffer.clear();
	m_isRecognizeFinish = true;
	LOG("Trans_Recognition {}", ++m_times);
	return true;
}

void ZhTransRecognition::createHandle(double centFreq, double sampleRate)
{
	if (m_handle == 0)
	{
		m_centFreq = centFreq;
		m_sampleRate = sampleRate;
		TransmissionSystemRecognition::create(&m_handle, centFreq, sampleRate);
	}
	else if (centFreq != m_centFreq || sampleRate != m_sampleRate)
	{
		destroy();
		m_centFreq = centFreq;
		m_sampleRate = sampleRate;
		TransmissionSystemRecognition::create(&m_handle, centFreq, sampleRate);
	}
}

void ZhTransRecognition::fillResult(zczh::zhIFAnalysis::Result& result)
{
	//if (m_transRes == nullptr)
		//return;
	*result.mutable_trans_result() = m_transRes;
	//m_transRes.reset();
}

void ZhTransRecognition::destroy()
{
	if (m_handle && m_isRecognizeFinish)
		TransmissionSystemRecognition::destroy(m_handle);
}