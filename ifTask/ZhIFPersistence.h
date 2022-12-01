/**
 * @file ZhIFPersistence.h
 * @brief 定频分析数据记录
 * @author 装备事业部软件组 王克东 
 * @version 0.1
 * @date 2022-7-18
 * 
 * @copyright Copyright (c) 2022  中国电子科技集团公司第四十一研究所
 * 
 */
#pragma once

#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "Miscellaneous.h"
#include <functional>

using namespace sqlite_orm;

namespace persistence
{
    struct IFDataItem {
        int seq;
        std::string datauuid;
        std::string taskuuid;
        double longitude;
        double latitude;
        double altitude;
        uint64_t startTime;
        uint64_t stopTime;
        uint32_t sweepCount;
        std::vector<char> spectrum;
        std::vector<char> hits;
        std::vector<char> signal;
    };

    struct IFTaskItem {
        int seq;
        std::string uuid;
        uint32_t nodeId;
        uint32_t deviceId;
        uint32_t points;
        uint32_t sweepCount;
        uint32_t dataItemCount;
        uint64_t startTime;
        uint64_t stopTime;
        int64 centerFreq;
        int64 bandWidth;
    };

    inline auto initIFTaskStorage(const std::string& path)
    {
        return make_storage(
            path,
            make_table("IFTask",
                make_column("seq",           &IFTaskItem::seq, primary_key()),
                make_column("uuid",         &IFTaskItem::uuid),
                make_column("nodeId",       &IFTaskItem::nodeId),
                make_column("deviceId",     &IFTaskItem::deviceId),
                make_column("points",       &IFTaskItem::points),
                make_column("sweepCount",   &IFTaskItem::sweepCount),
                make_column("dataItemCount",&IFTaskItem::dataItemCount),
                make_column("startTime",    &IFTaskItem::startTime),
                make_column("stopTime",     &IFTaskItem::stopTime),
                make_column("centerFreq",    &IFTaskItem::centerFreq),
                make_column("bandWidth",     &IFTaskItem::bandWidth)
            ),
            make_table("IFData",
                make_column("seq",           &IFDataItem::seq, primary_key()),
                make_column("datauuid",      &IFDataItem::datauuid),
                make_column("taskuuid",      &IFDataItem::taskuuid),
                make_column("longitude",    &IFDataItem::longitude),
                make_column("latitude",     &IFDataItem::latitude),
                make_column("altitude",     &IFDataItem::altitude),
                make_column("startTime",    &IFDataItem::startTime),
                make_column("stopTime",     &IFDataItem::stopTime),
                make_column("sweepCount",   &IFDataItem::sweepCount),
                make_column("spectrum",     &IFDataItem::spectrum),
                make_column("hits",         &IFDataItem::hits),
                make_column("signal",        &IFDataItem::signal)
            )
        );
    }

    using IFDataItemHandler = std::function<void(const IFDataItem&)>;

    class IFDBAccessor
    {
    public:
        IFDBAccessor(const std::string& path, std::mutex& dblock)
            :storage(initIFTaskStorage(path)),
            lock(dblock)
        {
            storage.sync_schema();
        }
    public:
        int insertTask(const IFTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(taskItem);
        }

        void updateTask(const IFTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            storage.update(taskItem);
        }

        int insertData(const IFDataItem& dataItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(dataItem);
        }

        std::vector<std::string> findMatchTask(int64 centerFreq, int64 bandWidth, uint32_t points, uint64_t startTime = 0, uint64_t stopTime = 0)
        {
            std::vector<std::string> taskuuid;
            std::lock_guard<std::mutex> lg(lock);
            if (startTime == 0 || stopTime == 0)
            {
                auto taskItems = storage.iterate<IFTaskItem>(
                    where(
                        c(&IFTaskItem::centerFreq) == centerFreq and
                        c(&IFTaskItem::bandWidth) == bandWidth and
                        c(&IFTaskItem::points) == points
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            else
            {
                auto taskItems = storage.iterate<IFTaskItem>(
                    where(
                        c(&IFTaskItem::centerFreq) == centerFreq and
                        c(&IFTaskItem::bandWidth) == bandWidth and
                        c(&IFTaskItem::points) == points and
                        c(&IFTaskItem::startTime) > startTime and
                        c(&IFTaskItem::stopTime) < stopTime
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            return taskuuid;
        }

        std::unique_ptr<IFTaskItem> getTaskItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto all = storage.get_all<IFTaskItem>(where(c(&IFTaskItem::uuid) == taskuuid));
            if (all.empty())
                return nullptr;
            else
                return std::make_unique<IFTaskItem>(all[0]);
        }

        std::vector<IFTaskItem> findTask(const spectrum::SignalBand* band, const TimeSpan* timeSpan)
        {
            if (band && timeSpan)
            {
                auto center = static_cast<int64_t>(band->center_frequency());
                auto bandwidth = static_cast<int64_t>(band->band_width());
                auto timeStart = timestampToUINT64(timeSpan->start_time());
                auto timeStop = timestampToUINT64(timeSpan->stop_time());
                return storage.get_all<IFTaskItem>(where(
                    c(&IFTaskItem::centerFreq) == center and
                    c(&IFTaskItem::bandWidth) == bandwidth and
                    c(&IFTaskItem::startTime) > timeStart and
                    c(&IFTaskItem::stopTime) < timeStop)
                );   
            }
            else
            {
                if (band)
                {
                    auto center = static_cast<int64_t>(band->center_frequency());
                    auto bandwidth = static_cast<int64_t>(band->band_width());
                    return storage.get_all<IFTaskItem>(where(
                        c(&IFTaskItem::centerFreq) == center and
                        c(&IFTaskItem::bandWidth) == bandwidth)
                        );
                }
                else
                {
                    auto timeStart = timestampToUINT64(timeSpan->start_time());
                    auto timeStop = timestampToUINT64(timeSpan->stop_time());
                    return storage.get_all<IFTaskItem>(where(
                        c(&IFTaskItem::startTime) > timeStart and
                        c(&IFTaskItem::stopTime) < timeStop)
                        );
                }
            }
        }

        static void toTaskDescriptor(const IFTaskItem& dbItem, zczh::zhIFAnalysis::RecordDescriptor& desc)
        {
            NodeDevice nd;
            nd.mutable_node_id()->set_value(dbItem.nodeId);
            nd.mutable_device_id()->set_value(dbItem.nodeId);
            desc.set_record_id(dbItem.uuid);
            *desc.mutable_from() = nd;
            *desc.mutable_time_span()->mutable_start_time() = UINT64ToTimestamp(dbItem.startTime);
            *desc.mutable_time_span()->mutable_stop_time() = UINT64ToTimestamp(dbItem.stopTime);
            desc.mutable_analysis_band()->set_center_frequency(dbItem.centerFreq);
            desc.mutable_analysis_band()->set_band_width(dbItem.bandWidth);
            desc.set_record_count(dbItem.dataItemCount);
        }

        std::vector<IFDataItem> getTaskAllDataItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.get_all<IFDataItem>(where(
                c(&IFDataItem::taskuuid) == taskuuid),
                order_by(&IFDataItem::seq)
                );
            return std::vector<IFDataItem>();
        }

        int useDataInSpan(const std::vector<std::string>& taskuuId, const zb::dcts::TimeSpan& span, int maxRecordCount, IFDataItemHandler handler)
        {
            std::lock_guard<std::mutex> lg(lock);
            int loadedCount = 0;
            uint64_t startTime = timestampToUINT64(span.start_time());
            uint64_t stopTime = timestampToUINT64(span.stop_time());
            uint64_t timeStep = (stopTime - startTime) / maxRecordCount;
            for (int i = 0; i < maxRecordCount; i++)
            {
                auto dataItems = storage.iterate<IFDataItem>(
                    where(
                        in(&IFDataItem::taskuuid, taskuuId) and
                        c(&IFDataItem::startTime) >= startTime and
                        c(&IFDataItem::stopTime)  <= (startTime + timeStep)
                        )
                    );
                startTime += timeStep;
                for(auto& item : dataItems)
                {
                    handler(item);
                    loadedCount++;
                    break;
                }
            }
            return loadedCount;
        }
    private:
        using Storage = decltype(initIFTaskStorage(""));
        Storage storage;
        std::mutex& lock;
    };
}


