#include "pch.h"
#include "ZhPScanSpectrumAverage.h"

ZhPScanSpectrumAverage::ZhPScanSpectrumAverage(const uint32_t& averageTimes)
	:m_averageTimes(averageTimes)
{

}

ZhPScanSpectrumAverage::~ZhPScanSpectrumAverage()
{
	m_buffer.clear();
}

senVecFloat ZhPScanSpectrumAverage::getAverageData(const senVecFloat& rawdata)
{
	if (m_averageTimes < MIN_AVERAGE_TIMES)
		return rawdata;
	m_buffer.emplace_back(LogarVecTolinearVec(rawdata));
	if (m_buffer.size() < m_averageTimes)
		return senVecFloat{};
	if (m_baseLine.empty())
	{
		initBaseLine();
	}
	else
	{
		updateBaseLine();
	}
	if (m_buffer.size() >= m_averageTimes + 1)
		m_buffer.pop_front();
	return LinearVecTologarVec(m_baseLine);
}

senVecFloat ZhPScanSpectrumAverage::LogarVecTolinearVec(const senVecFloat& raw)
{
	senVecFloat res;
	for (auto ra : raw)
	{
		res.emplace_back(logarToLinear(ra));
	}
	return res;
}

void ZhPScanSpectrumAverage::initBaseLine()
{
	m_baseLine.resize(m_buffer.front().size());
	for (auto frame : m_buffer)
	{
		std::transform(frame.begin(), frame.end(), m_baseLine.begin(), m_baseLine.begin(), std::plus<>());
	}
	std::transform(m_baseLine.begin(), m_baseLine.end(), m_baseLine.begin(), std::bind2nd(std::multiplies<float>(), 1.0 / m_averageTimes));
}

void ZhPScanSpectrumAverage::updateBaseLine()
{
	senVecFloat averageData(m_buffer.front().size());
	std::transform(m_buffer.front().begin(), m_buffer.front().end(), averageData.begin(), averageData.begin(), std::plus<>());
	std::transform(m_buffer.back().begin(), m_buffer.back().end(), averageData.begin(), averageData.begin(), std::minus<>());
	std::transform(averageData.begin(), averageData.end(), averageData.begin(), std::bind2nd(std::multiplies<float>(), 1.0 / m_averageTimes));
	std::transform(averageData.begin(), averageData.end(), m_baseLine.begin(), m_baseLine.begin(), std::plus<>());
}

senVecFloat ZhPScanSpectrumAverage::LinearVecTologarVec(const senVecFloat& raw)
{
	senVecFloat res;
	for (auto ra : raw)
	{
		res.emplace_back(LinearTologar(ra));
	}
	return res;
}

float ZhPScanSpectrumAverage::logarToLinear(const float& raw)
{
	return pow(10, raw / 20.0);
}

float ZhPScanSpectrumAverage::LinearTologar(const float& raw)
{
	return 20 * log10(raw);
}
