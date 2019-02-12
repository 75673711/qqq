#include "timezonemanager.h"

#include <fstream>
#include <iostream>


using namespace std;

static string time_zone_file = "/Users/wanghaodong/Documents/QTproject/TimeZone/hehe";

static string temp_file = "/Users/wanghaodong/Documents/QTproject/TimeZone/haha";

string MakeDSTCmd(int year, const string& time_zone)
{
    return "zdump -v " + time_zone + " | grep " + std::to_string(year);
}

TimeZoneManager::TimeZoneManager()
{

}

bool TimeZoneManager::Init(int year)
{
    if (inited_)
    {
        if (year > current_year_)
        {
            Uninit();
        }
        else
        {
            return true;
        }
    }

    vector<string> time_zones;
    if (GetStringListFromFile(time_zones, time_zone_file))
    {
        for (string it : time_zones)
        {
            cout << it << endl;
            DSTInfo info;
            info.in_time = it;

            vector<string> output;
            if (!GetStringListByCmd(output, MakeDSTCmd(2019, it)))
            {
                cout << "error ---- " << it;
                continue;
            }

            info_map_[it] = info;
        }
    }
    else
    {
        cout << "failed" << endl;

        return false;
    }

    current_year_ = year;
    inited_ = true;
    return inited_;
}

void TimeZoneManager::Uninit()
{
    if (inited_)
    {
        info_map_.clear();
        current_year_ = -1;
        version_ = "";

        inited_ = false;
    }
}

bool TimeZoneManager::GetInfo(DSTInfo& info, int year, const string &time_zone)
{
    if (inited_ && current_year_ == year)
    {
        // todo: map if exist
        info = info_map_[time_zone];
        return true;
    }

    return false;
}

bool TimeZoneManager::GetStringListByCmd(std::vector<std::string>& str_vec, const std::string& cmd)
{
    ifstream file(cmd);
    if (file.is_open()) {
        file.close();
        remove(temp_file.c_str());
    }

    string str_cmd = cmd + " > " + temp_file;
    system(str_cmd.c_str());

    bool result = GetStringListFromFile(str_vec, temp_file);
    remove(temp_file.c_str());
    return result;
}

bool TimeZoneManager::GetStringListFromFile(vector<string>& str_vec, const string& file_name)
{
    ifstream file(file_name);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (!line.empty())
            {
                str_vec.push_back(line);
            }
        }
        file.close();

        return true;
    }

    return false;
}
