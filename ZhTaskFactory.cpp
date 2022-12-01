#include "pch.h"
#include "ZhTaskFactory.h"
#include "ZhPScanTask.h"
#include "ZhPScanReplayTask.h"
#include "ZhDFTask.h"
#include "ZhIQAcquireTask.h"
#include "IQReplayTask.h"
#include "ZhIFTask.h"
using namespace std;

ZhTaskFactory::ZhTaskFactory(const std::string& dbPath)
	:pscanAccessor(make_shared<persistence::PScanDBAccessor>(dbPath, dbMutex)),
	//dfAccessor(make_shared<persistence::DFDBAccessor>(dbPath, dbMutex)),
	ifAccessor(make_shared<persistence::IFDBAccessor>(dbPath, dbMutex)),
	iqAccessor(make_shared<persistence::IQAcquireDBAccessor>(dbPath, dbMutex))
{
}

std::shared_ptr<NodeTask> ZhTaskFactory::createTask(NodeTask::TaskInitEntry& entry, const DeviceRequestFunc& deviceRequest)
{
	auto primaryType = entry.type.pri_task_type();
	auto secondaryType = entry.type.sec_task_type();
	if (primaryType != ZCZH_TASK)
		return nullptr;
	std::shared_ptr<NodeTask> task = nullptr;
	if (secondaryType >= PSCAN && secondaryType < REPLAY_PSCAN)
	{
		entry.device = deviceRequest({ entry.header.task_runner().device_id(), SENSOR_3900 });
		if (!entry.device)
			return nullptr;
		if (entry.header.task_runner().device_id().value() == 0)	//若自动指派需在此指定具体设备id
		{
			*(entry.header.mutable_task_runner()->mutable_device_id()) = entry.device->id();
		}
	}
	switch (secondaryType)
	{
	case PSCAN:
		task = make_shared<ZhPScanTask>(entry, pscanAccessor);
		break;
	case REPLAY_PSCAN:
		task = make_shared<ZhPScanReplayTask>(entry, pscanAccessor);
		break;
	case DIRECTION_FINDING_SPATIAL_SPECTRUM:
		task = make_shared<ZhDFTask>(entry);
		break;
	case IF_SCAN:
		task = make_shared<ZhIFTask>(entry, ifAccessor);
		break;
	case IQ_ACQUIRE:
		task = make_shared<ZhIQAcquireTask>(entry, iqAccessor);
		break;
	case REPLAY_IQ:
		task = make_shared<IQReplayTask>(entry, iqAccessor);
		break;
	default:
		break;
	}
	return task;
}