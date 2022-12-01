#pragma once
#include "ZhSensorDef.h"
#include "node/module/spatialSpectrumInternal.pb.h"

int calculateSegmentNum(const senPanoParms& panoParms);
int calculateSegmentIndex(const senPanoParms& panoParms, double stopFreq);
void calculateFreqSegments(std::vector<senFrequencySegment>& segments,const senPanoParms& panoParms);
struct timeval getTimeVal();
senSegmentData extractHeader(unsigned long numPoints, double segStopFreq, const senPanoParms& panoParms, Position pos, const struct timeval time);
senSegmentData extractHeader(unsigned long numPoints, Position pos, const struct timeval time);
senIQSegmentData extractIQHeader(unsigned long numPoints, double centerFreq, double sampleRate, unsigned long long sequenceNumber = 0, Position pos = Position{}, const struct timeval time= timeval());

void fillPanoRequest(zb::dcts::node::module::spatailSpectrum::internal::PanoramicScanRequest& request, const senPanoParms& panoParms);
void fillDFRequest(zb::dcts::node::module::spatailSpectrum::internal::DirectionRequest& request, const senDFParms& dfParms);
std::vector<senDFTarget> extractDFTarget(const zb::dcts::node::module::spatailSpectrum::internal::DirectionResult& result, const double direction);
void fillIQRequest(zb::dcts::node::module::spatailSpectrum::internal::IQRequest& request, const senIQParms& iqParms);
void fillIFRequest(zb::dcts::node::module::spatailSpectrum::internal::SignalAnalysisRequest& request, const senIFParms& ifParms);

Physical generalPhysical(std::string module, double value, Physical::Type type);