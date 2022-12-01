#pragma once
#include "ZhSensorDef.h"
#include "ZhAnalogDemod.h"
#include "node/modulation.pb.h"
#include "node/zczh/ZhIFAnalysis.pb.h"

enum ZhRecognitionType
{
	UNDO = 0,    
	MOD = 1,    //调制方式
	TRANS = 2,  //制式识别
	HOPP = 3,   //跳频识别
	DEMOD = 4,  //解调
};
using namespace zb::dcts::node::modulation;

class ZhModRecognition;
class ZhTransRecognition;
class ZhAnalogDemod;
class ZhRecognition
{
public:
	ZhRecognition();
	~ZhRecognition();
public:
	void resetRecogniseType(ZhRecognitionType type);
	void resetDemodType(ZhDemodType type);
	bool Recognise(const senIQSegmentData& header, const std::vector<float> iqData);
	void fillResult(zczh::zhIFAnalysis::Result& result);
private:
	ZhRecognitionType m_recogType;
	std::unique_ptr<ZhModRecognition> m_modRec;
	std::unique_ptr<ZhTransRecognition> m_transRec;
	std::unique_ptr<ZhAnalogDemod> m_demod;
};