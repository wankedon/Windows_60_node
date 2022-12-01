#pragma once
#include "ZhDFPersistence.h"
#include "node/zczh/ZhDirection.pb.h"
#include "AccumulatedSink.h"
#include <future>

class DFDataAsyncWriter
{
public:
    DFDataAsyncWriter(std::shared_ptr<persistence::DFDBAccessor> accessor);
    ~DFDataAsyncWriter();
public:
    bool isReady();
    bool writeItem(const persistence::DFDataItem& item);
    int recordCount() const; 
private:
    std::shared_ptr<persistence::DFDBAccessor> accessor;
    std::atomic<int> writeItemCounter;
    std::future<void> asyncWriteProc;
};

class DFRecorder
{
    friend class DFSpectrumProcessor;    //暴露私有成员给processor，以便能够让其链接成数据流
public:
    DFRecorder(std::shared_ptr<persistence::DFDBAccessor> accessor);
    ~DFRecorder();
public:
    void writeTaskInfo(const persistence::DFTaskItem& taskItem);
    void onResultReport(const zczh::zhdirection::Result& result);
    void enable();
    void disable();
    void rewriteTaskInfo(const zczh::zhdirection::OperationStatus& lastStatus);
    int recordCount() const { return dataWriter.recordCount(); }
private:
    bool isEnabled() const;
    void makeDataItem(const zczh::zhdirection::Result& result);
    void resetAcc();
private:
    std::shared_ptr<persistence::DFDBAccessor> accessor;
    std::unique_ptr<persistence::DFTaskItem> taskInfo;
    AccumulatedSink accumulatedSpectrum;
    AccumulatedSink accumulatedHits;
    persistence::DFDataItem dataItem;
    DFDataAsyncWriter dataWriter;
};

