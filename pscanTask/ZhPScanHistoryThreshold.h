#pragma once

#include "HistoryThreshold.h"
#include "ZhPScanPersistence.h"

class PScanHistoryThreshold : public HistoryThresholdBase
{
public:
	PScanHistoryThreshold(const spectrum::FrequencyBand& wholeBand, std::shared_ptr<persistence::PScanDBAccessor> accessor);
	~PScanHistoryThreshold();
public:
	bool loadHistory(const detection::HistoryThresholdParam& param) override;
private:
	std::shared_ptr<persistence::PScanDBAccessor> dbAccessor;
};