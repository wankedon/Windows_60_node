#include "pch.h"
#include "ZhModRecognition.h"
#include "Logger.h"

ZhModRecognition::ZhModRecognition()
	:m_handle(0),
	m_centFreq(0),
    m_sampleRate(0),
	m_isRecognizeFinish(true),
	m_times(0)
{

}

ZhModRecognition::~ZhModRecognition()
{
	destroy();
}

bool ZhModRecognition::Recognise(const senIQSegmentData& header, const std::vector<float> iqData)
{
	m_iqDataBuffer.insert(m_iqDataBuffer.end(), iqData.begin(), iqData.end());
	if (m_iqDataBuffer.size() < IQ_PAIR_COUNT * 4096*2)
	{
		//m_iqDataBuffer.insert(m_iqDataBuffer.end(), iqData.begin(), iqData.end());
		return false;
	}
	if (!m_isRecognizeFinish)
		return false;
	createHandle(header.centerFrequency, header.sampleRate);
	if (m_handle == 0)
		return false;
	m_isRecognizeFinish = false;
	auto sigDes = ModulationRecognition::recognize(m_handle, m_iqDataBuffer.data(), m_iqDataBuffer.size());
	//if (m_recRes == nullptr)
	{
		//m_recRes = std::make_unique<zb::dcts::node::modulation::RecognizeResult>();
		m_recRes.set_center_freq(sigDes.centFreq);
		m_recRes.set_band_width(sigDes.band);
		m_recRes.set_mod_type((zb::dcts::node::modulation::ModType)sigDes.modFormat);
		m_recRes.set_signal_type((zb::dcts::node::modulation::SigType)(sigDes.sigFormat - 100));
		m_iqDataBuffer.clear();
	}
	m_isRecognizeFinish = true;
	LOG("Mod_Recognition {}", ++m_times);
	return true;
}

void ZhModRecognition::createHandle(double centFreq, double sampleRate)
{
	if (m_handle == 0)
	{
		m_centFreq = centFreq;
		m_sampleRate = sampleRate;
		ModulationRecognition::create(&m_handle, centFreq, sampleRate);
	}
	else if(centFreq != m_centFreq || sampleRate != m_sampleRate)
	{
		destroy();
		m_centFreq = centFreq;
		m_sampleRate = sampleRate;
		ModulationRecognition::create(&m_handle, centFreq, sampleRate);
	}
}

void ZhModRecognition::fillResult(zczh::zhIFAnalysis::Result& result)
{
	//if (m_recRes == nullptr)
		//return;
	*result.mutable_recognize_result() = m_recRes;
	//m_recRes.reset();
}

void ZhModRecognition::destroy()
{
	if (m_handle && m_isRecognizeFinish)
		ModulationRecognition::destroy(m_handle);
}