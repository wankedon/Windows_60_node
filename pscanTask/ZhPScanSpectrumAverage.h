#pragma once
#include "ZhSensorDef.h"

class ZhPScanSpectrumAverage
{
public:
	ZhPScanSpectrumAverage(const uint32_t& averageTimes);
	~ZhPScanSpectrumAverage();
public:
	senVecFloat getAverageData(const senVecFloat& rawdata);
private:
	senVecFloat LogarVecTolinearVec(const senVecFloat& raw);
	senVecFloat LinearVecTologarVec(const senVecFloat& raw);
	float logarToLinear(const float& raw);
	float LinearTologar(const float& raw);
	void initBaseLine();
	void updateBaseLine();
private:
	uint32_t m_averageTimes;
	senVecFloat m_baseLine;
	std::list<senVecFloat> m_buffer;
private:
	const uint32_t MIN_AVERAGE_TIMES = 2;
};