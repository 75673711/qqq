#include "timezonemanager.h"

#include <fstream>
#include <iostream>

#include <sstream>
#include <iterator>

#include <time.h>

using namespace std;

static string time_zone_file = "/Users/wanghaodong/Documents/QTproject/TimeZone/hehe";

static string temp_file = "/Users/wanghaodong/Documents/QTproject/TimeZone/haha";

// important: 会改变入参
time_t UTCmktime(tm* tm_in)
{
    tm_in->tm_isdst = -1;
    return mktime(tm_in) - timezone;
}

string SecsToHourMinTimeString(int secs)
{
    int offset_sec = secs;
    string c = offset_sec >= 0 ? "+" : "-";
    if (offset_sec < 0)
    {
        offset_sec = -offset_sec;
    }
    int hour = offset_sec / 3600;
    int min = (offset_sec % 3600) / 60;
    return c + (hour < 10 ? "0" : "") + std::to_string(hour)
            + ":" + (min < 10 ? "0" : "") + std::to_string(min);
}

// skip_empty为true时 跳过空字符串
template<typename Out>
void split(const std::string& s, char delim, bool skip_empty, Out result)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        if (skip_empty && item.empty())
        {
            continue;
        }
        *(result++) = item;
    }
}

string MakeDSTCmd(const string& time_zone, int year = -1)
{
    if (year == -1)
    {
        return "zdump -v " + time_zone;
    }

    return "zdump -v " + time_zone + " | grep " + std::to_string(year);
}

string GetUTCOffset(const vector<string>& str_vec)
{
    for (int i = str_vec.size() - 1; i >= 0 ; --i)
    {
        string line = str_vec[i];
        if (line.find("isdst=0") != string::npos)
        {
            vector<string> sub_strings;
            split(line, ' ', true, std::back_inserter(sub_strings));
            if (sub_strings.size() == 0)
            {
                assert(false);
                break;
            }

            string local_time_str = sub_strings[1] + " "
                    + sub_strings[2] + " "
                    + sub_strings[3] + " "
                    + sub_strings[4] + " "
                    + sub_strings[5];
            string utc_time_str = sub_strings[8] + " "
                    + sub_strings[9] + " "
                    + sub_strings[10] + " "
                    + sub_strings[11] + " "
                    + sub_strings[12];

            struct tm local_time;
            strptime(local_time_str.c_str(), "%c", &local_time);
            struct tm utc_time;
            strptime(utc_time_str.c_str(), "%c", &utc_time);

            time_t local_sec = UTCmktime(&local_time);
            time_t utc_sec = UTCmktime(&utc_time);
            int offset_sec = utc_sec - local_sec;
            return "(UTC" + SecsToHourMinTimeString(static_cast<int>(offset_sec)) + ") " + sub_strings[0];
        }
    }

    return string();
}

bool GetUTCInAndOut(const vector<string>& str_vec, string& in_time, string& out_time)
{
    if (str_vec.size() != 4)
    {
        cout << "GetUTCInAndOut size invalid !!!" << endl;
        assert(false);
        return false;
    }

    auto GetTime = [=](const string& line, string& local, string& utc) {
        vector<string> sub_strings;
        split(line, ' ', true, std::back_inserter(sub_strings));
        if (sub_strings.size() == 0)
        {
            assert(false);
            return;
        }

        local = sub_strings[1] + " "
                + sub_strings[2] + " "
                + sub_strings[3] + " "
                + sub_strings[4] + " "
                + sub_strings[5];
        utc = sub_strings[8] + " "
                + sub_strings[9] + " "
                + sub_strings[10] + " "
                + sub_strings[11] + " "
                + sub_strings[12];
    };

    string local_0, utc_0;
    GetTime(str_vec[0], local_0, utc_0);
    string local_1, utc_1;
    GetTime(str_vec[1], local_1, utc_1);
    string local_2, utc_2;
    GetTime(str_vec[2], local_2, utc_2);
    string local_3, utc_3;
    GetTime(str_vec[3], local_3, utc_3);

    struct tm utc_1_tm;
    strptime(utc_1.c_str(), "%c", &utc_1_tm);
    struct tm utc_0_tm;
    strptime(utc_0.c_str(), "%c", &utc_0_tm);
    struct tm utc_3_tm;
    strptime(utc_3.c_str(), "%c", &utc_3_tm);
    struct tm utc_2_tm;
    strptime(utc_2.c_str(), "%c", &utc_2_tm);

    // 先是dst == 1则为南半球   第一个为0的是退出夏令时
    if (str_vec[0].find("isdst=1") != string::npos)
    {
        cout << "south" << endl;
        // 拿  3 1 整  2 0 非整
        // utc in 3-2    out 1-0 偏移 +1s


        time_t in_offset = UTCmktime(&utc_3_tm) - UTCmktime(&utc_2_tm) - time_t(1);

        string in_by = SecsToHourMinTimeString(static_cast<int>(in_offset));

        time_t out_offset = UTCmktime(&utc_1_tm) - UTCmktime(&utc_0_tm) - time_t(1);

        string out_by = SecsToHourMinTimeString(static_cast<int>(out_offset));

        in_time = local_3 + " by " + in_by;
        out_time = local_1 + " by " + out_by;

        return true;
    }

    if (str_vec[0].find("isdst=0") != string::npos)
    {
        cout << "north" << endl;
        // 拿  1 3整    0 2 非整
        // utc in 1-0    out 3-2 偏移 +1s
        time_t in_offset = UTCmktime(&utc_1_tm) - UTCmktime(&utc_0_tm) - time_t(1);

        string in_by = SecsToHourMinTimeString(static_cast<int>(in_offset));

        time_t out_offset = UTCmktime(&utc_3_tm) - UTCmktime(&utc_2_tm) - time_t(1);

        string out_by = SecsToHourMinTimeString(static_cast<int>(out_offset));

        in_time = local_1 + " by " + in_by;
        out_time = local_3 + " by " + out_by;

        return true;
    }

    // 先是dst == 0则为北半球   第一个为0的是进入夏令时
    cout << "where ?" << endl;
    return false;
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
            //cout << it << endl;
            DSTInfo info;
            info.in_time = it;

            bool exist_dst = true;
            vector<string> output;
            if (!GetStringListByCmd(output, MakeDSTCmd(it, 2019)))
            {
                cout << "error ---- " << it;
                continue;
            }

            if (output.size() == 0)
            {
                exist_dst = false;
                if (!GetStringListByCmd(output, MakeDSTCmd(it)))
                {
                    cout << "error ---- " << it;
                    continue;
                }
            }
            else
            {
                for (auto it : output)
                {
                    cout << it << endl;
                }
                cout << "------------";
                // get utc in and out
                if (!GetUTCInAndOut(output, info.in_time, info.out_time))
                {
                    cout << "dst in and out error ---- " << it;
                    continue;
                }

                // todo: 冬令时如何？
            }

            // todo: output
            // get utc offset
            string utc_offset = GetUTCOffset(output);
            info.zone_name = utc_offset;
            info.dst = exist_dst;
            cout << utc_offset << info.in_time << info.out_time << endl;

            info_map_[it] = info;
        }
    }
    else
    {
        cout << "failed" << endl;

        return false;
    }

    cout << "init success" << endl;

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
