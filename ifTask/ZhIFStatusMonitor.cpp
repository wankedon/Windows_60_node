#include "pch.h"
#include "ZhIFStatusMonitor.h"

using namespace std;

IFStatusMonitor::IFStatusMonitor(int32_t r, int32_t s)
	:resultInterval(r),
	statusInterval(std::max({r, s, 100}))
{
}

IFStatusMonitor::~IFStatusMonitor()
{

}

void IFStatusMonitor::onSweepEnd(const Timestamp& ts, const Position& pos)
{
	if (!header.time_span().has_start_time())
	{
		*header.mutable_time_span()->mutable_start_time() = ts;
	}
	if (!status.time_span().has_start_time())
	{
		*status.mutable_time_span()->mutable_start_time() = ts;
	}
	*header.mutable_time_span()->mutable_stop_time() = ts;
	*header.mutable_device_position() = pos;
	header.set_sweep_count(header.sweep_count() + 1);
	*status.mutable_time_span()->mutable_stop_time() = ts;
	status.set_total_sweep_count(status.total_sweep_count() + 1);
}

bool IFStatusMonitor::needResultReport() const
{
	return resultTimer.isLargerThan(resultInterval);
}

bool IFStatusMonitor::needStatusReport() const
{
	return statusTimer.isLargerThan(statusInterval);
}

void IFStatusMonitor::setupForNextResult()
{
	header.set_sequence_number(header.sequence_number() + 1);
	header.clear_sweep_count();
	header.clear_time_span();
	restartResultTimer();
}

void IFStatusMonitor::restartResultTimer()
{
	resultTimer.restart();
}

void IFStatusMonitor::restartStatusTimer()
{
	statusTimer.restart();
}