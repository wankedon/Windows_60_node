#include "pch.h"
#include "ZhAnalogDemod.h"
#include "Resampler.h"
#include "Logger.h"

ZhAnalogDemod::ZhAnalogDemod()
	:m_times(0),
	m_sampleRate(0),
	m_iqPairSize(0),
	m_isDemodFinish(true),
	m_demodType(DemodType::Demodulation_AM)
{

}

ZhAnalogDemod::~ZhAnalogDemod()
{
	
}

void ZhAnalogDemod::resetDemodType(ZhDemodType type)
{
	m_demodType = (type == Demod_none) ? Demodulation_AM : (DemodType)type;
}

bool ZhAnalogDemod::Demod(const senIQSegmentData& header, const std::vector<float> iqData)
{
	if (!m_isDemodFinish)
		return false;
	if (iqData.empty())
		return false;
	createDemod(header.sampleRate, iqData.size() / 2);
	if (m_demod == nullptr)
		return false;
	std::vector<int32_t> rawdata{ iqData.begin(), iqData.end() };
	std::vector<int16_t> demodData;
	m_demod->demod(rawdata, demodData);
	if (m_audioData.empty())
		m_audioData.resize(demodData.size());
	createResample(header.sampleRate, iqData.size() / 2);
	if (!convertResult(demodData, m_audioData))
		return false;
	zczh::zhIFAnalysis::AudioBlock block;
	block.mutable_pcm_bitstream()->CopyFrom({ m_audioData .begin(),m_audioData.end()});
	if (m_audioResult == nullptr)
		m_audioResult = std::make_unique<zczh::zhIFAnalysis::AudioResult>();
	*m_audioResult->mutable_blocks()->Add() = block;
	m_audioData.clear();
	m_isDemodFinish = true;
	CLOG("ZhAnalogDemod Demod {}", ++m_times);
	return true;
}

void ZhAnalogDemod::fillResult(zczh::zhIFAnalysis::Result& result)
{
	if (m_audioResult == nullptr)
		return;
	*result.mutable_audio_result() = *m_audioResult;
	LOG("blocks_size: {}", result.audio_result().blocks_size());
	m_audioResult.reset();
	///<码流数据归一化50搬移填充给频谱>
	std::vector<double> pcm;
	for (auto au :result.audio_result().blocks())
	{
		pcm.insert(pcm.end(), au.pcm_bitstream().begin(), au.pcm_bitstream().end());
	}
	if (pcm.empty())
		return;
	auto minval = fabs(*std::min_element(pcm.begin(), pcm.end()));
	std::transform(pcm.begin(), pcm.end(), pcm.begin(), std::bind2nd(std::plus<double>(), minval));
	auto maxval = *std::max_element(pcm.begin(), pcm.end());
	std::transform(pcm.begin(), pcm.end(), pcm.begin(), std::bind2nd(std::multiplies<double>(), 50/maxval));
	std::transform(pcm.begin(), pcm.end(), pcm.begin(), std::bind2nd(std::minus<double>(), 100));
	result.mutable_spectrum()->mutable_cur_trace()->CopyFrom({ pcm.begin(), pcm.end() });
	///</码流数据归一化50搬移填充给频谱>
}

void ZhAnalogDemod::createDemod(float sampleRate, int16_t iqPairSize)
{
	if (m_demodType == DemodType::Demodulation_none)
		return;
	if (m_demod == nullptr)
	{
		m_demod = std::make_unique<AnalogDemod>(SAMPLE_RATE, iqPairSize, m_demodType);
	}
	else if (m_iqPairSize != iqPairSize)
	{
		destroy();
		m_demod = std::make_unique<AnalogDemod>(SAMPLE_RATE, iqPairSize, m_demodType);
	    CLOG("AnalogDemod reset");	
	}
	m_iqPairSize = iqPairSize;
}

void ZhAnalogDemod::createResample(float sampleRate, int16_t iqPairSize)
{
	if (m_resampler == nullptr)
	{
		m_resampler = std::make_unique<StreamResampler>(iqPairSize, SRC_SINC_FASTEST, 1, 8000.0 / sampleRate);// 80000.0 / sampleRate
	}
	else if (m_iqPairSize != iqPairSize || m_sampleRate != sampleRate)
	{
		m_resampler.reset();
		m_resampler = std::make_unique<StreamResampler>(iqPairSize, SRC_SINC_FASTEST, 1, 8000.0 / sampleRate);
		CLOG("Resample reset");
	}
	m_sampleRate = sampleRate;
	m_iqPairSize = iqPairSize;
}

bool ZhAnalogDemod::convertResult(std::vector<int16_t>& demodRawData, std::vector<int16_t>& demodAudioData)
{
	if (m_resampler->convert(demodRawData))
	{
		return m_resampler->output(demodAudioData);
	}
	return false;
}

void ZhAnalogDemod::destroy()
{
	if (m_demod && m_isDemodFinish)
		m_demod.reset();
}