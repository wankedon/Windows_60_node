#pragma once

#include "NodeTaskFactory.h"
#include "ZhPScanPersistence.h"
//#include "ZhDFPersistence.h"
#include "ZhIFPersistence.h"
#include "IQAcquirePersistence.h"

class ZhTaskFactory : public NodeTaskFactory
{
public:
    ZhTaskFactory(const std::string& dbPath);
    ~ZhTaskFactory() = default;
public:
    std::shared_ptr<NodeTask> createTask(NodeTask::TaskInitEntry&, const DeviceRequestFunc&) override;
    std::mutex dbMutex;
    std::shared_ptr<persistence::PScanDBAccessor> pscanAccessor;
    //std::shared_ptr<persistence::DFDBAccessor> dfAccessor;
    std::shared_ptr<persistence::IFDBAccessor> ifAccessor;
    std::shared_ptr<persistence::IQAcquireDBAccessor> iqAccessor;
};