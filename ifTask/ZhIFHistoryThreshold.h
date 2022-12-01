#pragma once
#include "HistoryThreshold.h"
#include "ZhIFPersistence.h"

class IFHistoryThreshold : public HistoryThresholdBase
{
public:
	IFHistoryThreshold(const spectrum::FrequencyBand& wholeBand, std::shared_ptr<persistence::IFDBAccessor> accessor);
	~IFHistoryThreshold();
public:
	bool loadHistory(const detection::HistoryThresholdParam& param) override;
private:
	std::shared_ptr<persistence::IFDBAccessor> dbAccessor;
};