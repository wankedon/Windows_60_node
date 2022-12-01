#pragma once
#include "IQAcquirePersistence.h"
#include "node/zczh/ZhIQAcquire.pb.h"
#include <future>

class IQDataAsyncWriter
{
public:
    IQDataAsyncWriter(std::shared_ptr<persistence::IQAcquireDBAccessor> accessor);
    ~IQDataAsyncWriter();
public:
    bool isReady();
    bool writeItem(const std::string& taskid, const zczh::zhIQAcquire::Result& result);
    int recordCount() const;
private:
    persistence::IQAcquireDataItem makeDataItem(const std::string& taskId, const zczh::zhIQAcquire::Result& result);
private:
    std::shared_ptr<persistence::IQAcquireDBAccessor> accessor;
    std::atomic<int> writeItemCounter;
    std::future<void> asyncWriteProc;
};

class IQAcquireRecorder
{
public:
    IQAcquireRecorder(std::shared_ptr<persistence::IQAcquireDBAccessor> accessor);
    ~IQAcquireRecorder();
public:
    void writeTaskInfo(const persistence::IQAcquireTaskItem& taskItem);
    void onResultReport(const zczh::zhIQAcquire::Result& result);
    void enable();
    void disable();
    void rewriteTaskInfo(const Timestamp& stop);
    int recordCount() const { return dataWriter.recordCount(); }
private:
    bool isEnabled() const;
private:
    std::atomic_bool enableFlag;
    std::shared_ptr<persistence::IQAcquireDBAccessor> accessor;
    std::unique_ptr<persistence::IQAcquireTaskItem> taskInfo;
    persistence::IQAcquireDataItem dataItem;
    IQDataAsyncWriter dataWriter;
};

