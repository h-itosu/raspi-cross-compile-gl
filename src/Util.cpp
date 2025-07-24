#include <sys/sysinfo.h>
#include <stdexcept>
#include "Util.h"

// /proc/meminfo から MemAvailable を取得し、利用可能なメモリ量（kB単位）を返す
long util::getAvailableMemory()
{
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open())
    {
        std::cerr << "Error: Cannot open /proc/meminfo" << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(meminfo, line))
    {
        if (line.rfind("MemAvailable:", 0) == 0)
        {
            // "MemAvailable:" の後の数値部分を抽出
            // 例: "MemAvailable:   123456 kB"
            try
            {
                return std::stol(line.substr(13, line.find(" kB") - 13));
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Error parsing memory info: " << e.what() << std::endl;
                return -1;
            }
        }
    }
    return -1; // MemAvailableが見つからなかった場合
}

long util::LogAvailableMemory()
{
    long memKB = getAvailableMemory();
    std::cout << "Available Memory: " << memKB << " KB" << std::endl;
    return memKB;
}

// 現在時刻（エポックからのナノ秒などのraw値）でタイマーを開始
void util::StartTimer()
{
    _startTime = std::chrono::system_clock::now().time_since_epoch().count();
}

// タイマー開始からの経過時間（ナノ秒などのraw値）を返す
long util::GetElapsedTime()
{
    long currentTime = std::chrono::system_clock::now().time_since_epoch().count();
    return currentTime - _startTime;
}

// タイマー開始からの経過時間（秒単位）を返す
long util::GetElapsedTimeSec()
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = now.time_since_epoch() - std::chrono::system_clock::duration(_startTime);
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}

// 経過秒数をログ出力し、その値を返す
long util::LogElapsedTimeSec()
{
    long elapsed = GetElapsedTimeSec();
    std::cout << "Elapsed time: " << elapsed << " seconds" << std::endl;
    return elapsed;
}

// 経過HH:MM:SSログ出力し、その値を返す
long util::LogElapsedTimeHMS()
{
    long elapsed = GetElapsedTimeSec();
    long hours = elapsed / 3600;
    long minutes = (elapsed % 3600) / 60;
    long seconds = elapsed % 60;
    std::cout << "Elapsed time: " << hours << "h " << minutes << "m " << seconds << "s" << std::endl;
    return elapsed;
}
