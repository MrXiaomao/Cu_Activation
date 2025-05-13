/*
 * @Author: MaoXiaoqing
 * @Date: 2025-04-06 20:15:30
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-05-13 09:33:57
 * @Description: 符合计算算法
 */
#include "coincidenceanalyzer.h"
#include <queue>
// #include "linearfit.h"
#include "gaussFit.h"
#include "sysutils.h"

using namespace std;

struct particle_data
{
    long long time;
    unsigned short energy;
    unsigned char detectorID;
    particle_data(){}
    particle_data(long long t, unsigned short en, unsigned char id):time(t),energy(en),detectorID(id){}
    friend bool operator<(particle_data a, particle_data b)
    {
        return a.time > b.time; //按x升序,要升序的话，改这里的大于小于号就行
    }
};

//priority_queue<particle_data> q;//此时创建的优先队列是按x大小升序排列的

CoincidenceAnalyzer::CoincidenceAnalyzer():
countCoin(0), coolingTime_Manual(0), coolingTime_Auto(0), isChangeEnWindow(false),autoFirst(true),
GaussCountMin(200000),
#ifdef QT_DEBUG
GaussMinGapTime(300)
#else
GaussMinGapTime(10)
#endif
{
    for(unsigned short i=0; i<MULTI_CHANNEL; i++)
    {
        GaussFitSpec.time = 0;
        GaussFitSpec.spectrum[0][i] = 0;
        GaussFitSpec.spectrum[1][i] = 0;
    }
    EnergyWindow[0] = 1;
    EnergyWindow[1] = MULTI_CHANNEL - 1;
    EnergyWindow[2] = 1;
    EnergyWindow[3] = MULTI_CHANNEL - 1;
}

void CoincidenceAnalyzer::set_callback(std::function<void(SingleSpectrum, vector<CoincidenceResult>)> func)
{
    m_pfunc = func;
}

void CoincidenceAnalyzer::set_callback(pfRealCallbackFunction func, void *callbackUser)
{
    m_pfuncEx = func;
    m_pfuncUser = callbackUser;
}

void CoincidenceAnalyzer::calculate(vector<TimeEnergy> _data1, vector<TimeEnergy> _data2,
              unsigned short E_win[4], int windowWidthT, int delayTime, 
              bool countFlag, bool autoEnWidth)
{
    // 如果是自动调整窗宽，则只在测量开始的首次读取窗宽，后续都自动更新窗宽
    if(autoEnWidth) {
        if (autoFirst){
            EnergyWindow[0] = E_win[0];
            EnergyWindow[1] = E_win[1];
            EnergyWindow[2] = E_win[2];
            EnergyWindow[3] = E_win[3];
            autoFirst = false;
        }
    }
    else {
        EnergyWindow[0] = E_win[0];
        EnergyWindow[1] = E_win[1];
        EnergyWindow[2] = E_win[2];
        EnergyWindow[3] = E_win[3];
    }

    //合并上次没处理完的数据与新数据
    //优化1 减少容器拷贝，使用预分配空间
    unusedData1.reserve(unusedData1.size() + _data1.size());
    unusedData1.insert(unusedData1.end(), _data1.begin(), _data1.end());

    unusedData2.reserve(unusedData2.size() + _data2.size());
    unusedData2.insert(unusedData2.end(), _data2.begin(), _data2.end());

    if ((unusedData1.size()==0) && (unusedData2.size() == 0)){
        return;
    }

    // 准备计算
    int time1_elapseFPGA = 0;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
    int time2_elapseFPGA = 0;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
    if(unusedData1.size()>0) 
        time1_elapseFPGA = unusedData1.back().time/NANOSECONDS - (countCoin + coolingTime_Auto);//计算FPGA当前最大时间与上一时刻的时间差
    if(unusedData2.size()>0) 
        time2_elapseFPGA = unusedData2.back().time/NANOSECONDS - (countCoin + coolingTime_Auto);//计算FPGA当前最大时间与上一时刻的时间差

    int deltaT = 1; //单位秒
    //都存够1秒的数据才进行处理，或者其中某一个存满2s数据。
    while((time1_elapseFPGA >= deltaT && time2_elapseFPGA >= deltaT) || time1_elapseFPGA>1 || time2_elapseFPGA>1)
    {
        isChangeEnWindow = false;
        //先计算出当前一秒的数据点个数,若都没有完整一秒数据时，直接退出计算，下一次处理。
        GetDataPoint(unusedData1, unusedData2);

        //对当前一秒数据处理给出能谱
        calculateAllSpectrum(unusedData1, unusedData2);

        SingleSpectrum tempSpec = AllSpectrum.back();
        for(unsigned short i=0; i<MULTI_CHANNEL - 3; i++)
        {
            recentlySpectrum.time = tempSpec.time;
            recentlySpectrum.spectrum[0][i] += tempSpec.spectrum[0][i];
            recentlySpectrum.spectrum[1][i] += tempSpec.spectrum[1][i];
        }

        //计算计数曲线
        if (countFlag)
        {   
            //自动拟合，更新能窗
            if(autoEnWidth) 
            {
                //SingleSpectrum tempSpec = AllSpectrum.back();
                for(unsigned short i=0; i<MULTI_CHANNEL - 3; i++)
                {
                    GaussFitSpec.time = tempSpec.time;
                    GaussFitSpec.spectrum[0][i] += tempSpec.spectrum[0][i];
                    GaussFitSpec.spectrum[1][i] += tempSpec.spectrum[1][i];
                    // recentlySpectrum.time = tempSpec.time;
                    // recentlySpectrum.spectrum[0][i] += tempSpec.spectrum[0][i];
                    // recentlySpectrum.spectrum[1][i] += tempSpec.spectrum[1][i];
                }

                //距离上次自动高斯拟合的时间间隔，单位：s
                int GapTime = countCoin;
                if (GaussFitLog.size() > 0)
                    GapTime -= GaussFitLog.back().time;
                if(GapTime>GaussMinGapTime)  AutoEnergyWidth();

                //若更新了能窗，则重置相关数据
                if(isChangeEnWindow){
                    recentlySpectrum = GaussFitSpec; //更新这个最近能谱
                    memset(&GaussFitSpec, 0, sizeof(GaussFitSpec));
                    // for(unsigned short i=0; i<MULTI_CHANNEL; i++)
                    // {
                    //     GaussFitSpec.time = 0;//注意这里是在探测器2之后才重置
                    //     GaussFitSpec.spectrum[0][i] = 0;
                    //     GaussFitSpec.spectrum[1][i] = 0;
                    // }
                }
            }

            //对当前一秒数据处理给出各自的计数以及符合计数
            Coincidence(unusedData1, unusedData2, EnergyWindow, windowWidthT, delayTime);
        }

        //删除容器中已经处理的数据点
        if (AllPoint.back().dataPoint1 <= (int)unusedData1.size()) {
            // 交换法，适合大规模数据
            vector<TimeEnergy>(unusedData1.begin() + AllPoint.back().dataPoint1, unusedData1.end()).swap(unusedData1);
        } else {
            unusedData1.clear(); // 如果N大于容器的大小，清空容器
        }

        if (AllPoint.back().dataPoint2 <= (int)unusedData2.size()) {
            // 交换法，适合大规模数据
            vector<TimeEnergy>(unusedData2.begin() + AllPoint.back().dataPoint2, unusedData2.end()).swap(unusedData2);
        } else {
            unusedData2.clear(); // 如果N大于容器的大小，清空容器
        }

        time1_elapseFPGA = 0;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
        time2_elapseFPGA = 0;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
        if(unusedData1.size()>0)
            time1_elapseFPGA = unusedData1.back().time/NANOSECONDS - (countCoin + coolingTime_Auto);//计算FPGA当前最大时间与上一时刻的时间差
        else
            time1_elapseFPGA = 0;

        if(unusedData2.size()>0)
            time2_elapseFPGA = unusedData2.back().time/NANOSECONDS - (countCoin + coolingTime_Auto);//计算FPGA当前最大时间与上一时刻的时间差
        else
            time2_elapseFPGA = 0;

        //当只计算能谱时，则不调用回调函数，外部通过GetAccumulateSpectrum获取累积能谱？？？
        if (m_pfunc)
            m_pfunc(recentlySpectrum, coinResult);
        if (m_pfuncEx)
            m_pfuncEx(recentlySpectrum, coinResult, m_pfuncUser);
    }
}

/* calculateAllSpectrum:统计给出两个探测器各自当前一秒钟测量数据的能谱，当前一秒钟没有测量信号，则能谱全为零
 * data1：探测器1测量数据
 * data2：探测器2测量数据
 */
void CoincidenceAnalyzer::calculateAllSpectrum(vector<TimeEnergy> data1, vector<TimeEnergy> data2)
{
    SingleSpectrum spec_temp;
    spec_temp.time = countCoin;

    int length1 = AllPoint.back().dataPoint1;
    int length2 = AllPoint.back().dataPoint2;
    int binWidth = MAX_ENERGY/MULTI_CHANNEL;
    if(length1>0)
    {
        vector<int> spectrum1(MULTI_CHANNEL, 0);
        for(int i=0; i < length1; i++)
        {
            int binIndex = data1[i].energy/binWidth;
            if(binIndex >=0 && binIndex < MULTI_CHANNEL) {
                spectrum1[binIndex]++;
            }
        }

        for(int i = 0; i<MULTI_CHANNEL; i++)
        {
            spec_temp.spectrum[0][i] = spectrum1[i];
            AccumulateSpectrum.spectrum[0][i] += spectrum1[i];
        }
    }
    if(length2>0)
    {
        vector<int> spectrum2(MULTI_CHANNEL, 0);
        for(int i=0; i < length2; i++)
        {
            int binIndex = data2[i].energy/binWidth;
            if(binIndex >=0 && binIndex < MULTI_CHANNEL) {
                spectrum2[binIndex]++;
            }
        }

        for(int i = 0; i<MULTI_CHANNEL; i++)
        {
            spec_temp.spectrum[1][i] = spectrum2[i];
            AccumulateSpectrum.spectrum[1][i] += spectrum2[i];
        }
    }
    
    //由于绘图出现最后3道计数较大，因此最后3道强制置零
    for (int i=0; i<3; ++i){
        AccumulateSpectrum.spectrum[0][MULTI_CHANNEL-i-1] = 0;
        AccumulateSpectrum.spectrum[1][MULTI_CHANNEL-i-1] = 0;
    }

    AccumulateSpectrum.time = countCoin;
    if(AllSpectrum.size() >= MAX_REMAIN_SPECTRUM){
        // 删除头部元素
        // 交换法，适合大规模数据
        vector<SingleSpectrum>(AllSpectrum.begin() + 1, AllSpectrum.end()).swap(AllSpectrum);
    }
    AllSpectrum.push_back(spec_temp);
}

// 根据输入能量数据，绘制出能谱，
// data:能量点数组
// maxEnergy: 多道中最大道址对应的能量值。
// ch: 多道道数
vector<int> CoincidenceAnalyzer::GetSingleSpectrum(const vector<int>& data, int maxEnergy, unsigned short ch)
{
    // 自动算出多道计数器的能量bin
    vector<int> binEdges;
    int binWidth = maxEnergy/ch;
    for(int i=0; i< ch+1; i++){
        binEdges.push_back(binWidth*i);
    }
    // 计算能谱
    vector<int> spectrum = computeHistogram(data, binEdges);
    return spectrum;
}

/*Coincidence 对两个探测器的[时刻、能量]数据进行符合处理，对能量在指定范围内的数据进行符合，
 * 然后当两个事件的时间差小于符合分辨时间，则认为是符合事件.
 * 给出符合事件的计数率(cps)随时间的变化，每一秒钟一个符合数据点。
 * data1：探测器1测量数据
 * data2：探测器2测量数据
 * E_left:能窗左边界，单位无
 * E_right：能窗右边界，单位无
 * windowWidthT：符合时间窗宽度,单位ns
 * return：
 * 单符合计数曲线：
 * 多符合事件曲线：
*/
void CoincidenceAnalyzer::Coincidence(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
        unsigned short EnWindowWidth[4],
        int windowWidthT, int delayTime)
{
    CoincidenceResult tmpCoinResult;
    tmpCoinResult.time = AllPoint.back().time;
    int length1 = AllPoint.back().dataPoint1;
    int length2 = AllPoint.back().dataPoint2;
    
    //对FPGA丢包的时间占比进行计算。注意是以探测器1通道的丢包率来作为统一修正。
    double correctRatio = 1.0;
    int FPGA_time = 0;
    FPGA_time = tmpCoinResult.time - coolingTime_Manual; //只有手动测量才需要扣除，对于自动测量，coolingTime_Manual=0；
    {
        QMutexLocker locker(&mutexlossDATA);
        auto it = lossData_time_num.find(FPGA_time);
        if (it != lossData_time_num.end()) {
            double value = it->second * 1.0 /1e9;  // 获取值
            if(value<1.0)  correctRatio = 1.0 / (1.0 - value); //确保异常情况导致
            lossData_time_num.erase(it);
        }
    }
    
    // 取出待处理数据，按照顺序排列
    vector<TimeEnergy> data_temp1,data_temp2;
    data_temp1.reserve(length1);
    data_temp2.reserve(length2);

    //统计探测器1在能窗内的计数以及死时间
    int count1 = 0;
    int deathTime1 = 0;
    for (int i=0; i<length1; i++)
    {
        if(data1[i].energy>EnWindowWidth[0] && data1[i].energy <=EnWindowWidth[1]) {
            count1++;
            data1[i].time += delayTime; //增加对电路延迟时间的处理
            data_temp1.push_back(data1[i]);
        }
        deathTime1 += data1[i].dietime;
    }
    
    tmpCoinResult.CountRate1 = count1;
    tmpCoinResult.DeathRatio1 = deathTime1*100.0/1E9; // 给出1s内的死时间率。转化为100%的单位

    //统计探测器2在能窗内的计数以及死时间
    int count2 = 0;
    int deathTime2 = 0;
    for (int i=0; i<length2; i++)
    {
        if(data2[i].energy>EnWindowWidth[2] && data2[i].energy <=EnWindowWidth[3]) {
            count2++;
            data_temp2.push_back(data2[i]);
        }
        deathTime2 += data2[i].dietime;
    }
    tmpCoinResult.CountRate2 = count2;
    tmpCoinResult.DeathRatio2 = deathTime2*100.0/1E9; // 给出1s内的死时间率。转化为100%的单位


    //这里是适用于什么情况，没看懂 ???
    // 好像是当某一秒其中一个探测器没有数据，另一个探测器有数据。
    if(length1>0 || length2>0){
        if(length1==0 || length2==0)
            if (coinResult.size() > 0)
                coinResult.back().ConCount_single = 0;
    }

    //当任何一个探测器没有信号，则说明没有符合事件
    if(length1==0 || length2==0)  {
        coinResult.push_back(tmpCoinResult);
        return ;
    }

    //优化2: 使用批量构建优先队列
    vector<particle_data> tempVec;
    tempVec.reserve(data_temp1.size() + data_temp2.size());
    for(auto& data : data_temp1) {
        tempVec.emplace_back(data.time, data.energy, 0b01);
    }
    for(auto& data : data_temp2) {
        tempVec.emplace_back(data.time, data.energy, 0b10);
    }

    priority_queue<particle_data> orderTimeEn(std::less<particle_data>(), std::move(tempVec));

    // 使用临时队列遍历
    priority_queue<particle_data> temp(orderTimeEn); // 复制原队列到临时队列
    long long startTime = 0; //符合事件的起始时刻
    unsigned char type = 0b00; //用来判断是否存在不同探头的符合事件
    int count = 0; //符合事件中的信号个数

    while (!temp.empty()) {
       particle_data temp_data = temp.top(); // 返回堆顶元素的引用
       temp.pop(); // 弹出元素以遍历下一个元素

       // 判断时间窗内有没有新的事件产生
       if (temp_data.time - startTime > windowWidthT)
       {
          //先处理上一事件
          if(type==0b11){
            if(count == 2) tmpCoinResult.ConCount_single++; //单符合事件
            if(count > 2) tmpCoinResult.ConCount_multiple++; //多符合事件，单独记录
          }

          //更新起始时间、探测器类型、和符合事件中的信号的个数。
          startTime = temp_data.time;
          type = temp_data.detectorID;
          count = 1;
       }
       else
       {
          startTime = temp_data.time; //扩展型事件，起始时间发生扩展。
          type |= temp_data.detectorID;
          count++;
        }

       //处理最后一次事件
       if(temp.empty()){
            if(type==0b11){
                if(count == 2) tmpCoinResult.ConCount_single++; //单符合事件
                if(count > 2) tmpCoinResult.ConCount_multiple++; //多符合事件，单独记录
            }
       }
    }
    
    //对FPGA丢包进行修正
    if(correctRatio > 1.0) 
    {
        tmpCoinResult.CountRate1 = static_cast<int>(tmpCoinResult.CountRate1 * correctRatio);
        tmpCoinResult.CountRate2 = static_cast<int>(tmpCoinResult.CountRate2 * correctRatio);
        tmpCoinResult.ConCount_single = static_cast<int>(tmpCoinResult.ConCount_single * correctRatio);
        tmpCoinResult.ConCount_multiple = static_cast<int>(tmpCoinResult.ConCount_multiple * correctRatio);
    }
    coinResult.push_back(tmpCoinResult);
}

// 自动更新能窗
void CoincidenceAnalyzer::AutoEnergyWidth()
{
    //计算自动拟合的数据点数
    int sumEnergy1 = 0; 
    int sumEnergy2 = 0;

    for(unsigned short i=0; i<MULTI_CHANNEL; i++)
    {        
        //记录全谱的计数
        sumEnergy1 += GaussFitSpec.spectrum[0][i];
        sumEnergy2 += GaussFitSpec.spectrum[1][i];
    }

    bool changed = false;

    //探测器1高斯拟合
    if(sumEnergy1 > GaussCountMin && sumEnergy2 > GaussCountMin)  
    {
        vector<double> sx,sy;
        for(unsigned short i=0; i<MULTI_CHANNEL; i++)
        {
            if(i >= EnergyWindow[0] && i <= EnergyWindow[1]) {sx.push_back(i*1.0); sy.push_back(GaussFitSpec.spectrum[0][i]);}
        }
        
        int fcount = EnergyWindow[1] - EnergyWindow[0] + 1;
        if(fcount>5){
            double result[3];
            bool status = GaussFit(sx, sy, fcount, result);
            if(status)
            {
                double mean = result[1];
                double FWHM = 2*sqrt(2*log(2))*result[2];
                changed = true;
                double Left = mean - FWHM*0.5; // 峰位-0.5*半高宽FWHM
                double Right = mean + FWHM*0.5; // 峰位+0.5*半高宽FWHM
                if(Left >= 1) EnergyWindow[0] = (unsigned short) Left;
                else EnergyWindow[0] = 1u;

                if(Right < MULTI_CHANNEL - 1u) EnergyWindow[1] = (unsigned short)Right;
                else EnergyWindow[1] = MULTI_CHANNEL-1u;
            }
            else
            {
                // qDebug().noquote() <<"探测器1自动高斯拟合发生异常,可能原因，选取的初始峰位不具有高斯形状，无法进行高斯拟合";
            }
        }
        else{
            // qDebug().noquote() <<"探测器1自动高斯拟合发生异常,待拟合的数据点数小于6个，无法拟合";
        }
    }

    //探测器2高斯拟合
    if(sumEnergy1 > GaussCountMin && sumEnergy2 > GaussCountMin)
    {
        vector<double> sx,sy;
        for(unsigned short i=0; i<MULTI_CHANNEL; i++)
        {
            if(i >= EnergyWindow[2] && i <= EnergyWindow[3]) {sx.push_back(i*1.0); sy.push_back(GaussFitSpec.spectrum[1][i]);}
        }
        
        int fcount = EnergyWindow[3] - EnergyWindow[2] + 1;
        double result[3];
        bool status = GaussFit(sx, sy, fcount, result);
        if(status)
        {
            double mean = result[1];
            double FWHM = 2*sqrt(2*log(2))*result[2];
            changed = true;
            double Left = mean - FWHM*0.5; // 峰位-0.5*半高宽FWHM
            double Right = mean + FWHM*0.5; // 峰位+0.5*半高宽FWHM

            if(Left >= 1) EnergyWindow[2] = (unsigned short) Left;
            else EnergyWindow[2] = 1u;

            if(Right < MULTI_CHANNEL - 1u) EnergyWindow[3] = (unsigned short)Right;
            else EnergyWindow[3] = MULTI_CHANNEL - 1u;
        }
        else{
            // qDebug().noquote()<<"探测器2 自动高斯拟合发生异常,可能原因，选取的初始峰位不具有高斯形状，无法进行高斯拟合";
        }
    }
    if(changed) GaussFitLog.push_back({countCoin, EnergyWindow[0], EnergyWindow[1], EnergyWindow[2], EnergyWindow[3]});
    isChangeEnWindow = changed;
}

//统计给出当前一秒内的两个探测器各自数据点的个数
bool CoincidenceAnalyzer::GetDataPoint(vector<TimeEnergy> data1, vector<TimeEnergy> data2)
{
    countCoin++;
    CurrentPoint onePoint;

    long long current_nanosconds = (long long)(countCoin + coolingTime_Auto)  * NANOSECONDS;
    onePoint.time = countCoin + coolingTime_Auto + coolingTime_Manual; //对于自动测量时，coolingTime_Manual为0。对于手动测量时，coolingTime_Auto为零
    int ilocation1_below = 0;
    int ilocation2_below = 0;
    int ilocation1_above = 0; 
    int ilocation2_above = 0;
    bool find1_below = find_index_below(data1, current_nanosconds, ilocation1_below);
    bool find2_below = find_index_below(data2, current_nanosconds, ilocation2_below);
    bool find1_above = find_index_above(data1, current_nanosconds, ilocation1_above);
    bool find2_above = find_index_above(data2, current_nanosconds, ilocation2_above);

    if(find1_above)
        onePoint.dataPoint1 = ilocation1_above; //给出本次处理的探测器1数据点数,不论当前这一秒有没有数据，在这一秒之后有数据。
    else
    {
        if(find1_below)
            onePoint.dataPoint1 = ilocation1_below+1; //当前这一秒有数据，但是在这一秒后没数据，所以只有below,没有above。
        else
            onePoint.dataPoint1 = 0;
    }

    if(find2_above)
        onePoint.dataPoint2 = ilocation2_above; //给出本次处理的探测器1数据点数,不论当前这一秒有没有数据，在这一秒之后有数据。
    else
    {
        if(find2_below)
            onePoint.dataPoint2 = ilocation2_below; //当前这一秒有数据，但是在这一秒后没数据，所以只有below,没有above。
        else
            onePoint.dataPoint2 = 0;
    }
    AllPoint.push_back(onePoint);
    return true;
}

// 计算直方图
vector<int> CoincidenceAnalyzer::computeHistogram(const vector<int>& data, const vector<int>& binEdges)
{
    vector<int> histogram(binEdges.size() - 1, 0); // 初始化直方图，大小为 binEdges.size() - 1

    // 遍历数据，统计每个 bin 的频数
    for (int value : data) {
        // 找到 value 所属的 bin
        auto it = std::lower_bound(binEdges.begin(), binEdges.end(), value);
        int binIndex = it - binEdges.begin() - 1;

        // 确保 value 在 binEdges 的范围内
        if (binIndex >= 0 && binIndex < (int)histogram.size())
        {
            histogram[binIndex]++;
        }
    }

    return histogram;
}

//找到第一个大于value的下标。返回是否查找到的标记
bool CoincidenceAnalyzer::find_index_above(vector<TimeEnergy> data, unsigned long long value, int& index)
{
    if(data.size() ==0 ) return false;

    // 使用 std::find_if 和 lambda 表达式来查找第一个大于 value 的元素
    // 自定义比较函数
    auto comp = [](unsigned long long val, const TimeEnergy& te) {
        return te.time >=val;
    };
    
    // 使用 upper_bound 进行二分查找
    auto it = std::upper_bound(data.begin(), data.end(), value, comp);
    
    // 检查是否找到了元素
    if (it != data.end()) {
        index = distance(data.begin(), it);
        return true;
    } else {
        // 没有找到
        return false;
    }
}

//找到最后一个小于value的下标。返回是否查找到的标记
bool CoincidenceAnalyzer::find_index_below(vector<TimeEnergy> data, unsigned long long value, int& index)
{
    if(data.size() ==0 ) return false;

    auto it = std::upper_bound(data.begin(), data.end(), value,
    [](unsigned long long val, const TimeEnergy& te) {
        return val <= te.time;  // 注意这里的比较顺序和条件
    });
    
    if (it != data.begin()) {
        // 最后一个小于value的元素是前一个位置
        index = distance(data.begin(), it);
        index--;
        return true;
        // 使用 last_less->time 或 last_less->energy
    } else {
        // 所有元素都 >= value，没有小于value的元素
        return false;
    }
}

double CoincidenceAnalyzer::getInintialActive(DetectorParameter detPara, int time_SetEnWindow)
{
    //如果没有前面的calculate的计算，那么后续计算不能进行
    if(coinResult.size() == 0) return 0.0;
   
    //测量的起点时刻（这个时刻以活化物活化后开始计时）
    int start_time = 0;
    if(detPara.measureModel == mmManual) start_time = detPara.coolingTime + time_SetEnWindow; //对于手动拟合，选取能窗前的一段数据要舍弃
    if(detPara.measureModel == mmAuto) start_time = detPara.coolingTime;
    
    //测量时间终点时刻（相对于活化0时刻）。
    int time_end = 0;
    if(detPara.measureModel == mmManual) time_end = detPara.coolingTime + coinResult.back().time;
    if(detPara.measureModel == mmAuto) time_end = coinResult.back().time;

    if(time_end < start_time) return 0.0; //不允许起始时间小于停止时间
    double A0_omiga = getInintialActive(detPara, start_time, time_end);

    return A0_omiga;
}

/**
 * @description: 用于离线处理。根据计数曲线，选取合适的时间区间，对这个区间的数据进行积分处理，给出其初始的相对活度
 * 注意：这里的时间以铜活化结束开始计时。
 * @param {DetectorParameter} detPara 测量参数
 * @param {int} startT 起始时间，单位s
 * @param {int} endT 结束时间，单位s
 * @return {*} 相对活度 铜片β+衰变强度乘以近端探测器立体角效率
 */
double CoincidenceAnalyzer::getInintialActive(DetectorParameter detPara, int start_time, int time_end)
{
    // vector<CoincidenceResult> coinResult = GetCoinResult();
    //如果没有前面的calculate的计算，那么后续计算不能进行
    if(coinResult.size() == 0) return 0.0;
    
    size_t num = coinResult.size(); //计数点个数。

    //符合时间窗,单位ns
    int timeWidth_tmp = detPara.timeWidth;
    timeWidth_tmp = timeWidth_tmp/1e9;

    if(time_end < start_time) return 0.0; //不允许起始时间小于停止时间

    int N1 = 0;
    int N2 = 0;
    int Nc = 0;
    double deathTime_ratio_total[2] = {0.0, 0.0};
    double deathTime_ratio_ave[2] = {0.0, 0.0};
    for(auto coin:coinResult)
    {
        // 手动测量，在改变能窗之前的数据不处理，注意剔除。现在数据没有保存这一段数据
        // if(coin.time <= time_SetEnWindow) continue;

        N1 += coin.CountRate1;
        N2 += coin.CountRate2;
        Nc = Nc + coin.ConCount_single + coin.ConCount_multiple;
        deathTime_ratio_total[0] += coin.DeathRatio1 * 0.01; //注意将百分比但我转化为小数
        deathTime_ratio_total[1] += coin.DeathRatio2 * 0.01;
    }
    
    deathTime_ratio_ave[0] = deathTime_ratio_total[0] / num;
    deathTime_ratio_ave[1] = deathTime_ratio_total[1] / num;
    
    //f因子。暂且称为积分因子
    //这里不考虑Cu62的衰变分支，也就是测量必须时采用冷却时长远大于Cu62半衰期(9.67min = 580s)的数据。
    // double T_halflife = 9.67*60; //单位s，Cu62的半衰期
    double T_halflife = 12.7*60*60; // 单位s,Cu64的半衰期
    double lamda = log(2) / T_halflife;
    double f = 1/lamda*(exp(-lamda*start_time) - exp(-lamda*time_end));

    //对符合计数进行真偶符合修正
    //注意timeWidth_tmp单位为ns，要换为时间s。
    double measureTime = (time_end - start_time)*1.0;
    double Nco = (Nc*measureTime - 2*timeWidth_tmp*N1*N2)/(measureTime - timeWidth_tmp*(N1+N2));

    //死时间修正
    double N10 = N1 / (1 - deathTime_ratio_ave[0]);
    double N20 = N2 / (1 - deathTime_ratio_ave[1]);
    double Nco0 = Nco / (1 - deathTime_ratio_ave[0]) / (1 - deathTime_ratio_ave[1]);
    //计算出活度乘以探测器几何效率的值。本项目中称之为相对活度
    //反推出0时刻的计数。

    double A0_omiga = N10 * N20 / Nco0 / f;

    return A0_omiga;
}


void CoincidenceAnalyzer::doFPGA_lossDATA_correction(std::map<unsigned int, unsigned long long> lossData)
{
    int coinNum = coinResult.size();
    for (const auto& pair:lossData) {
        unsigned int time_seconds = pair.first; 
        
        //从活化后开始计时，注意，手动测量模式下，FPGA计时之前存在一段非零计数。自动测量下coolingTime_Manual=0
        unsigned int absolutetime_seconds = time_seconds + coolingTime_Manual;

        double value = pair.second * 1.0 /1e9;  // 获取值
        if(value<1.0) {
            double correctRatio = 1.0 / (1.0 - value); //确保异常情况导致
            unsigned int pos = absolutetime_seconds - 1; //注意下标索引的时候减一
            if(pos < coinNum){
                coinResult[pos].CountRate1 =  static_cast<int>(coinResult[pos].CountRate1 * correctRatio);
                coinResult[pos].CountRate2 =  static_cast<int>(coinResult[pos].CountRate2 * correctRatio);
                coinResult[pos].ConCount_single =  static_cast<int>(coinResult[pos].ConCount_single * correctRatio);
                coinResult[pos].ConCount_multiple =  static_cast<int>(coinResult[pos].ConCount_multiple * correctRatio);
            }
        }
    }
}
