#include "pch.h"
#include "ZhIFHistoryThreshold.h"

IFHistoryThreshold::IFHistoryThreshold(const spectrum::FrequencyBand& wholeBand, std::shared_ptr<persistence::IFDBAccessor> accessor)
	:HistoryThresholdBase(wholeBand),
	dbAccessor(accessor)
{

}

IFHistoryThreshold::~IFHistoryThreshold() = default;

bool IFHistoryThreshold::loadHistory(const detection::HistoryThresholdParam& param)
{
	auto startTime = timestampToUINT64(param.span().start_time());
	auto stopTime = timestampToUINT64(param.span().stop_time());
	if (stopTime <= startTime)
		return false;
	auto span = bandSpan(getBand());
	auto points = frequencyBandPoints(getBand());
	auto centerFreq = static_cast<int64_t>(span.start_freq() + span.stop_freq()) / 2;
	auto bandwidth = static_cast<int64_t>(span.stop_freq() - span.start_freq());

	auto taskIds = dbAccessor->findMatchTask(centerFreq, bandwidth, points, startTime, stopTime);
	if (taskIds.empty())
		return false;
	persistence::IFDataItemHandler handler = [this](const persistence::IFDataItem& item) {thresholdLine += item.spectrum; };
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
