#include "pch.h"
#include "ZhPScanHistoryThreshold.h"

PScanHistoryThreshold::PScanHistoryThreshold(const spectrum::FrequencyBand& wholeBand, std::shared_ptr<persistence::PScanDBAccessor> accessor)
	:HistoryThresholdBase(wholeBand),
	dbAccessor(accessor)
{

}

PScanHistoryThreshold::~PScanHistoryThreshold() = default;

bool PScanHistoryThreshold::loadHistory(const detection::HistoryThresholdParam& param)
{
	auto startTime = timestampToUINT64(param.span().start_time());
	auto stopTime = timestampToUINT64(param.span().stop_time());
	if (stopTime <= startTime)
		return false;
	auto span = bandSpan(getBand());
	auto points = frequencyBandPoints(getBand());
	auto startFreq = static_cast<int64_t>(span.start_freq());
	auto stopFreq = static_cast<int64_t>(span.stop_freq());
	auto taskIds = dbAccessor->findMatchTask(startFreq, stopFreq, points, startTime, stopTime);
	if (taskIds.empty())
		return false;
	persistence::PScanDataItemHandler handler = [this](const persistence::PScanDataItem& item) {thresholdLine += item.data; };
	auto count = dbAccessor->useDataInSpan(taskIds, param.span(), param.max_load_count(), handler);
	if (count)
	{
		thresholdLine.offset(param.offset());
		updateLoadedCount(count);
		return true;
	}
	else
	{
		return false;
	}
}
