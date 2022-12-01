#pragma once
#include "ZhIFPersistence.h"
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "AccumulatedSink.h"
#include <future>

class IFDataAsyncWriter
{
public:
    IFDataAsyncWriter(std::shared_ptr<persistence::IFDBAccessor> accessor);
    ~IFDataAsyncWriter();
public:
    bool isReady();
    bool writeItem(const persistence::IFDataItem& item);
    int recordCount() const; 
private:
    std::shared_ptr<persistence::IFDBAccessor> accessor;
    std::atomic<int> writeItemCounter;
    std::future<void> asyncWriteProc;
};

class IFRecorder
{
    friend class IFSpectrumProcessor;    //暴露私有成员给processor，以便能够让其链接成数据流
public:
    IFRecorder(std::shared_ptr<persistence::IFDBAccessor> accessor);
    ~IFRecorder();
public:
    void writeTaskInfo(const persistence::IFTaskItem& taskItem);
    void onResultReport(const zczh::zhIFAnalysis::Result& result);
    void enable();
    void disable();
    void rewriteTaskInfo(const zczh::zhIFAnalysis::OperationStatus& lastStatus);
    int recordCount() const { return dataWriter.recordCount(); }
private:
    bool isEnabled() const;
    void makeDataItem(const zczh::zhIFAnalysis::Result& result);
    void resetAcc();
private:
    std::shared_ptr<persistence::IFDBAccessor> accessor;
    std::unique_ptr<persistence::IFTaskItem> taskInfo;
    AccumulatedSink accumulatedSpectrum;
    AccumulatedSink accumulatedHits;
    persistence::IFDataItem dataItem;
    IFDataAsyncWriter dataWriter;
};

