#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread> // std::this_thread::sleep_for のためにインクルード

// ユーティリティクラス: メモリ取得や経過時間計測などの便利関数を提供
class util
{
private:
    long _startTime; // タイマー開始時刻（ナノ秒などのエポック値）

public:
    // 利用可能なメモリ量（バイト単位）を取得
    static long getAvailableMemory();

    static long LogAvailableMemory();

    // タイマーを現在時刻で開始
    void StartTimer();

    // タイマー開始からの経過時間（ナノ秒などのraw値）を取得
    long GetElapsedTime();

    // タイマー開始からの経過時間（秒単位）を取得
    long GetElapsedTimeSec();

    // 経過秒数をログ出力し、その値を返す
    long LogElapsedTimeSec();

    // 経過HH:MM:SSログ出力し、その値を返す
    long LogElapsedTimeHMS();
};
