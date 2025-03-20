#include "coincidenceanalyzer.h"
#include <queue>
using namespace std;

#include<queue>
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
countCoin(0)
{

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
CoinResult CoincidenceAnalyzer::Coincidence(vector<TimeEnergy> data1, vector<TimeEnergy> data2,
                        unsigned short E_left, unsigned short E_right, int windowWidthT)
{
    countCoin++;
    CoinResult result_temp;
//    long long current_nanosconds = (long long)countCoin * 1000000000LL;
    long long current_nanosconds = (long long)countCoin * 1000LL;

    int ilocation1_below, ilocation2_below;
    int ilocation1_above, ilocation2_above;
    bool find1_below = find_index_below(data1, current_nanosconds, ilocation1_below);
    bool find2_below = find_index_below(data2, current_nanosconds, ilocation2_below);
    bool find1_above = find_index_above(data1, current_nanosconds, ilocation1_above);
    bool find2_above = find_index_above(data2, current_nanosconds, ilocation2_above);

    if(find1_above || find2_above){//FPGA时钟都已经超出当前一秒，故可以取完整一秒内的数据进行处理
        if(find1_below || find2_below)
        {
            //说明当前一秒内只有一个探测器有数据，不存在符合事件。
            if(!(find1_below && find1_below)) {
                result_temp.dataPoint1 = ilocation1_above; //给出本次处理的探测器1数据点数
                result_temp.dataPoint2 = ilocation2_above;
                result_temp.ConCount_single = 0; //没有符合计数
                result_temp.ConCount_multiple = 0;

                result.push_back(result_temp);
                return result_temp;
            }
            //说明当前一秒内都有数据，进行可以进行后续数据处理。
        }
        else{
            result_temp.dataPoint1 = 0; //给出本次处理的探测器1数据点数
            result_temp.dataPoint2 = 0;
            result_temp.ConCount_single = 0; //没有符合计数
            result_temp.ConCount_multiple = 0;

            result.push_back(result_temp);
            return result_temp;
        }
    }
    else{//探测器1或者探测器2存在某个没有装满一秒钟的数据，所以这一次不处理数据，计时器减一
        countCoin--;
        result_temp.dataPoint1 = 0; //告知没有处理数据
        result_temp.dataPoint2 = 0; //告知没有处理数据
        return result_temp;
    }


    // 取出待处理数据，按照顺序排列
    vector<TimeEnergy> data_temp1,data_temp2;
    copy(data1.begin(), data1.begin()+ilocation1_above, back_inserter(data_temp1));//部分拷贝
    copy(data2.begin(), data2.begin()+ilocation2_above, back_inserter(data_temp2));
    result_temp.dataPoint1 = ilocation1_above;
    result_temp.dataPoint2 = ilocation2_above;

    //
    priority_queue<particle_data> q;
    for(auto data:data_temp1)
    {
        //判断粒子能量是否在范围内
        if(data.energy > E_left && data.energy < E_right){
            q.push(particle_data(data.time, data.energy, 0b01));
        }
    }
    for(auto data:data_temp2)
    {
        //判断粒子能量是否在范围内
        if(data.energy > E_left && data.energy < E_right){
            q.push(particle_data(data.time, data.energy, 0b10));
        }
    }

    // 使用临时队列遍历
    priority_queue<particle_data> temp(q); // 复制原队列到临时队列
    int startTime = 0; //符合事件的起始时刻
    unsigned char type = 0b00; //用来判断是否存在不同探头的符合事件
    int count = 0; //符合事件中的信号个数

    while (!temp.empty()) {
       particle_data temp_data = temp.top(); // 返回堆顶元素的引用
       temp.pop(); // 弹出元素以遍历下一个元素

       // 判断时间窗内有没有新的事件产生
       if(temp_data.time - startTime > windowWidthT)
       {
          //先处理上一事件
          if(type==0b11){
            if(count == 2) result_temp.ConCount_single++; //单符合事件
            if(count > 2) result_temp.ConCount_multiple++; //多符合事件，单独记录
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
    }

    result.push_back(result_temp);
    return result_temp;
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

//找到第一个小于value的下标。返回是否查找到的标记
bool CoincidenceAnalyzer::find_index_below(vector<TimeEnergy> data, long long value, int& index)
{
    // 使用 std::find_if 和 lambda 表达式来查找第一个小于 value 的元素
    auto it = find_if(data.begin(), data.end(), [value](TimeEnergy x) { return x.time < value; });

    // 检查是否找到了元素
    if (it != data.end()) {
        // 找到了，输出该元素的下标
//        cout << "第一个小于 " << value << " 的元素下标是: " << distance(data.begin(), it) << std::endl;
        index = distance(data.begin(), it);
        return true;
    } else {
        // 没有找到
//        cout << "没有找到小于 " << value << " 的元素。" << endl;
        return false;
    }
}
