#pragma once
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "node/spectrum.pb.h"
#include "ZhSensor.h"
#include "ZMsgWrap.h"
#include "ZhIFRecorder.h"

class SpectrumDataHolder;
class FrequencyScale;
class ThresholdManager;
class SignalDetection;

class IFSpectrumProcessor
{
public:
	IFSpectrumProcessor(const spectrum::FrequencyBand& band, int32_t outputPoints, std::shared_ptr<persistence::IFDBAccessor> dbAccessor);
	~IFSpectrumProcessor(void);
public:
	ErrorType onCmd(MessageExtractor& extractor);
	void onTaskBegin(const NodeDevice& nd);
	void onTaskEnd(const zczh::zhIFAnalysis::OperationStatus& status);
	void onSpectrum(const senSegmentData& header, const std::vector<float>& segmentData);
	void fillResult(zczh::zhIFAnalysis::Result& result);
	void fillStatus(zczh::zhIFAnalysis::OperationStatus& status);
private:
	void processPendingCmd(std::list<MessageExtractor>& cmdCopy);
	void onDataHoldOpen(MessageExtractor& extractor);
	void onDataHoldClose(MessageExtractor& extractor);
	void onDataHoldReset(MessageExtractor& extractor);
	void onDetectOpen(MessageExtractor& extractor);
	void onDetectClose(MessageExtractor& extractor);
	void onZoominOpen(MessageExtractor& extractor);
	void onZoominClose(MessageExtractor& extractor);
	void onRecordOpen(MessageExtractor& extractor);
	void onRecordClose(MessageExtractor& extractor);
	void onDetailKeeping(MessageExtractor& extractor);
	void fillDataHoldResult(zczh::zhIFAnalysis::Result& result);
	void fillDetectResult(zczh::zhIFAnalysis::Result& result);
	void fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama);
	persistence::IFTaskItem makeTaskItem(const NodeDevice& nd);
	void signalDetect(const zczh::zhIFAnalysis::Result& result);
private:
	const int32_t expectedPoints;
	std::unique_ptr<spectrum::FrequencySegment> zoominParam;
	std::unique_ptr<FrequencyScale> scale;								//近景频率范围控制
	std::map<spectrum::DataHoldType, std::shared_ptr<SpectrumDataHolder>> dataHolder;	//数据持有
	std::unique_ptr<ThresholdManager> thresholdManager;						//门限
	std::unique_ptr<SignalDetection> signalDetector;
	std::list<MessageExtractor> pendingCmd;		//待执行的命令
	std::shared_ptr<persistence::IFDBAccessor> dbAccessor;
	std::unique_ptr<IFRecorder> recorder;
	std::mutex mtx;
};