#pragma once
#include <memory>
#include "Device.h"
#include "ZhDeviceControl.h"
#include "ZhSensorDef.h"
#include "ThreadWrap.h"
#include <functional>
#include "node/spectrum.pb.h"

namespace ZBDevice
{
	class ZhDeviceControl;
	class ZhSensor : public Device
	{
	public:
		ZhSensor(const senSensorInfo& info);
		~ZhSensor();
	public:
		void add();
		bool connect();
		void updateStatus(const senSensorStatus& status);
		void onTimer() override;
		std::vector<unsigned long> supportedSampleRates() const;
		unsigned long adjustSampleRate(double sampleRate);
		//全景
		senErrorType startPanoSweep(const senPanoParms& panoParms, const SendSpectrumHandler& panodataHandler);
		senErrorType stopPanoSweep();
		//测向
		senErrorType startDF(const senDFParms& dfParms, const SendDFResultHandler& dfResultHandler);
		senErrorType stopDF();
		//IQ
		senErrorType startIQ(const senIQParms& dfParms, const SendIQDataHandler& iqDataHandler);
		senErrorType stopIQ();
		//定频分析
		senErrorType startIF(const senIFParms& ifParms, const SendSpectrumHandler& specHandler, const SendIQDataHandler& iqHandler);
		senErrorType stopIF();

	private:
		std::string m_name;
		const senSensorInfo m_info;
		senSensorStatus m_status;

	private:
		static const std::vector<unsigned long> SUPPORTED_SAMPLE_RATES;
		std::shared_ptr<ZhDeviceControl> m_devCtrl;
	};

    //全景
	class SpectrumSweeper
	{
		using SweepResultHandler = std::function<void(const senSegmentData&, const std::vector<float>&)>;
		using SweepEventHandler = std::function<void(const senSegmentData&)>;
		using ThreadEventHandler = std::function<void()>;
	public:
		SpectrumSweeper(std::shared_ptr<ZhSensor> sensor);
		~SpectrumSweeper();
	public:
		void configPanoBandParam(double start, double stop, int ifBWIndex);
		void configPanoWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval, uint32_t ave);
		void configCalibParam(senCalibParam* calibParam);
	public:
		bool start(SweepResultHandler handler);
		void setFirstSegHandler(SweepEventHandler handler);
		void setLastSegHandler(SweepEventHandler handler);
		void setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit);
		void abort();
		int sweepPoints() const;
		std::pair<double, double> frequencyRange() const;
		spectrum::FrequencyBand frequencyBand() const;
		size_t segmentCount() const { return m_freqSegments.size(); }
		void onAcqSpectrum(const senSegmentData& header, const std::vector<float>& spectrumBuffer);
	private:
		ThreadWrap t;
		std::shared_ptr<ZhSensor> sensor;
		senSweepParms sweepParms;
		senPanoParms m_panoParms;
		std::vector<senFrequencySegment> m_freqSegments;
		SweepResultHandler resultHandler;
		SweepEventHandler firstSegHandler;
		SweepEventHandler lastSegHandler;
		ThreadEventHandler threadEntranceHandler;
		ThreadEventHandler threadQuitHandler;
		static const std::vector<double> SUPPORTED_IF_BAND_WIDTH;
	};

	//测向
	class DirectionFinder
	{
	public:
		using DFResultHandler = std::function<void(const senSegmentData& header, const std::vector<senDFTarget>& dfResult,const std::vector<senVecFloat>& dfSpectrum)>;
		using ThreadEventHandler = std::function<void()>;
	
	public:
		DirectionFinder(std::shared_ptr<ZhSensor> sensor);
		~DirectionFinder();

	public:
		void configDFBandParam(double centerFreq, int ifBWIndex, int dfBWIndex);
		void configDFWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval);
		void configDFThreshold(int32_t thMode, int32_t thVal, int32_t targetCount);
	
	public:
		spectrum::FrequencyBand frequencyBand() const;
		bool start(DFResultHandler handler);
		void setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit);
		void abort();

	private:
		void onAcqDFResult(const senSegmentData& header, const std::vector<senDFTarget>& dfResult, const std::vector<senVecFloat>& dfSpectrum);

	private:
		std::shared_ptr<ZhSensor> sensor;
		senDFParms m_dfParms;
		senFrequencySegment segment;
		DFResultHandler resultHandler;
		ThreadEventHandler threadEntranceHandler;
		ThreadEventHandler threadQuitHandler;
		static const std::vector<double> SUPPORTED_IF_BAND_WIDTH;
		static const std::vector<double> SUPPORTED_DF_BAND_WIDTH;
	};
	
	//IQ获取
	class IQSweeper
	{
		using SweepResultHandler = std::function<void(const senIQSegmentData&, const std::vector<float>&)>;
	public:
		IQSweeper(std::shared_ptr<ZhSensor> sensor);
		~IQSweeper();
	public:
		void configBandParam(double start, double stop, int ifBWIndex);
		void configWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval);
		void configOther(int32_t points);

	public:
		bool start(SweepResultHandler handler);
		void onAcqIQData(const senIQSegmentData& header, const std::vector<float>& iqdata);
		void abort();

	private:
		std::shared_ptr<ZhSensor> sensor;
		senIQParms m_iqParms;
		SweepResultHandler resultHandler;
		static const std::vector<double> SUPPORTED_IF_BAND_WIDTH;
	};

	//定频分析
	class IfAnalyst
	{
	public:
		using IFSpectrumHandler = std::function<void(const senSegmentData& header, const senVecFloat& ifSpectrum)>;
		using IFIQHandler = std::function<void(const senIQSegmentData& header, const std::vector<float>& iqdata)>;

		using ThreadEventHandler = std::function<void()>;
	public:
		IfAnalyst(std::shared_ptr<ZhSensor> sensor);
		~IfAnalyst();
	public:
		void configIFBandParam(double start, double stop, int ifBWIndex);
		void configIFWorkMode(int attGain, senAntennaType ant, senReceiverType receiverMode, int32_t interval);
		void configIFThreshold(int32_t thMode, int32_t thVal, uint32_t ave);

	public:
		spectrum::FrequencyBand frequencyBand() const;
		bool start(IFSpectrumHandler specHandler, IFIQHandler iqHandler);
		void setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit);
		void abort();

	private:
		void onAcqIFSpectrum(const senSegmentData& header, const senVecFloat& dfSpectrum);
		void onAcqIFIQ(const senIQSegmentData& header, const std::vector<float>& iqdata);

	private:
		std::shared_ptr<ZhSensor> sensor;
		senIFParms m_ifParms;
		senFrequencySegment segment;
		IFSpectrumHandler m_specHandler;
		IFIQHandler m_iqHandler;
		ThreadEventHandler threadEntranceHandler;
		ThreadEventHandler threadQuitHandler;
		static const std::vector<double> SUPPORTED_IF_BAND_WIDTH;
	};

	//...
	inline double getSegFreqStart(const senFrequencySegment& freqSeg)
	{
		return freqSeg.startFrequency;
	}

	inline double getSegFreqStop(const senFrequencySegment& freqSeg)
	{
		return freqSeg.stopFrequency;;
	}

	template <class Header>
	Timestamp extractSensorTS(const Header& header)
	{
		Timestamp ts;
		ts.set_seconds(header.timestampSec);
		ts.set_nanos(header.timestampNSec);
		return ts;
	}

	template <class Location>
	Position extractSensorPos(const Location& loc)
	{
		Position pos;
		pos.set_longitude(loc.longitude);
		pos.set_latitude(loc.latitude);
		pos.set_altitude(loc.elevation);
		return pos;
	}
}