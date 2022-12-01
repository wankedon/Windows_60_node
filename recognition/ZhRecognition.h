#pragma once
#include "ZhSensorDef.h"
#include "ZhAnalogDemod.h"
#include "node/modulation.pb.h"
#include "node/zczh/ZhIFAnalysis.pb.h"

enum ZhRecognitionType
{
	UNDO = 0,    
	MOD = 1,    //���Ʒ�ʽ
	TRANS = 2,  //��ʽʶ��
	HOPP = 3,   //��Ƶʶ��
	DEMOD = 4,  //���
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