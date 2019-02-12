#ifndef TIMEZONEMANAGER_H
#define TIMEZONEMANAGER_H

#include <map>
#include <string>
#include <vector>


struct DSTInfo
{
    bool dst;                   // 是否支持夏令时
    std::string zone_name;      // 时区名  (UTC-10:00) America/Adak
    std::string in_time;
    std::string out_time;
};

class TimeZoneManager
{
public:
    TimeZoneManager();

    bool Init(int year = -1);
    void Uninit();

    bool GetInfo(DSTInfo& info, int year, const std::string& time_zone);

protected:
    bool GetStringListFromFile(std::vector<std::string>& str_vec, const std::string& file_name);

    bool GetStringListByCmd(std::vector<std::string>& str_vec, const std::string& cmd);

private:
    std::map<std::string, DSTInfo> info_map_;
    std::string version_;
    int current_year_ = -1;
    bool inited_ = false;
};

#endif // TIMEZONEMANAGER_H
