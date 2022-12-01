#include "pch.h"
#include "ZhDeviceControl.h"
#include "ZhSpectrumAverage.h"
#include "ZhTools.h"
#include "Logger.h"
#include <time.h>
#ifdef SAVE_BIN_IQDATA
#include <fstream>
#endif
using namespace std;

namespace ZBDevice
{
	ZhDeviceControl::ZhDeviceControl(std::string devCtrlSerAddress)
		:m_devCtrlSerAddress(devCtrlSerAddress),
		m_iqPackNumber(0),
		m_flightControl(std::make_unique<ZhFlightControl>())
	{
		createStub(devCtrlSerAddress);
	}

	ZhDeviceControl::~ZhDeviceControl()
	{
		m_thread[DEVSTATUS].reset();
		m_flightControl.reset();
	}

	bool ZhDeviceControl::isConnect()
	{ 
		if (m_stub == nullptr)
			createStub(m_devCtrlSerAddress);
		m_flightControl->openSerialPort();
		return m_devStatus.connected();
	}

	void ZhDeviceControl::createStub(std::string devCtrlSerAddress)
	{
		grpc::ChannelArguments channelArgs;
		channelArgs.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, 20 * 1024 * 1024);
		auto channel = grpc::CreateCustomChannel(devCtrlSerAddress, grpc::InsecureChannelCredentials(), channelArgs);
		//auto channel = grpc::CreateChannel(devCtrlSerAddress, grpc::InsecureChannelCredentials());
		gpr_timespec timeout{ 1,0,GPR_TIMESPAN };
		if (!channel->WaitForConnected<gpr_timespec>(timeout))
		{
			LOG("Can't CreateChannel");
			return;
		}
		LOG("CreateChannel {}", devCtrlSerAddress);
		m_stub = DeviceControlService::NewStub(channel);
		if (m_stub == nullptr)
			return;
		//启动获取设备状态线程
		m_thread[DEVSTATUS] = make_unique<ThreadLoop>([this]() {getDeviceStatusLoop(); });
	}

	//获取设备状态
	void ZhDeviceControl::getDeviceStatusLoop()
	{
		LOG("Start GetDeviceStatusLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetDeviceStatus(&context, request));
		while (m_thread[DEVSTATUS] && m_thread[DEVSTATUS]->isRuning())
		{
			reader->Read(&m_devStatus);
			if (!m_devStatus.connected())
				LOG("DeviceStatus False");
			else
				LOG("DeviceStatus True");
		}
		Status status = reader->Finish();
		LOG("Stop GetDeviceStatusLoop");
		if (!status.ok())
		{
			cout << "getDeviceStatus error:" << status.error_message() << endl;
		}
	}

	Position ZhDeviceControl::position()
	{
		//根据m_flightControl获取位置
		auto flpos = m_flightControl->getPosition();
		Position pos;
		pos.set_altitude(flpos.altitude);
		pos.set_latitude(flpos.latitude * M_PI / 180);
		pos.set_longitude(flpos.longitude * M_PI / 180);
		return pos;
	}

	struct timeval ZhDeviceControl::timeVal()
	{
		return m_flightControl->getTimeVal();
	}

	SYSTEMTIME ZhDeviceControl::systemTime()
	{
		return m_flightControl->systemTime();
	}

	zb::dcts::node::DeviceStatus ZhDeviceControl::deviceStatus()
	{
		return m_devStatus.connected() ? D_IDLE : D_OFFLINE;
	}

	std::vector<Physical> ZhDeviceControl::physical()
	{
		m_physicalList.clear();
		m_physicalList.emplace_back(generalPhysical("Processor_Temperature", m_devStatus.processor_temperature(), Physical::Type::Physical_Type_TEMPERATURE));
		//physicalList.emplace_back(generalPhysical("processor_status", m_devStatus.processor_status()));
		m_physicalList.emplace_back(generalPhysical("Receiver_Temperature", m_devStatus.receiver_temperature(), Physical::Type::Physical_Type_TEMPERATURE));
		//physicalList.emplace_back(generalPhysical("receiver_status", m_devStatus.receiver_status()));
		m_physicalList.emplace_back(generalPhysical("MasterControlKernel_Temperature", m_devStatus.mainctl_kernel_temp(), Physical::Type::Physical_Type_TEMPERATURE));
		m_physicalList.emplace_back(generalPhysical("MasterControl_Temperature", m_devStatus.mainctl_temperature(), Physical::Type::Physical_Type_TEMPERATURE));
		m_physicalList.emplace_back(generalPhysical("MasterControl_Humidity", m_devStatus.mainctl_humidity(), Physical::Type::Physical_Type_HUMIDITY));
		m_physicalList.emplace_back(generalPhysical("MasterControl_Voltage", m_devStatus.mainctl_vccin(), Physical::Type::Physical_Type_VELOCITY));
		//physicalList.emplace_back(generalPhysical("mainctl_status", m_devStatus.mainctl_status()));
		auto systime = systemTime();
		LOG("timeVal().tv_sec: {}", timeVal().tv_sec);
		//physicalList.emplace_back(generalPhysical("UTCTime", timeVal().tv_sec, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Year", systime.wYear, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Month", systime.wMonth, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Day", systime.wDay, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Hour", systime.wHour, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Minute", systime.wMinute, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Second", systime.wSecond, Physical::Type::Physical_Type_CURRENT));
		m_physicalList.emplace_back(generalPhysical("Milliseconds", systime.wMilliseconds, Physical::Type::Physical_Type_CURRENT));
		LOG("time {} {} {} {} {} {} {}", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
		return m_physicalList;
	}

	//开始全景
	senErrorType ZhDeviceControl::startPano(const senPanoParms& panoParms,const SendSpectrumHandler& panoDataHandler)
	{
		m_panoParms = panoParms;
		m_sendPScanSpectrumHandler = panoDataHandler;
		ClientContext context;
		PanoramicScanRequest request;
		fillPanoRequest(request, panoParms);
		DeviceReply reply;
		auto status = m_stub->StartPanoramic(&context, request, &reply);
		if (status.ok())
		{
			LOG("Start PScan");
			//启动获取全景频谱线程
			m_thread[PSCAN] = make_unique<ThreadLoop>([this]() {getPScanSpectrumLoop(); });
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//获取全景数据
	void ZhDeviceControl::getPScanSpectrumLoop()
	{
		LOG("Start GetPScanSpectrumLoop");
		auto segmentNum = calculateSegmentNum(m_panoParms);
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetPanoramicSpectrum(&context, request));
		PanoramicScanResult result;
		int currentIndex = 0;
		while (m_thread[PSCAN] && m_thread[PSCAN]->isRuning())
		{
			if (reader->Read(&result) && result.spectrum().amplitude_size())
			{
				senVecFloat spectrum(result.spectrum().amplitude().begin(), result.spectrum().amplitude().end());
				auto header = extractHeader(spectrum.size(), result.spectrum().span().stop_freq(), m_panoParms, position(), timeVal());
				if (header.segmentIndex == currentIndex && m_sendPScanSpectrumHandler)
				{
					m_sendPScanSpectrumHandler(header, spectrum);
					currentIndex++;
				}
				else
				{
					LOG("Index {} PScanSpectrum Lost", currentIndex);
				}
				currentIndex = (currentIndex == segmentNum) ? 0 : currentIndex;
			}
		}
		LOG("Stop GetPScanSpectrumLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getPScanSpectrum error:" << status.error_message() << endl;
		//}
	}

	//停止全景
	senErrorType ZhDeviceControl::stopPano()
	{
		ClientContext context;
		::google::protobuf::Empty request;
		DeviceReply reply;
		auto status = m_stub->StopPanoramic(&context, request, &reply);
		if (status.ok())
		{
			m_thread[PSCAN]->stop();
			LOG("Stop PScan");
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//开始测向
	senErrorType ZhDeviceControl::startDF(const senDFParms& dfParms, const SendDFResultHandler& dfResultHandler)
	{
		m_dfSpectrumbuffer = make_unique<ZhSpectrumBuffer>();
		m_sendDFResultHandler = dfResultHandler;
		ClientContext context;
		DirectionRequest request;
		fillDFRequest(request, dfParms);
		DeviceReply reply;
		auto status = m_stub->StartDirection(&context, request, &reply);
		if (status.ok())
		{
			LOG("Start DF");
			//启动获取测向频谱线程
			m_thread[DF_SPECTRUM] = make_unique<ThreadLoop>([this]() {getDFSpectrumLoop(); });
			//启动获取测向目标线程
			m_thread[DF_TARGET] = make_unique<ThreadLoop>([this]() {getDFTargetLoop(); });
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//获取测向结果
	void ZhDeviceControl::getDFTargetLoop()
	{
		LOG("Start GetDFTargetLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetDirectionResult(&context, request));
		DirectionResult result;
		while (m_thread[DF_TARGET]&&m_thread[DF_TARGET]->isRuning()&&reader->Read(&result))
		{
			auto spectrum = m_dfSpectrumbuffer->output();
			auto numPoints = (spectrum.size() > 0) ? spectrum[0].size() : 0;
			if (m_sendDFResultHandler)
			{
#ifdef MOVE_TARGET
				auto pos = position();
				double dir = 0;
				if (pos.longitude() == 120.1691 * M_PI / 180)
					dir = 45;
				if (pos.longitude() == 120.1891 * M_PI / 180)
					dir = 315;
				m_sendDFResultHandler(extractHeader(numPoints, pos, timeVal()), extractDFTarget(result, dir), spectrum);
#else
				m_sendDFResultHandler(extractHeader(numPoints, position(), timeVal()), extractDFTarget(result, m_flightControl->getDirection(result.target_detection().at(0).direction(), 0)), spectrum);
#endif
			}
		}
		LOG("Stop GetDFTargetLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getDFTarget error:" << status.error_message() << endl;
		//}
	}

	//获取测向频谱
	void ZhDeviceControl::getDFSpectrumLoop()
	{
		LOG("Start GetDFSpectrumLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetDirectionSpectrum(&context, request));
		DirectionSpectrum result;
		while (m_thread[DF_SPECTRUM]&&m_thread[DF_SPECTRUM]->isRuning() && reader->Read(&result))
		{
			m_dfSpectrumbuffer->input(result.spectrum());//测向频谱放入缓冲区
		}
		LOG("Stop GetDFSpectrumLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getDFSpectrum error:" << status.error_message() << endl;
		//}
	}

	//停止测向
	senErrorType ZhDeviceControl::stopDF()
	{
		ClientContext context;
		::google::protobuf::Empty request;
		DeviceReply reply;
		auto status = m_stub->StopDirection(&context, request, &reply);
		if (status.ok())
		{
			m_thread[DF_SPECTRUM]->stop();
			m_thread[DF_TARGET]->stop();
			m_dfSpectrumbuffer.reset();
			LOG("Stop DF");
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//开始IQ
	senErrorType ZhDeviceControl::startIQ(const senIQParms& iqParms, const SendIQDataHandler& iqdataHandler)
	{
		m_iqbuffer = make_unique<ZhIQDataBuffer>();
		m_sendIQDataHandler = iqdataHandler;
		ClientContext context;
		IQRequest request;
		fillIQRequest(request, iqParms);
		DeviceReply reply;
		auto status = m_stub->StartIQ(&context, request, &reply);
		if (status.ok())
		{
			//启动获取IQ数据线程
			LOG("Start IQ");
			m_thread[IQ_ACQUIRE] = std::make_unique<ThreadLoop>([this]() {getIQDataLoop(); });
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//获取IQ数据
	void ZhDeviceControl::getIQDataLoop()
	{
		LOG("Start GetIQDataLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetIQData(&context, request));
		IQResult result;
		unsigned long long sequenceNumber = 0;
		while (m_thread[IQ_ACQUIRE] && m_thread[IQ_ACQUIRE]->isRuning())
		{
			if (reader->Read(&result) && result.data_size())
			{
				senVecFloat iqData(result.data().begin(), result.data().end());
				if (iqData.size() <= 2048)
				{
					if (m_sendIQDataHandler)
						m_sendIQDataHandler(extractIQHeader(iqData.size(),
							result.base_band().center_frequency(),
							result.base_band().sample_rate(), sequenceNumber, position(), timeVal()),
							iqData);
					sequenceNumber++;
#ifdef SAVE_BIN_IQDATA
					//string filename = "IQData_"
					//	+ std::to_string(result.base_band().center_frequency() / 1e6) + "MHz_"
					//	+ std::to_string(result.base_band().sample_rate() / 1e6) + "MHz"
					//	+ ".dat";
					//writeBinaryFile(filename, iqData);
#endif 
				}
				else
				{
					m_iqbuffer->input(iqData, IQDataDescribe{
						result.base_band().center_frequency(),
						result.base_band().sample_rate() });
					//启动发送IQ数据线程
					if (m_iqPackNumber == 0)
						m_thread[IQ_SEND] = std::make_unique<ThreadLoop>([this]() {sendIQDataLoop(); });
					m_iqPackNumber++;
				}
				//LOG("----------ZhDeviceControl GetIQData {}", iqData.size());
			}
		}
		LOG("Stop GetIQDataLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getIQData error:" << status.error_message() << endl;
		//}
	}

	//发送IQ数据
	void ZhDeviceControl::sendIQDataLoop()
	{
		LOG("Start SendIQDataLoop");
		unsigned long long sequenceNumber = 0;
		while (m_thread[IQ_SEND] && m_thread[IQ_SEND]->isRuning())
		{
			auto iqFrame = m_iqbuffer->output();
			if (!iqFrame.first.empty())
			{
				if (m_sendIQDataHandler)
					m_sendIQDataHandler(extractIQHeader(iqFrame.first.size(),
						iqFrame.second.centerFreq,
						iqFrame.second.sampleRate, sequenceNumber, position(), timeVal()),
						iqFrame.first);
				sequenceNumber++;
#ifdef SAVE_BIN_IQDATA
				string filename = "IQData_" 
					+ std::to_string(iqFrame.second.centerFreq / 1e6) + "MHz_" 
					+ std::to_string(iqFrame.second.sampleRate / 1e6) + "MHz" 
					+ "_-40dBm_"+".dat";
				writeBinaryFile(filename, iqFrame.first);
#endif 
				CLOG("ZhDeviceControl Send IQData");
			}
			else
			{
				CLOG("IQData Exhausted");
			}
		}
		LOG("Stop SendIQDataLoop");
	}
#ifdef SAVE_BIN_IQDATA
	void ZhDeviceControl::writeBinaryFile(const std::string fileName, const std::vector<float>& data)
	{
		ofstream file(fileName, ios::out | ios::binary | ios::app);
		for (auto da : data)
		{
			file.write((char*)&da, sizeof(float));
		}
		file.close();
		LOG("Write {} IQData To BinaryFile", data.size());
	}
#endif 
	//停止IQ
	senErrorType ZhDeviceControl::stopIQ()
	{
		ClientContext context;
		::google::protobuf::Empty request;
		DeviceReply reply;
		auto status = m_stub->StopIQ(&context, request, &reply);
		if (status.ok())
		{
			if (m_thread[IQ_SEND])
				m_thread[IQ_SEND]->stop();
			m_thread[IQ_ACQUIRE]->stop();
			m_iqPackNumber = 0;
			m_iqbuffer.reset();
			LOG("Stop IQ");
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//开始定频
	senErrorType ZhDeviceControl::startIF(const senIFParms& ifParms, const SendSpectrumHandler& specHandler, const SendIQDataHandler& iqHandler)
	{
		if (m_average == nullptr)
			m_average = std::make_unique<ZhSpectrumAverage>(ifParms.average_count);
		m_sendIFSpectrumHandler = specHandler;
		m_sendIFIQDataHandler = iqHandler;
		ClientContext context;
		SignalAnalysisRequest request;
		fillIFRequest(request, ifParms);
		DeviceReply reply;
		auto status = m_stub->StartSignalAnalysis(&context, request, &reply);
		if (status.ok())
		{
			LOG("Start IF");
			LOG("AverageCount:{}", ifParms.average_count);
			//启动获取定频频谱线程
			m_thread[IF_SPECTRUM] = std::make_unique<ThreadLoop>([this]() {getIFSpectrumLoop(); });
			//启动获取定频IQ线程
			m_thread[IF_IQ] = std::make_unique<ThreadLoop>([this]() {getIFIQDataLoop(); });
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}

	//获取定频频谱
	void ZhDeviceControl::getIFSpectrumLoop()
	{
		LOG("Start GetIFSpectrumLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetSignalAnalysisSpectrum(&context, request));
		SignalAnalysisSpectrum ifSpectrum;
		while (m_thread[IF_SPECTRUM]&&m_thread[IF_SPECTRUM]->isRuning()&&reader->Read(&ifSpectrum))
		{
			senVecFloat senSpretrum(ifSpectrum.spectrum().amplitude().begin(), ifSpectrum.spectrum().amplitude().end());
			auto averageData = m_average->getAverageData(senSpretrum);
			if (m_sendIFSpectrumHandler && !averageData.empty())
			{
				m_sendIFSpectrumHandler(extractHeader(averageData.size(), position(), timeVal()), averageData);
			}
		}
		LOG("Stop GetIFSpectrumLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getIFSpectrum error:" << status.error_message() << endl;
		//}
	}

	//获取定频IQ
	void ZhDeviceControl::getIFIQDataLoop()
	{
		LOG("Start GetIFIQDataLoop");
		ClientContext context;
		::google::protobuf::Empty request;
		auto reader(m_stub->GetSignalAnalysisIQ(&context, request));
		IQResult result;
		while (m_thread[IF_IQ]&&m_thread[IF_IQ]->isRuning()&&reader->Read(&result))
		{
			if (result.data_size())
			{
				senVecFloat iqData(result.data().begin(), result.data().end());
				//std::vector<float> iqData(result.data_size());
				//memcpy(iqData.data(), &result.data()[0], iqData.size() * sizeof(float));
				if (m_sendIFIQDataHandler)
					m_sendIFIQDataHandler(
						extractIQHeader(iqData.size(),
							result.base_band().center_frequency(),
							result.base_band().sample_rate()),
						iqData);
			}
		}
		LOG("Stop GetIFIQDataLoop");
		//Status status = reader->Finish();
		//if (!status.ok())
		//{
		//	cout << "getIFIQData error:" << status.error_message() << endl;
		//}
	}

	//停止定频
	senErrorType ZhDeviceControl::stopIF()
	{
		ClientContext context;
		::google::protobuf::Empty request;
		DeviceReply reply;
		auto status = m_stub->StopSignalAnalysis(&context, request, &reply);
		if (status.ok())
		{
			m_thread[IF_SPECTRUM]->stop();
			m_thread[IF_IQ]->stop();
			if (m_average)
				m_average.reset();
			LOG("Stop IF");
			return SEN_ERR_NONE;
		}
		else
		{
			return SEN_ERR_UNKNOWN;
		}
	}
}