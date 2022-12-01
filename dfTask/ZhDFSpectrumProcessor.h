#pragma once
#include "node/zczh/ZhDirection.pb.h"
#include "node/spectrum.pb.h"
#include "ZhSensor.h"
#include "ZMsgWrap.h"
//#include "ZhDFRecorder.h"

class SpectrumDataHolder;
class FrequencyScale;
class ThresholdManager;
class SignalDetection;

class DFSpectrumProcessor
{
public:
	DFSpectrumProcessor(const spectrum::FrequencyBand& band, int32_t outputPoints/*, std::shared_ptr<persistence::DFDBAccessor> dbAccessor*/);
	~DFSpectrumProcessor(void);
public:
	ErrorType onCmd(MessageExtractor& extractor);
	void onTaskBegin(const NodeDevice& nd);
	void onTaskEnd(const zczh::zhdirection::OperationStatus& status);
	void onSpectrum(const senSegmentData& header, const std::vector<float>& segmentData);
	void fillResult(zczh::zhdirection::Result& result);
	void fillStatus(zczh::zhdirection::OperationStatus& status);
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
	void fillDataHoldResult(zczh::zhdirection::Result& result);
	void fillDetectResult(zczh::zhdirection::Result& result);
	void fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama);
	//persistence::DFTaskItem makeTaskItem(const NodeDevice& nd);
	void signalDetect(const zczh::zhdirection::Result& result);
private:
	const int32_t expectedPoints;
	std::unique_ptr<spectrum::FrequencySegment> zoominParam;
	std::unique_ptr<FrequencyScale> scale;								//近景频率范围控制
	std::map<spectrum::DataHoldType, std::shared_ptr<SpectrumDataHolder>> dataHolder;	//数据持有
	std::unique_ptr<ThresholdManager> thresholdManager;						//门限
	std::unique_ptr<SignalDetection> signalDetector;
	std::list<MessageExtractor> pendingCmd;		//待执行的命令
	//std::shared_ptr<persistence::DFDBAccessor> dbAccessor;
	//std::unique_ptr<DFRecorder> recorder;
	std::mutex mtx;
};