#include "pch.h"
#include "ZhTools.h"
#include <time.h>
#include "cmath"

int calculateSegmentNum(const senPanoParms& panoParms)
{
	return ceil((panoParms.stop_freq - panoParms.start_freq) / panoParms.IF_bandwidth);
}

int calculateSegmentIndex(const senPanoParms& panoParms,double stopFreq)
{
	return (stopFreq - panoParms.start_freq) / panoParms.IF_bandwidth-1;
}

void calculateFreqSegments(std::vector<senFrequencySegment>& segments, const senPanoParms& panoParms)
{
	auto numSegments = calculateSegmentNum(panoParms);
	senFrequencySegment freSegment;
	for (size_t i = 0; i < numSegments; i++)
	{
		freSegment.numPoints = 800;
		freSegment.startFrequency = panoParms.start_freq + i * panoParms.IF_bandwidth;
		freSegment.stopFrequency = freSegment.startFrequency + panoParms.IF_bandwidth;
		segments.emplace_back(freSegment);
	}
}

struct timeval getTimeVal()
{
	struct timeval tv;
#ifdef PLATFROM_WINDOWS
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	struct tm tm;
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_wday = wtm.wDayOfWeek;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	tv.tv_sec = mktime(&tm);
	tv.tv_usec = wtm.wMilliseconds * 1000;
#else
	struct timezone tz;
	gettimeofday(&tv, &tz);
#endif 
	return tv;
}

senSegmentData extractHeader(unsigned long numPoints, Position pos,const struct timeval tval)
{
	senSegmentData header;
	header.numPoints = numPoints;
	//auto tval = getTimeVal();
	header.location.longitude = pos.longitude();
	header.location.latitude = pos.latitude();
	header.location.elevation = pos.altitude();
	header.timestampSec = tval.tv_sec;
	header.timestampNSec = tval.tv_usec;
	return header;
}

senSegmentData extractHeader(unsigned long numPoints, double segStopFreq, const senPanoParms& panoParms, Position pos,const struct timeval tval)
{
	senSegmentData header = extractHeader(numPoints, pos,tval);
	header.segmentIndex = calculateSegmentIndex(panoParms, segStopFreq);
	return header;
}

senIQSegmentData extractIQHeader(unsigned long numPoints, double centerFreq, double sampleRate,unsigned long long sequenceNumber, Position pos, const struct timeval tval)
{
	senIQSegmentData header;
	header.sequenceNumber = sequenceNumber;
	header.numSamples = numPoints;
	header.centerFrequency = centerFreq;
	header.sampleRate = sampleRate;
	//auto tval = getTimeVal();
	header.timestampSeconds = tval.tv_sec;
	header.timestampNSeconds = tval.tv_usec;
	header.longitude = pos.longitude();
	header.latitude = pos.latitude();
	header.elevation = pos.altitude();
	return header;
}

void fillPanoRequest(zb::dcts::node::module::spatailSpectrum::internal::PanoramicScanRequest& request, const senPanoParms& panoParms)
{
	auto bandparam = request.mutable_band_param();
	bandparam->mutable_span()->set_start_freq(panoParms.start_freq);
	bandparam->mutable_span()->set_stop_freq(panoParms.stop_freq);
	bandparam->set_if_bandwidth(panoParms.IF_bandwidth);
	auto workmode = request.mutable_work_mode();
	workmode->set_antenna_mode(panoParms.antenna_mode);
	workmode->set_receive_mode(panoParms.receive_mode);
	workmode->set_gain(panoParms.gain);
	workmode->set_interval(0);//panoParms.interval
	if (panoParms.calib_param == nullptr)
	{
		request.set_scan_type(3);
		return;
	}
	auto calibparam = request.mutable_calibration_param();
	auto sencalibparam = *panoParms.calib_param;
	calibparam->set_antenna_mode(sencalibparam.antenna_mode);
	calibparam->set_gain(sencalibparam.gain);
	calibparam->set_downconverter_mode(sencalibparam.downconverter_mode);
	calibparam->set_downconverter_if_attenuation(sencalibparam.downconverter_if_attenuation);
	calibparam->set_receive_mode(sencalibparam.receive_mode);
	calibparam->set_receive_rf_attenuation(sencalibparam.receive_rf_attenuation);
	calibparam->set_receive_if_attenuation(sencalibparam.receive_if_attenuation);
	calibparam->set_attenuation_mode(sencalibparam.attenuation_mode);
	request.set_scan_type(6);
}

void fillDFRequest(zb::dcts::node::module::spatailSpectrum::internal::DirectionRequest& request, const senDFParms& dfParms)
{
	request.set_center_freq(dfParms.center_freq);
	request.set_if_bandwidth(dfParms.IF_bandwidth);
	request.set_df_bandwidth(dfParms.DF_bandwidth);
	auto workmode = request.mutable_work_mode();
	workmode->set_antenna_mode(dfParms.antenna_mode);
	workmode->set_receive_mode(dfParms.receive_mode);
	workmode->set_gain(dfParms.gain);
	workmode->set_interval(0);
	auto thredparam = request.mutable_threshold_mode();
	thredparam->set_mode(dfParms.mode);
	thredparam->set_value(dfParms.value);
}

std::vector<senDFTarget> extractDFTarget(const zb::dcts::node::module::spatailSpectrum::internal::DirectionResult& result,const double direction)
{
	std::vector<senDFTarget> dfTarget;
	for (auto tar : result.target_detection())
	{
		senDFTarget tatget;
		tatget.center_frequency = tar.center_frequency();
		tatget.direction = direction;
		tatget.confidence = tar.confidence();
		tatget.amplitude = tar.amplitude();
		dfTarget.emplace_back(tatget);
	}
	return dfTarget;
}

void fillIQRequest(zb::dcts::node::module::spatailSpectrum::internal::IQRequest& request, const senIQParms& iqParms)
{
	auto bandparam = request.mutable_band_param();
	bandparam->mutable_span()->set_start_freq(iqParms.start_freq);
	bandparam->mutable_span()->set_stop_freq(iqParms.stop_freq);
	bandparam->set_if_bandwidth(iqParms.IF_bandwidth);
	auto workmode = request.mutable_work_mode();
	workmode->set_antenna_mode(iqParms.antenna_mode);
	workmode->set_receive_mode(iqParms.receive_mode);
	workmode->set_gain(iqParms.gain);
	workmode->set_interval(0);
	request.set_points(iqParms.points);
}

void fillIFRequest(zb::dcts::node::module::spatailSpectrum::internal::SignalAnalysisRequest& request, const senIFParms& ifParms)
{
	auto bandparam = request.mutable_band_param();
	bandparam->mutable_span()->set_start_freq(ifParms.start_freq);
	bandparam->mutable_span()->set_stop_freq(ifParms.stop_freq);
	bandparam->set_if_bandwidth(ifParms.IF_bandwidth);
	auto workmode = request.mutable_work_mode();
	workmode->set_antenna_mode(ifParms.antenna_mode);
	workmode->set_receive_mode(ifParms.receive_mode);
	workmode->set_gain(ifParms.gain);
	workmode->set_interval(0);
	auto thredparam = request.mutable_threshold_mode();
	thredparam->set_mode(ifParms.mode);
	thredparam->set_value(ifParms.value);
}

Physical generalPhysical(std::string module, double value, Physical::Type type)
{
	Physical physical;
	physical.set_type(type);
	physical.set_value(value);
	physical.set_unit(module);
	return physical;
}