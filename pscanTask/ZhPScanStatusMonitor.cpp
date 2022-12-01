#include "pch.h"
#include "ZhPScanStatusMonitor.h"

using namespace std;

ZhPScanStatusMonitor::ZhPScanStatusMonitor(int32_t r, int32_t s)
	:resultInterval(r),
	statusInterval(std::max({r, s, 100}))
{
}

ZhPScanStatusMonitor::~ZhPScanStatusMonitor()
{

}

void ZhPScanStatusMonitor::onSweepBegin(const Timestamp& ts)
{
	if (!header.time_span().has_start_time())
	{
		*header.mutable_time_span()->mutable_start_time() = ts;
	}
	if (!status.time_span().has_start_time())
	{
		*status.mutable_time_span()->mutable_start_time() = ts;
	}
}

void ZhPScanStatusMonitor::onSweepEnd(const Timestamp& ts, const Position& pos)
{
	*header.mutable_time_span()->mutable_stop_time() = ts;
	*header.mutable_device_position() = pos;
	header.set_sweep_count(header.sweep_count() + 1);
	*status.mutable_time_span()->mutable_stop_time() = ts;
	status.set_total_sweep_count(status.total_sweep_count() + 1);
}

bool ZhPScanStatusMonitor::needResultReport() const
{
	return resultTimer.isLargerThan(resultInterval);
}

bool ZhPScanStatusMonitor::needStatusReport() const
{
	return statusTimer.isLargerThan(statusInterval);
}

void ZhPScanStatusMonitor::setupForNextResult()
{
	header.set_sequence_number(header.sequence_number() + 1);
	header.clear_sweep_count();
	header.clear_time_span();
	restartResultTimer();
}

void ZhPScanStatusMonitor::restartResultTimer()
{
	resultTimer.restart();
}

void ZhPScanStatusMonitor::restartStatusTimer()
{
	statusTimer.restart();
}