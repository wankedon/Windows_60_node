#pragma once
#include "HistoryThreshold.h"
#include "ZhDFPersistence.h"

class DFHistoryThreshold : public HistoryThresholdBase
{
public:
	DFHistoryThreshold(const spectrum::FrequencyBand& wholeBand, std::shared_ptr<persistence::DFDBAccessor> accessor);
	~DFHistoryThreshold();
public:
	bool loadHistory(const detection::HistoryThresholdParam& param) override;
private:
	std::shared_ptr<persistence::DFDBAccessor> dbAccessor;
};