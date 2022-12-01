#include "pch.h"
#include "ZhSensor.h"
#if _MSC_VER > 1900
#include "fmt_/core.h"
#else
#include "fmt/core.h"
#endif // _MSC_VER > 1900
#include "ZhTools.h"
using namespace std;

namespace ZBDevice
{

#ifdef MODEL_3900F
	const std::vector<unsigned long> Sensor60::SUPPORTED_SAMPLE_RATES = { 56000000, 28000000, 14000000, 7000000, 3500000, 1750000, 875000, 437500, 218750, 109375 };
#else
	const std::vector<unsigned long> ZhSensor::SUPPORTED_SAMPLE_RATES = { 28000000, 14000000, 7000000, 3500000, 1750000, 875000, 437500, 218750, 109375 };
#endif // MODEL_3900F

	const std::vector<double> SpectrumSweeper::SUPPORTED_IF_BAND_WIDTH = {
	1e3,2e3,5e3,10e3,20e3,50e3,100e3,200e3,500e3,
	1e6,2e6,5e6,10e6,20e6,40e6,80e6};

	ZhSensor::ZhSensor(const senSensorInfo& info)
		:Device(DeviceType::SENSOR_3900, D_IDLE),
		m_info(info),
		m_status{},
		m_devCtrl(std::make_shared<ZhDeviceControl>(string(info.ipAddress) + ":9901"))
	{
		
	}

	ZhSensor::~ZhSensor()
	{

	}

	void ZhSensor::add()
	{
		//m_name = fmt::format("{}_{}", m_info.modelNumber, m_info.ipAddress);
	}

	bool ZhSensor::connect()
	{
		if (m_devCtrl == nullptr)
			return false;
		return m_devCtrl->isConnect();
	}

	//全景
	senErrorType ZhSensor::startPanoSweep(const senPanoParms& panoParms, const SendSpectrumHandler& panodataHandler)
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->startPano(panoParms, panodataHandler);
	}

	senErrorType ZhSensor::stopPanoSweep()
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->stopPano();
	}

	//测向
	senErrorType ZhSensor::startDF(const senDFParms& dfParms, const SendDFResultHandler& dfResultHandler)
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->startDF(dfParms, dfResultHandler);
	}

	senErrorType ZhSensor::stopDF()
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->stopDF();
	}

	//IQ获取
	senErrorType ZhSensor::startIQ(const senIQParms& iqParms, const SendIQDataHandler& iqDataHandler)
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->startIQ(iqParms, iqDataHandler);
	}

	senErrorType ZhSensor::stopIQ()
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->stopIQ();
	}

	//定频分析
	senErrorType ZhSensor::startIF(const senIFParms& ifParms, const SendSpectrumHandler& specHandler, const SendIQDataHandler& iqHandler)
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->startIF(ifParms, specHandler, iqHandler);
	}

	senErrorType ZhSensor::stopIF()
	{
		if (m_devCtrl == nullptr)
			return SEN_ERR_UNKNOWN;
		return m_devCtrl->stopIF();
	}

	void ZhSensor::updateStatus(const senSensorStatus& status)
	{
		m_status = status;
		*deviceInfo.mutable_position() = m_devCtrl->position();
		auto physicalList = m_devCtrl->physical();
		deviceInfo.clear_physicals();
		for (auto physical: physicalList)
		{
			*deviceInfo.add_physicals() = physical;
		}
		deviceInfo.set_status(m_devCtrl->deviceStatus());
	}

	void ZhSensor::onTimer()
	{

	}

	std::vector<unsigned long> ZhSensor::supportedSampleRates() const
	{
		return SUPPORTED_SAMPLE_RATES;
	}

	unsigned long ZhSensor::adjustSampleRate(double sampleRate)
	{
		unsigned long srcSR = static_cast<unsigned long>(sampleRate);
		if (srcSR <= SUPPORTED_SAMPLE_RATES.back())
			return SUPPORTED_SAMPLE_RATES.back();
		if (srcSR >= SUPPORTED_SAMPLE_RATES.front())
			return SUPPORTED_SAMPLE_RATES.front();
		// in the sensor supported range
		unsigned long dstSR = SUPPORTED_SAMPLE_RATES.front();
		for (auto iter = SUPPORTED_SAMPLE_RATES.crbegin(); iter != SUPPORTED_SAMPLE_RATES.crend(); ++iter)
		{
			if (*iter >= srcSR)
			{
				dstSR = *iter;
				break;
			}
		}
		return dstSR;
	}

	//SpectrumSweeper
	SpectrumSweeper::SpectrumSweeper(std::shared_ptr<ZhSensor> sensor)
		:sensor(sensor),
		sweepParms{}
	{

	}

	SpectrumSweeper::~SpectrumSweeper()
	{
		abort();
	}

	void SpectrumSweeper::configPanoBandParam(double start, double stop, int ifBWIndex)
	{
		m_panoParms.start_freq = start;
		m_panoParms.stop_freq = stop;
		m_panoParms.IF_bandwidth = SUPPORTED_IF_BAND_WIDTH[ifBWIndex];
		sweepParms.numSegments = calculateSegmentNum(m_panoParms);
	}

	void SpectrumSweeper::configPanoWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode,int32_t interval, uint32_t ave)
	{
		m_panoParms.antenna_mode = ant;
		m_panoParms.gain = attGain;
		m_panoParms.interval = interval;
		m_panoParms.receive_mode = receiverMode;
		m_panoParms.average_count = ave;
	}

	void SpectrumSweeper::configCalibParam(senCalibParam* calibParam)
	{
		m_panoParms.calib_param = calibParam;
	}

	bool SpectrumSweeper::start(SweepResultHandler handler)
	{
		resultHandler = handler;
		//计算段freqSegments
		calculateFreqSegments(m_freqSegments, m_panoParms);
		//获取全景段频谱
		auto acqHandler = [this](const senSegmentData& header, const std::vector<float>& spectrumBuffer)
		{
			onAcqSpectrum(header, spectrumBuffer);
		};
		//开始扫描
		if (sensor->startPanoSweep(m_panoParms, acqHandler) != SEN_ERR_NONE)
			return false;
		if (threadEntranceHandler)
		{
			threadEntranceHandler();
		}
		return true;
	}

	void SpectrumSweeper::onAcqSpectrum(const senSegmentData& header, const std::vector<float>& spectrumBuffer)
	{
		const size_t totalBlock = sweepParms.numSweeps * m_freqSegments.size();
		if (header.segmentIndex == 0)
		{
			if (firstSegHandler)
				firstSegHandler(header);
		}
		resultHandler(header, spectrumBuffer);
		if (header.segmentIndex + 1 == m_freqSegments.size())
		{
			if (lastSegHandler)
				lastSegHandler(header);
			if (header.sequenceNumber + 1 == totalBlock)
			{
				if (threadQuitHandler) threadQuitHandler();
			}
		}
	}

	void SpectrumSweeper::setFirstSegHandler(SweepEventHandler handler)
	{
		firstSegHandler = handler;
	}

	void SpectrumSweeper::setLastSegHandler(SweepEventHandler handler)
	{
		lastSegHandler = handler;
	}

	void SpectrumSweeper::setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit)
	{
		if (entrance)
		{
			threadEntranceHandler = entrance;
		}
		if (quit)
		{
			threadQuitHandler = quit;
		}
	}

	void SpectrumSweeper::abort()
	{
		if (threadQuitHandler)
			threadQuitHandler();//调试回放时临时添加
		sensor->stopPanoSweep();
	}

	int SpectrumSweeper::sweepPoints() const
	{
		int points = 0;
		for (auto& seg : m_freqSegments)
		{
			points += seg.numPoints;
		}
		return points;
	}

	pair<double, double> SpectrumSweeper::frequencyRange() const
	{
		if (m_freqSegments.empty())
			return { 0,0 };
		double start = getSegFreqStart(m_freqSegments.front());
		double stop = getSegFreqStop(m_freqSegments.back());
		return { start, stop };
	}

	spectrum::FrequencyBand SpectrumSweeper::frequencyBand() const
	{
		spectrum::FrequencyBand band;
		auto ms = band.mutable_segments();
		for (auto& seg : m_freqSegments)
		{
			auto newSeg = ms->Add();
			newSeg->set_points(seg.numPoints);
			newSeg->mutable_freq_span()->set_start_freq(getSegFreqStart(seg));
			newSeg->mutable_freq_span()->set_stop_freq(getSegFreqStop(seg));
		}
		return band;
	}

	//DirectionFinder
	const std::vector<double> DirectionFinder::SUPPORTED_IF_BAND_WIDTH = {
1e3,2e3,5e3,10e3,20e3,50e3,100e3,200e3,500e3,
1e6,2e6,5e6,10e6,20e6,40e6,80e6 };
	const std::vector<double> DirectionFinder::SUPPORTED_DF_BAND_WIDTH = {
100,200,500,1e3,2e3,5e3,10e3,20e3,50e3,100e3,200e3,500e3,
1e6,2e6,5e6,10e6};

	DirectionFinder::DirectionFinder(std::shared_ptr<ZhSensor> sensor)
		:sensor(sensor),
		m_dfParms{}
	{

	}

	DirectionFinder::~DirectionFinder()
	{
		abort();
	}

	void DirectionFinder::configDFBandParam(double centerFreq, int ifBWIndex, int dfBWIndex)
	{
		m_dfParms.center_freq = centerFreq;
		m_dfParms.IF_bandwidth = SUPPORTED_IF_BAND_WIDTH[ifBWIndex];
		m_dfParms.DF_bandwidth = SUPPORTED_DF_BAND_WIDTH[dfBWIndex];
		segment.numPoints = 800;
		segment.startFrequency = centerFreq - 0.5 * m_dfParms.IF_bandwidth;
		segment.stopFrequency = centerFreq + 0.5 * m_dfParms.IF_bandwidth;
	}

	void DirectionFinder::configDFWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval)
	{
		m_dfParms.antenna_mode = ant;
		m_dfParms.gain = attGain;
		m_dfParms.interval = interval;
		m_dfParms.receive_mode = receiverMode;
	}

	void DirectionFinder::configDFThreshold(int32_t thMode, int32_t thVal, int32_t targetCount)
	{
		m_dfParms.mode = thMode;
		m_dfParms.value = thVal;
		m_dfParms.target_count = targetCount;
	}

	spectrum::FrequencyBand DirectionFinder::frequencyBand() const
	{
		spectrum::FrequencyBand band;
		auto ms = band.mutable_segments();
		auto newSeg = ms->Add();
		newSeg->set_points(segment.numPoints);
		newSeg->mutable_freq_span()->set_start_freq(getSegFreqStart(segment));
		newSeg->mutable_freq_span()->set_stop_freq(getSegFreqStop(segment));
		return band;
	}

	bool DirectionFinder::start(DFResultHandler handler)
	{
		resultHandler = handler;
		//获取测向结果
		auto acqHandler = [this](const senSegmentData& header, const std::vector<senDFTarget>& dfTarget, const std::vector<senVecFloat>& dfSpectrum)
		{
			onAcqDFResult(header, dfTarget, dfSpectrum);
		};
		//开始测向
		if (sensor->startDF(m_dfParms, acqHandler) != SEN_ERR_NONE)
			return false;
		if (threadEntranceHandler)
		{
			threadEntranceHandler();
		}
		return true;
	}

	void DirectionFinder::onAcqDFResult(const senSegmentData& header, const std::vector<senDFTarget>& dfResult, const std::vector<senVecFloat>& dfSpectrum)
	{
		resultHandler(header, dfResult, dfSpectrum);
	}

	void DirectionFinder::setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit)
	{
		if (entrance)
		{
			threadEntranceHandler = entrance;
		}
		if (quit)
		{
			threadQuitHandler = quit;
		}
	}

	void DirectionFinder::abort()
	{
		if (threadQuitHandler) threadQuitHandler();
		sensor->stopDF();
	}

	//IQSweeper
	const std::vector<double> IQSweeper::SUPPORTED_IF_BAND_WIDTH = {
1e3,2e3,5e3,10e3,20e3,50e3,100e3,200e3,500e3,
1e6,2e6,5e6,10e6,20e6,40e6,80e6 };

	IQSweeper::IQSweeper(std::shared_ptr<ZhSensor> sensor)
		:sensor(sensor),
		m_iqParms{}
	{

	}

	IQSweeper::~IQSweeper()
	{
		abort();
	}
	
	void IQSweeper::configBandParam(double start, double stop, int ifBWIndex)
	{
		m_iqParms.start_freq = start;
		m_iqParms.stop_freq = stop;
		m_iqParms.IF_bandwidth = SUPPORTED_IF_BAND_WIDTH[ifBWIndex];;
	}

	void IQSweeper::configWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval)
	{
		m_iqParms.antenna_mode = ant;
		m_iqParms.gain = attGain;
		m_iqParms.interval = interval;
		m_iqParms.receive_mode = receiverMode;
	}

	void IQSweeper::configOther(int32_t points)
	{
		m_iqParms.points = points;
	}

	bool IQSweeper::start(SweepResultHandler handler)
	{
		resultHandler = handler;
		//获取IQ数据
		auto acqHandler = [this](const senIQSegmentData& header, const std::vector<float>& iqdata)
		{
			onAcqIQData(header, iqdata);
		};
		//开始IQ
		if (sensor->startIQ(m_iqParms, acqHandler) != SEN_ERR_NONE)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	void IQSweeper::onAcqIQData(const senIQSegmentData& header, const std::vector<float>& iqdata)
	{
		resultHandler(header, iqdata);
	}

	void IQSweeper::abort()
	{
		sensor->stopIQ();
	}

	//IfAnalyst
	const std::vector<double> IfAnalyst::SUPPORTED_IF_BAND_WIDTH = {
1e3,2e3,5e3,10e3,20e3,50e3,100e3,200e3,500e3,
1e6,2e6,5e6,10e6,20e6,40e6,80e6 };

	IfAnalyst::IfAnalyst(std::shared_ptr<ZhSensor> sensor)
		:sensor(sensor),
		m_ifParms{}
	{

	}

	IfAnalyst::~IfAnalyst()
	{
		abort();
	}

	void IfAnalyst::configIFBandParam(double start, double stop, int ifBWIndex)
	{
		m_ifParms.start_freq = start;
		m_ifParms.stop_freq = stop;
		m_ifParms.IF_bandwidth = SUPPORTED_IF_BAND_WIDTH[ifBWIndex];
		segment.numPoints = 800;
		segment.startFrequency = start;
		segment.stopFrequency = stop;
	}

	void IfAnalyst::configIFWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval)
	{
		m_ifParms.antenna_mode = ant;
		m_ifParms.gain = attGain;
		m_ifParms.interval = interval;
		m_ifParms.receive_mode = receiverMode;
	}

	void IfAnalyst::configIFThreshold(int32_t thMode, int32_t thVal, uint32_t ave)
	{
		m_ifParms.mode = thMode;
		m_ifParms.value = thVal;
		m_ifParms.average_count = ave;
	}

	spectrum::FrequencyBand IfAnalyst::frequencyBand() const
	{
		spectrum::FrequencyBand band;
		auto ms = band.mutable_segments();
		auto newSeg = ms->Add();
		newSeg->set_points(segment.numPoints);
		newSeg->mutable_freq_span()->set_start_freq(getSegFreqStart(segment));
		newSeg->mutable_freq_span()->set_stop_freq(getSegFreqStop(segment));
		return band;
	}

	bool IfAnalyst::start(IFSpectrumHandler spectrumHandler, IFIQHandler iqDataHandler)
	{
		m_specHandler = spectrumHandler;
		m_iqHandler = iqDataHandler;
		//获取定频分析频谱
		auto specHandler = [this](const senSegmentData& header, const senVecFloat& ifSpectrum)
		{
			onAcqIFSpectrum(header, ifSpectrum);
		};
		//获取定频分析IQ
		auto iqHandler = [this](const senIQSegmentData& header, const std::vector<float>& iqdata)
		{
			onAcqIFIQ(header, iqdata);
		};
		//开始定频分析
		if (sensor->startIF(m_ifParms, specHandler, iqHandler) != SEN_ERR_NONE)
			return false;
		if (threadEntranceHandler)
		{
			threadEntranceHandler();
		}
		return true;
	}

	void IfAnalyst::onAcqIFSpectrum(const senSegmentData& header, const senVecFloat& ifSpectrum)
	{
		m_specHandler(header, ifSpectrum);
	}

	void IfAnalyst::onAcqIFIQ(const senIQSegmentData& header, const std::vector<float>& iqdata)
	{
		m_iqHandler(header, iqdata);
	}

	void IfAnalyst::setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit)
	{
		if (entrance)
		{
			threadEntranceHandler = entrance;
		}
		if (quit)
		{
			threadQuitHandler = quit;
		}
	}

	void IfAnalyst::abort()
	{
		if (threadQuitHandler) threadQuitHandler();
		sensor->stopIF();
	}
}
