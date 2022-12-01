#include "pch.h"
#include "ZhRecognition.h"
#include "ZhModRecognition.h"
#include "ZhTransRecognition.h"

ZhRecognition::ZhRecognition()
	:m_recogType(UNDO),
	m_modRec(std::make_unique<ZhModRecognition>()),
	m_transRec(std::make_unique<ZhTransRecognition>()),
	m_demod(std::make_unique<ZhAnalogDemod>())
{

}

ZhRecognition::~ZhRecognition()
{
	m_modRec.reset();
	m_transRec.reset();
	m_demod.reset();
}

void ZhRecognition::resetRecogniseType(ZhRecognitionType type)
{ 
	m_recogType = type; 
	m_modRec->resetTimes();
	m_transRec->resetTimes();
	m_demod->resetTimes();
}

void ZhRecognition::resetDemodType(ZhDemodType type)
{
	m_demod->resetDemodType(type);
	m_demod->resetTimes();
}

bool ZhRecognition::Recognise(const senIQSegmentData& header, const std::vector<float> iqData)
{
	if (m_recogType == MOD)
		m_modRec->Recognise(header, iqData);
	if (m_recogType == TRANS)
		m_transRec->Recognise(header, iqData);
	if (m_recogType == DEMOD)
		m_demod->Demod(header, iqData);
	return true;
}

void ZhRecognition::fillResult(zczh::zhIFAnalysis::Result& result)
{
	if (m_recogType == MOD)
		m_modRec->fillResult(result);
	if (m_recogType == TRANS)
		m_transRec->fillResult(result);
	if (m_recogType == DEMOD)
		m_demod->fillResult(result);
}