#include "coincidenceanalyzer.h"
#include <queue>
#include "linearfit.h"

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
countCoin(0),autoFirst(true),
GaussCountMin(100000),
#ifdef Q_NO_DEBUG
GaussMinGapTime(180)
#else
GaussMinGapTime(180)
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

void CoincidenceAnalyzer::calculate(vector<TimeEnergy> _data1, vector<TimeEnergy> _data2,
              unsigned short E_win[4], int windowWidthT,
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


    vector<TimeEnergy> data1;
    vector<TimeEnergy> data2;
    if (unusedData1.size() > 0){
        data1.insert(data1.end(), unusedData1.begin(), unusedData1.end());
        unusedData1.clear();
    }
    data1.insert(data1.end(), _data1.begin(), _data1.end());
    if (unusedData2.size() > 0){
        data2.insert(data2.end(), unusedData2.begin(), unusedData2.end());
        unusedData2.clear();
    }
    data2.insert(data2.end(), _data2.begin(), _data2.end());

    if (data1.size()<=0 || data2.size() <= 0){
        return;
    }

    // 准备计算
    int time1_elapseFPGA;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
    int time2_elapseFPGA;//计算FPGA当前最大时间与上一时刻的时间差,单位：秒
    time1_elapseFPGA = data1.back().time/NANOSECONDS - countCoin;//计算FPGA当前最大时间与上一时刻的时间差
    time2_elapseFPGA = data2.back().time/NANOSECONDS - countCoin;//计算FPGA当前最大时间与上一时刻的时间差

    int deltaT = 1; //单位秒
    //必须存够1秒的数据才进行处理
    while(time1_elapseFPGA >= deltaT && time2_elapseFPGA >= deltaT)
    {
        //先计算出当前一秒的数据点个数
        GetDataPoint(data1, data2);

        //对当前一秒数据处理给出能谱
        calculateAllSpectrum(data1,data2);
        
        //计算计数曲线
        if (countFlag)
        {   
            //自动拟合，更新能窗
            if(autoEnWidth) 
            {
                //距离上次自动高斯拟合的时间间隔，单位：s
                int GapTime = countCoin;
                if (GaussFitLog.size() > 0)
                    GapTime -= GaussFitLog.back().time;
                if(GapTime>GaussMinGapTime)  AutoEnergyWidth();
            }

            //对当前一秒数据处理给出各自的计数以及符合计数
            Coincidence(data1, data2, EnergyWindow, windowWidthT);
        }

        //删除容器中已经处理的数据点
        if (AllPoint.back().dataPoint1 <= (int)data1.size()) {
            data1.erase(data1.begin(), data1.begin() + AllPoint.back().dataPoint1);
        } else {
            data1.clear(); // 如果N大于容器的大小，清空容器
        }
        if (AllPoint.back().dataPoint2 <= (int)data2.size()) {
            data2.erase(data2.begin(), data2.begin() + AllPoint.back().dataPoint2);
        } else {
            data2.clear(); // 如果N大于容器的大小，清空容器
        }

        long long lastTime1 = data1.back().time;
        long long lastTime2 = data2.back().time;
        time1_elapseFPGA = lastTime1/NANOSECONDS - countCoin;//计算FPGA当前最大时间与上一时刻的时间差
        time2_elapseFPGA = lastTime2/NANOSECONDS - countCoin;//计算FPGA当前最大时间与上一时刻的时间差

        //当只计算能谱时，则不调用回调函数，外部通过GetAccumulateSpectrum获取累积能谱
        if (countFlag && m_pfunc) m_pfunc(AccumulateSpectrum, coinResult);
    }

    //将容器中未经处理的数据点添加到缓存保存起来，下次优先处理
    if (data1.size() > 0)
        unusedData1.insert(unusedData1.end(), data1.begin(), data1.end());
    if (data2.size() > 0)
        unusedData2.insert(unusedData2.end(), data2.begin(), data2.end());
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
    if(length1>0)
    {
        vector<int> dataE;
        for(int i=0; i < length1; i++)
        {
            dataE.push_back(data1.at(i).energy);
        }
        vector<int> spectrum1  = GetSingleSpectrum(dataE, MAX_ENERGY, MULTI_CHANNEL);

        for(int i = 0; i<MULTI_CHANNEL; i++)
        {
            spec_temp.spectrum[0][i] = spectrum1[i];
            AccumulateSpectrum.spectrum[0][i] += spectrum1[i];
        }
    }
    if(length2>0)
    {
        vector<int> dataE;
        for(int i=0; i < length2; i++)
        {
            dataE.push_back(data2.at(i).energy);
        }
        vector<int> spectrum2 = GetSingleSpectrum(dataE, MAX_ENERGY, MULTI_CHANNEL);

        for(int i = 0; i<MULTI_CHANNEL; i++)
        {
            spec_temp.spectrum[1][i] = spectrum2[i];
            AccumulateSpectrum.spectrum[1][i] += spectrum2[i];
        }
    }
    
    //由于绘图出现最后一道计数较大，因此最后一道强制置零
    AccumulateSpectrum.spectrum[0][MULTI_CHANNEL-1] = 0;
    AccumulateSpectrum.spectrum[1][MULTI_CHANNEL-1] = 0;

    AccumulateSpectrum.time = countCoin;
    if(AllSpectrum.size() >= MAX_REMAIN_SPECTRUM) AllSpectrum.erase(AllSpectrum.begin()); //当超过最大能谱数目时，则先进先出
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
        int windowWidthT)
{
    CoincidenceResult tmpCoinResult;
    int length1 = AllPoint.back().dataPoint1;
    int length2 = AllPoint.back().dataPoint2;
    //统计探测器1在能窗内的计数
    int count1 = 0;
    for (int i=0; i<length1; i++)
    {
        if(data1.at(i).energy>EnWindowWidth[0] && data1.at(i).energy <=EnWindowWidth[1]) count1++;
    }
    tmpCoinResult.CountRate1 = count1;

    //统计探测器2在能窗内的计数
    int count2 = 0;
    for (int i=0; i<length2; i++)
    {
        if(data2.at(i).energy>EnWindowWidth[2] && data2.at(i).energy <=EnWindowWidth[3]) count2++;
    }
    tmpCoinResult.CountRate2 = count2;

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

    // 取出待处理数据，按照顺序排列
    vector<TimeEnergy> data_temp1,data_temp2;
    copy(data1.begin(), data1.begin()+length1, back_inserter(data_temp1));//部分拷贝
    copy(data2.begin(), data2.begin()+length2, back_inserter(data_temp2));

    //
    priority_queue<particle_data> q;
    for(auto data:data_temp1)
    {
        //判断粒子能量是否在范围内
        if(data.energy > EnWindowWidth[0] && data.energy < EnWindowWidth[1]){
            q.push(particle_data(data.time, data.energy, 0b01));
        }
    }
    for(auto data:data_temp2)
    {
        //判断粒子能量是否在范围内
        if(data.energy > EnWindowWidth[2] && data.energy < EnWindowWidth[3]){
            q.push(particle_data(data.time, data.energy, 0b10));
        }
    }

    // 使用临时队列遍历
    priority_queue<particle_data> temp(q); // 复制原队列到临时队列
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
    coinResult.push_back(tmpCoinResult);
}

// 自动更新能窗
void CoincidenceAnalyzer::AutoEnergyWidth()
{
    SingleSpectrum tempSpec = AllSpectrum.back();

    //计算自动拟合的数据点数
    int sumEnergy1 = 0; 
    int sumEnergy2 = 0;

    for(unsigned short i=0; i<MULTI_CHANNEL; i++)
    {
        GaussFitSpec.time = tempSpec.time;
        GaussFitSpec.spectrum[0][i] += tempSpec.spectrum[0][i];
        GaussFitSpec.spectrum[1][i] += tempSpec.spectrum[1][i];
        
        //记录全谱的计数
        sumEnergy1 += GaussFitSpec.spectrum[0][i];
        sumEnergy2 += GaussFitSpec.spectrum[1][i];
    }

    bool changed = false;

    if(sumEnergy1 > GaussCountMin && sumEnergy2 > GaussCountMin)  
    {
        double result[3];
        vector<double> sx,sy;
        for(unsigned short i=0; i<MULTI_CHANNEL; i++)
        {
            if(i >= EnergyWindow[0] && i <= EnergyWindow[1]) {sx.push_back(i*1.0); sy.push_back(GaussFitSpec.spectrum[0][i]);}
            //重置
            GaussFitSpec.spectrum[0][i] = 0;
        }
        
        int fcount = EnergyWindow[1] - EnergyWindow[0] + 1;
        Gaussian gau;
        if(gau.setSample(sx, sy, fcount, result) && gau.process())
        {
            result[0] = gau.getResult(0);
            result[1] = gau.getResult(1);
            result[2] = gau.getResult(2);

            changed = true;
            double Left = result[1] - result[0]*0.5; // 峰位-0.5*半高宽FWHM
            double Right = (result[1] + result[0]*0.5); // 峰位+0.5*半高宽FWHM
            if(Left >= 1) EnergyWindow[0] = (unsigned short) Left;
            else EnergyWindow[0] = 1u;

            if(Right < MULTI_CHANNEL - 1u) EnergyWindow[1] = (unsigned short)Right;
            else EnergyWindow[1] = MULTI_CHANNEL-1u;
        }
    }

    if(sumEnergy1 > GaussCountMin && sumEnergy2 > GaussCountMin)  
    {
        double result[3];
        vector<double> sx,sy;
        for(unsigned short i=0; i<MULTI_CHANNEL; i++)
        {
            if(i >= EnergyWindow[2] && i <= EnergyWindow[3]) {sx.push_back(i*1.0); sy.push_back(GaussFitSpec.spectrum[1][i]);}
            //重置
            GaussFitSpec.time = 0;//注意这里是在探测器2之后才重置
            GaussFitSpec.spectrum[1][i] = 0;
        }

        int fcount = EnergyWindow[3] - EnergyWindow[2] + 1;
        Gaussian gau;
        if(gau.setSample(sx, sy, fcount, result) && gau.process())
        {
            result[0] = gau.getResult(0);
            result[1] = gau.getResult(1);
            result[2] = gau.getResult(2);

            changed = true;
            double Left = result[1] - result[0]*0.5;
            double Right = (result[1] + result[0]*0.5);
            if(Left >= 1) EnergyWindow[2] = (unsigned short) Left;
            else EnergyWindow[2] = 1u;

            if(Right < MULTI_CHANNEL - 1u) EnergyWindow[3] = (unsigned short)Right;
            else EnergyWindow[3] = MULTI_CHANNEL - 1u;
        }
    }
    if(changed) GaussFitLog.push_back({countCoin, EnergyWindow[0], EnergyWindow[1], EnergyWindow[2], EnergyWindow[3]});
}

//统计给出当前一秒内的两个探测器各自数据点的个数
void CoincidenceAnalyzer::GetDataPoint(vector<TimeEnergy> data1, vector<TimeEnergy> data2)
{
    countCoin++;
    CurrentPoint onePoint;

    long long current_nanosconds = (long long)countCoin * NANOSECONDS;

    int ilocation1_below, ilocation2_below;
    int ilocation1_above, ilocation2_above;
    bool find1_below = find_index_below(data1, current_nanosconds, ilocation1_below);
    bool find2_below = find_index_below(data2, current_nanosconds, ilocation2_below);
    bool find1_above = find_index_above(data1, current_nanosconds, ilocation1_above);
    bool find2_above = find_index_above(data2, current_nanosconds, ilocation2_above);

    if(find1_above && find2_above){//FPGA时钟都已经超出当前一秒，故可以取完整一秒内的数据进行处理
        if(find1_below || find2_below)
        {
            //说明当前一秒内有数据
            onePoint.dataPoint1 = ilocation1_above; //给出本次处理的探测器1数据点数
            onePoint.dataPoint2 = ilocation2_above;
        }
        else{
            onePoint.dataPoint1 = 0; //给出本次处理的探测器1数据点数
            onePoint.dataPoint2 = 0;
        }
    }
    else{//探测器1或者探测器2存在某个没有装满一秒钟的数据，所以这一次不处理数据，计时器减一
        countCoin--;
        onePoint.dataPoint1 = 0; //告知没有处理数据
        onePoint.dataPoint2 = 0; //告知没有处理数据
    }
    AllPoint.push_back(onePoint);
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
bool CoincidenceAnalyzer::find_index_above(vector<TimeEnergy> data,long long value, int& index)
{
    // 使用 std::find_if 和 lambda 表达式来查找第一个大于 value 的元素
    auto it = find_if(data.begin(), data.end(), [value](TimeEnergy x) { return x.time >= value; });

    // 检查是否找到了元素
    if (it != data.end()) {
        // 找到了，输出该元素的下标
//        cout << "第一个大于 " << value << " 的元素下标是: " << distance(data.begin(), it) << std::endl;
        index = distance(data.begin(), it);
        return true;
    } else {
        // 没有找到
//        cout << "没有找到大于 " << value << " 的元素。" << endl;
        return false;
    }
}

//找到最后一个小于value的下标。返回是否查找到的标记
bool CoincidenceAnalyzer::find_index_below(vector<TimeEnergy> data, long long value, int& index)
{
    // 使用 std::find_if 和 lambda 表达式来查找第一个小于 value 的元素
    auto it = find_if(data.rbegin(), data.rend(), [value](TimeEnergy x) { return x.time < value; });

    // 检查是否找到了元素
    if (it != data.rend()) {
        // 找到了，输出该元素的下标
//        cout << "第一个小于 " << value << " 的元素下标是: " << distance(data.begin(), it) << std::endl;
        index = data.rend() - it - 1;//distance(data.rbegin(), it);

        return true;
    } else {
        // 没有找到
//        cout << "没有找到小于 " << value << " 的元素。" << endl;
        return false;
    }

}
