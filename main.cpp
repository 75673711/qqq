#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

bool GetStringListFromFile(vector<string>& str_vec, const string& file_name)
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

int main()
{
    string temp_file;

    system("date > /Users/wanghaodong/Documents/QTproject/TimeZone/haha");

    vector<string> time_zones;
    if (GetStringListFromFile(time_zones, "/Users/wanghaodong/Documents/QTproject/TimeZone/hehe"))
    {
        cout << "success" << endl;
        for (string it : time_zones)
        {
            cout << it << endl;
        }
    }
    else
    {
        cout << "failed" << endl;
    }

    return 0;
}
