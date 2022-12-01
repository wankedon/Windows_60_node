#pragma once
#include "DeviceCluster.h"
#include "ZhSensor.h"

class ZhSensorCluster : public DeviceCluster
{
public:
	ZhSensorCluster();
	ZhSensorCluster(const LANConfig& cfg);
	~ZhSensorCluster();
public:
	void refresh() override;
private:
	void discoveredSensor(std::vector<senSensorInfo>& info, std::vector<senSensorStatus>& status);
	void updateByInfo(const std::vector<senSensorInfo>& info);
	void updateByStatus(const std::vector<senSensorStatus>& status);
private:
	std::map<std::string, std::shared_ptr<ZBDevice::ZhSensor>> sensors;
	LANConfig m_cfg;
	std::shared_ptr<ZBDevice::ZhSensor> m_sensor;
};