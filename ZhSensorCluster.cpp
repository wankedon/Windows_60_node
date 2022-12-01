#include "pch.h"
#include "ZhSensorCluster.h"
#include "Logger.h"

using namespace std;

ZhSensorCluster::ZhSensorCluster()
{

}

ZhSensorCluster::ZhSensorCluster(const LANConfig& cfg)
	:m_cfg(cfg)
{

}

ZhSensorCluster::~ZhSensorCluster()
{

}

void ZhSensorCluster::discoveredSensor(std::vector<senSensorInfo>& info, std::vector<senSensorStatus>& status)
{
	for (auto devConf : m_cfg.cluster_3900())
	{
		senSensorInfo sensorInfo;
		senSensorStatus sensorStatus;
		strcpy(sensorInfo.ipAddress, devConf.address().ip().c_str());
		info.emplace_back(sensorInfo);
		strcpy(sensorStatus.ipAddress, devConf.address().ip().c_str());
		status.emplace_back(sensorStatus);
	}
}

void ZhSensorCluster::refresh()
{
	if (m_cfg.cluster_3900_size() == 0)
		return;
	std::vector<senSensorInfo> info;
	std::vector<senSensorStatus> status;
	discoveredSensor(info, status);
	updateByInfo(info);
	updateByStatus(status);
}

void ZhSensorCluster::updateByInfo(const vector<senSensorInfo>& info)
{
	std::lock_guard<std::mutex> lg(mtx);
	for (auto& i : info)
	{
		if (sensors.find(i.ipAddress) == sensors.end())
		{
			if (m_sensor == nullptr)
				m_sensor = make_shared<ZBDevice::ZhSensor>(i);
			bool connected = m_sensor->connect();//建立连接
			if (connected)
			{
				sensors[i.ipAddress] = m_sensor;
				devices[m_sensor->id().value()] = m_sensor;
			}
			LOG("zczh_sensor {} connected {}", string(i.ipAddress), connected);
		}
	}
}

void ZhSensorCluster::updateByStatus(const vector<senSensorStatus>& status)
{
	std::lock_guard<std::mutex> lg(mtx);
	for (auto& sta: status)
	{
		auto iter = sensors.find(sta.ipAddress);
		if (iter != sensors.end())
		{
			iter->second->updateStatus(sta);
		}
	}
}