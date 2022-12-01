#pragma once
#include "ZhFlightControl.h"
#include "ZhSpectrumBuffer.h"
#include "ZhIQDataBuffer.h"
#include "ThreadLoop.h"
#include "cmath"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using namespace zb::dcts::node::module::spatailSpectrum::internal;

using SendSpectrumHandler = std::function<void(const senSegmentData& header, const senVecFloat& spectrum)>;
using SendDFResultHandler = std::function<void(const senSegmentData& header, const std::vector<senDFTarget>& dfTarget, const std::vector<senVecFloat>& dfSpectrum)>;
using SendIQDataHandler = std::function<void(const senIQSegmentData& header, const senVecFloat& iqdata)>;
using SendSignalHandler = std::function<void(const std::vector<senSignal>& analysis)>;

namespace ZBDevice
{
	class ZhFlightControl;
	class ThreadLoop;
	class ZhSpectrumBuffer;
	class ZhIQDataBuffer;
	class ZhSpectrumAverage;
	class ZhDeviceControl
	{
	public:
		ZhDeviceControl(std::string devCtrlSerAddress);
		~ZhDeviceControl();

	public:
		bool isConnect();
		Position position();
		struct timeval timeVal();
		SYSTEMTIME systemTime();
		std::vector<Physical> physical();
		zb::dcts::node::DeviceStatus deviceStatus();
		senErrorType startPano(const senPanoParms& panoParms, const SendSpectrumHandler& panoDataHandler);
		senErrorType stopPano();
		senErrorType startDF(const senDFParms& dfParms, const SendDFResultHandler& dfResultHandler);
		senErrorType stopDF();
		senErrorType startIQ(const senIQParms& iqParms, const SendIQDataHandler& iqDataHandle);
		senErrorType stopIQ();
		senErrorType startIF(const senIFParms& ifParms, const SendSpectrumHandler& spectrumHandler, const SendIQDataHandler& iqDataHandler);
		senErrorType stopIF();

	private:
		void createStub(std::string devCtrlSerAddress);
		void getDeviceStatusLoop();
		void getPScanSpectrumLoop();
		void getDFTargetLoop();
		void getDFSpectrumLoop();
		void getIQDataLoop();
		void sendIQDataLoop();
#ifdef SAVE_BIN_IQDATA
		void writeBinaryFile(const std::string fileName, const std::vector<float>& data);
#endif	
		void getIFSpectrumLoop();
		void getIFIQDataLoop();

	private:
		std::string m_devCtrlSerAddress;
		std::unique_ptr<DeviceControlService::Stub> m_stub;
		zb::dcts::node::module::spatailSpectrum::internal::DeviceStatus m_devStatus;
		senPanoParms m_panoParms;
		SendSpectrumHandler m_sendPScanSpectrumHandler;
		std::unique_ptr<ZhSpectrumBuffer> m_dfSpectrumbuffer;//测向频谱缓冲区
		SendDFResultHandler m_sendDFResultHandler;
		SendIQDataHandler m_sendIQDataHandler;
		unsigned long long m_iqPackNumber;
		std::unique_ptr<ZhIQDataBuffer> m_iqbuffer;//IQ数据缓冲区
		SendSpectrumHandler m_sendIFSpectrumHandler;
		SendIQDataHandler m_sendIFIQDataHandler;
		std::map<senThreadType, std::unique_ptr<ThreadLoop>> m_thread;
		std::unique_ptr<ZhFlightControl> m_flightControl;
		std::unique_ptr<ZhSpectrumAverage> m_average;
		std::vector<Physical> m_physicalList;
	};
}