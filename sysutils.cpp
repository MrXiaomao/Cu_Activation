#include "sysutils.h"
#include <iostream>
#include <math.h>

#include <QFile>
#include <QFileInfo>
#include <QDebug>

quint32 SysUtils::SequenceNumber = 0;
quint64 SysUtils::lastTime = 0;
quint64 SysUtils::firstTime = 0;
std::map<unsigned int, unsigned long long> SysUtils::lossData;
// QList<QByteArray> SysUtils::packets;

SysUtils::SysUtils()
{

}

// 计算直方图
vector<unsigned short> computeHistogram(const vector<unsigned short>& data, const vector<int>& binEdges) {
    vector<unsigned short> histogram(binEdges.size() - 1, 0); // 初始化直方图，大小为 binEdges.size() - 1

    // 遍历数据，统计每个 bin 的频数
    for (int value : data) {
        // 找到 value 所属的 bin
//        auto it = std::lower_bound(binEdges.begin(), binEdges.end(), value);
//        int binIndex = it - binEdges.begin() - 1;
        int binIndex = value /(binEdges.at(1)-binEdges.at(0));
        // 确保 value 在 binEdges 的范围内
        if (binIndex >= 0 && binIndex < (int)histogram.size())
        {
            histogram[binIndex]++;
        }
    }

    return histogram;
}

// 根据输入能量数据，绘制出能谱，
// data:能量点数组
// maxEnergy: 多道中最大道址对应的能量值。
// ch: 多道道数
/*vector<unsigned short> GetSpectrum(const vector<unsigned short>& data, unsigned int maxEnergy, int ch)
{
    // 自动算出多道计数器的能量bin。
    vector<int> binEdges;
    int binWidth = maxEnergy/ch;
    for(int i=0; i< ch+1; i++){
        binEdges.push_back(binWidth*i);
    }
    // 计算能谱
    vector<unsigned short> spectrum = computeHistogram(data, binEdges);
    return spectrum;
}*/

// 计算data中数据在指定能量区间的数据个数
// data：输入的能量数据点
// leftE：左区间
// rightE：右区间
// stepT统计的时间步长, 单位s
// return double：计数率,cps
double GetCount(const vector<unsigned short> &data, int leftE, int rightE, int stepT)
{
    int count = 0;
    for (int value : data)
    {
        if(value>leftE && value <=rightE) count++;
    }
    return ((double)count)/stepT;
}

// 对输入的粒子[时间、能量]数据进行统计,给出计数率随时间变化的曲线，注意这里dataT与dataE一定是相同的长度
// dataT粒子的时间, 单位ns
// dataE粒子的能量
// stepT统计的时间步长, 单位s
// leftE：左区间
// rightE：右区间
// return：vector<TimeCountRate> 计数率数组
struct TimeCountRate{
    int time; //单位s
    double CountRates; //单位cps
};
vector<TimeCountRate> GetCountRate(const vector<long long> &dataT, const vector<unsigned short> &dataE, int stepT, int leftE, int rightE)
{
    long long delaT = stepT;
    delaT *= 1E9;//单位转化 s转化ns

    // 初始化计数率数组
    long long TimeMax = dataT.back(); //最大时间
    int CountSize = TimeMax/delaT + 1; //计数率点的个数
    vector<TimeCountRate> countRate; //全部初始化为0
    countRate.resize(CountSize);

    // 初始化
    for (int i=0; i<CountSize; i++)
    {
        countRate.at(i).time = i*stepT;
        countRate.at(i).CountRates = 0.0;
    }

    //统计计数
    for (size_t i=0; i<dataT.size(); i++)
    {
        int indexT = static_cast<int>(dataT.at(i)/delaT); //相除取整
        if(dataE.at(i)>leftE && dataE.at(i)<=rightE)
        countRate.at(indexT).CountRates +=1.0;
    }

    //转化为计数率cps
    for (std::vector<TimeCountRate>::iterator it = countRate.begin(); it != countRate.end(); ++it)
    {
        (*it).CountRates /= stepT;
    }

    return countRate;
}

// 打印直方图
void printHistogram(const vector<int>& binEdges, const vector<int>& histogram) {
    for (size_t i = 0; i < histogram.size(); i++) {
        std::cout << "Bin "<<i<<"[" << binEdges[i] << ", " << binEdges[i + 1] << "): " << histogram[i] << std::endl;
    }
}

// count  待拟合数据长度
// sx 待拟合数据x数组，
// sy 待拟合数据y数组
// 返回值result, result[0]为半高宽FWHM,result[1] 为峰位，result[2]为峰值。
/*#include "polynomialfit.h"
bool fit_GaussCurve(int count, std::vector<double> sx, std::vector<double> sy, double* result)
{
    int degree = 2;
    double* weight = new double[count];
    // 赋初值
    result[0] = 0.;
    result[1] = 0.;
    result[2] = 0.;
    // 参考[1]陈伟,方方,莫磊,等.一种单能高斯峰解析方法[J].太赫兹科学与电子信息学报,2020,18(03):527-530.
    // 核能谱单能峰扣除背景后，其净计数近似服从高斯分布：y(x) = a*exp[-4*ln2*(x-u)^2/FWHM^2]
    // a为峰值，u为峰位道址，FWHM 为半高宽。
    // 将高斯函数变化为多项式，取对数,整理为二次多项式形式，b=c1x^2+c2x+c3;
    // b = lny, c1=-4ln2/(FWHM^2);c2=8ln2*u/FWHM^2; c2=-4ln2*u^2/FWHM^2+lna
    // **需要注意的是，由于这里取对数，需要保证sy的值大于0. 因此当 输入的sy中存在0时，拟合结果会可能是错误，
    // **这里简单的将拟合数据y为0的时候直接设置为1e-20.这对于能谱拟合是可以的，但是对于非能谱领域，需要重新考虑。
    for(int i=0; i<count; i++)
    {
        if(sy[i]<1e-20) sy[i] = 1e-20;
        sy[i] = log(sy[i]); //log()函数返回数字的自然对数。
        weight[i] = 1.0;
    }
    PolynomialFit pf;
    pf.setAttribute(degree , false , 1.0);
    if( pf.setSample(sx,sy,count,false, weight) &&
            pf.process() )
    {
        //pf.print();
        double c1,c2,c3;
        c1 = pf.getResult(0);
        c2 = pf.getResult(1);
        c3 = pf.getResult(2);
        double FWHM = 0.0;
        if(-4*log(2)/c1) FWHM = sqrt(-4*log(2)/c1);
        else std::cout<<"cannot fit"<<std::endl;

        double mean = -c2/2/c1;
        double a = exp(c3+4*log(2)*mean*mean/(FWHM*FWHM));

        std::cout << "FWHM = " << FWHM << std::endl;
        std::cout << "mean = " << mean << std::endl;
        std::cout << "a = " << a << std::endl;
        result[0] = FWHM;
        result[1] = mean;
        result[2] = a;
        delete[] weight;
        return true;
    }else
    {
        std::cout << "failed" ;
        delete[] weight;
        return false;
    }
}*/

std::vector<DetTimeEnergy> SysUtils::getDetTimeEnergy(const char* filename)
{
    std::vector<DetTimeEnergy> result;
    FILE* input_file = fopen(filename, "rb");
    if (ferror(input_file))
        return result;

    uint32_t packetSize = (PARTICLE_NUM_ONE_PAKAGE +1) * 16;
    std::vector<uint8_t> read_buffer(packetSize, 0);
    std::vector<uint8_t> cache_buffer;

    while (!feof(input_file)){
        //std::vector<uint8_t>().swap(read_buffer);
        size_t read_size = fread(reinterpret_cast<char*>(read_buffer.data()), 1, packetSize, input_file);
        cache_buffer.insert(cache_buffer.end(), read_buffer.begin(), read_buffer.begin() + read_size);

        uint8_t *ptrOffset = cache_buffer.data();
        uint32_t buffer_size = cache_buffer.size();
        while (buffer_size >= packetSize){
            //判断头部
            if ((uint8_t)ptrOffset[0] == 0x00 && (uint8_t)ptrOffset[1] == 0x00 && (uint8_t)ptrOffset[2] == 0xaa && (uint8_t)ptrOffset[3] == 0xb3){
                ptrOffset = read_buffer.data() + packetSize - 4;
                //判断尾部
                if ((uint8_t)ptrOffset[0] == 0x00 && (uint8_t)ptrOffset[1] == 0x00 && (uint8_t)ptrOffset[2] == 0xcc && (uint8_t)ptrOffset[3] == 0xd3){
                    // 分拣数据
                    ptrOffset = cache_buffer.data();

                    //通道号(4字节)
                    ptrOffset += 4;                    
                    uint32_t channel = static_cast<uint32_t>(ptrOffset[0]) << 24 |
                                      static_cast<uint32_t>(ptrOffset[1]) << 16 |
                                      static_cast<uint32_t>(ptrOffset[2]) << 8 |
                                      static_cast<uint32_t>(ptrOffset[3]);
                    //通道值转换
                    channel = (channel == 0xFFF1) ? 1 : 2;
                                
                    //数据包序号，4字节。两个探测器分别从1开始编号
                    ptrOffset += 4; //包头4字节
                    uint32_t dataNum = static_cast<uint32_t>(ptrOffset[0]) << 24 |
                                    static_cast<uint32_t>(ptrOffset[1]) << 16 |
                                    static_cast<uint32_t>(ptrOffset[2]) << 8 |
                                    static_cast<uint32_t>(ptrOffset[3]);

                    //粒子模式数据PARTICLE_NUM_ONE_PAKAGE*16byte, 6字节:空置；6字节：时间(单位:*10ns), 2字节：死时间（单位:*10ns）2字节：幅度
                    int ref = 1;
                    ptrOffset += 4;

                    std::vector<TimeEnergy> temp;
                    while (ref++<=PARTICLE_NUM_ONE_PAKAGE){
                        //空置48bit
                        ptrOffset += 6;

                        //时间:48bit
                        uint64_t t = static_cast<uint64_t>(ptrOffset[0]) << 40 |
                                      static_cast<uint64_t>(ptrOffset[1]) << 32 |
                                      static_cast<uint64_t>(ptrOffset[2]) << 24 |
                                      static_cast<uint64_t>(ptrOffset[3]) << 16 |
                                      static_cast<uint64_t>(ptrOffset[4]) << 8 |
                                      static_cast<uint64_t>(ptrOffset[5]);
                        t *= 10;
                        ptrOffset += 6;

                        //死时间:16bit
                        ptrOffset += 2;
                        unsigned short dietime = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);

                        //幅度:16bit
                        unsigned short e = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);
                        ptrOffset += 2;

                        if (t != 0x00 && e != 0x00)
                            temp.push_back(TimeEnergy(t, dietime, e));
                    }

                    //数据分拣完毕
                    {
                        DetTimeEnergy detTimeEnergy;
                        detTimeEnergy.channel = channel;
                        detTimeEnergy.timeEnergy.swap(temp);
                        result.push_back(detTimeEnergy);
                    }

                    cache_buffer.erase(cache_buffer.begin(), cache_buffer.begin() + packetSize);
                }
            } else {
                cache_buffer.erase(cache_buffer.begin());
            }

            buffer_size = cache_buffer.size();
        }
    }

    fclose(input_file);
    input_file = nullptr;

    return result;
}

void SysUtils::realAnalyzeTimeEnergy(const char* filename, std::function<void(DetTimeEnergy,
    unsigned long long progress/*文件进度*/, unsigned long long filesize/*文件大小*/,bool/*结束标识*/, bool */*是否被终止*/)> callback)
{
    //连接探测器
    SequenceNumber = 0;
    lossData.clear();

    bool interrupted = false;

    FILE* input_file = fopen(filename, "rb");
    unsigned long long progress = 0/*文件进度*/;//字节数
    _fseeki64(input_file, 0, SEEK_END);
    unsigned long long filesize = _ftelli64(input_file)/*文件大小*/;
    _fseeki64(input_file, 0, SEEK_SET);

    if (!input_file) {
        perror("Failed to open file");
        interrupted = true;
        callback(DetTimeEnergy(), 1, filesize, true, &interrupted);
        return;
    }

    uint32_t packetSize = (PARTICLE_NUM_ONE_PAKAGE +1) * 16;
    std::vector<uint8_t> read_buffer(packetSize, 0);
    std::vector<uint8_t> cache_buffer;
    while (!feof(input_file)){
        size_t read_size = fread(reinterpret_cast<char*>(read_buffer.data()), 1, packetSize, input_file);
        cache_buffer.insert(cache_buffer.end(), read_buffer.begin(), read_buffer.begin() + read_size);
        progress += read_size;

        uint32_t buffer_size = cache_buffer.size();
        while (buffer_size >= packetSize){
            //判断头部
            uint8_t *ptrOffset = cache_buffer.data();
            if ((uint8_t)ptrOffset[0] == 0x00 && (uint8_t)ptrOffset[1] == 0x00 && (uint8_t)ptrOffset[2] == 0xaa && (uint8_t)ptrOffset[3] == 0xb3){
                ptrOffset = cache_buffer.data() + packetSize - 4;
                //判断尾部
                if ((uint8_t)ptrOffset[0] == 0x00 && (uint8_t)ptrOffset[1] == 0x00 && (uint8_t)ptrOffset[2] == 0xcc && (uint8_t)ptrOffset[3] == 0xd3){
                    // 分拣数据
                    ptrOffset = cache_buffer.data();

                    //通道号(4字节)
                    ptrOffset += 4; //包头
                    uint32_t channel = static_cast<uint32_t>(ptrOffset[0]) << 24 |
                                       static_cast<uint32_t>(ptrOffset[1]) << 16 |
                                       static_cast<uint32_t>(ptrOffset[2]) << 8 |
                                       static_cast<uint32_t>(ptrOffset[3]);

                    //通道值转换
                    channel = (channel == 0xFFF1) ? 0 : 1;
                    //序号（4字节）
                    uint32_t dataNum =   static_cast<uint32_t>(ptrOffset[4]) << 24 |
                                        static_cast<uint32_t>(ptrOffset[5]) << 16 |
                                        static_cast<uint32_t>(ptrOffset[6]) << 8 |
                                        static_cast<uint32_t>(ptrOffset[7]);

                    ptrOffset += 8;

                    //粒子模式数据PARTICLE_NUM_ONE_PAKAGE*16byte,前6字节:时间,后2字节:能量
                    int ref = 0;
                    quint64 firsttime_temp = 0;
                    quint64 lasttime_temp = 0;
                    std::vector<TimeEnergy> temp;
                    while (ref++ < PARTICLE_NUM_ONE_PAKAGE){
                        //空置48bit
                        ptrOffset += 6;

                        //时间:48bit
                        uint64_t t =  static_cast<uint64_t>(ptrOffset[0]) << 40 |
                                      static_cast<uint64_t>(ptrOffset[1]) << 32 |
                                      static_cast<uint64_t>(ptrOffset[2]) << 24 |
                                      static_cast<uint64_t>(ptrOffset[3]) << 16 |
                                      static_cast<uint64_t>(ptrOffset[4]) << 8 |
                                      static_cast<uint64_t>(ptrOffset[5]);
                        t *= 10;
                        ptrOffset += 6;

                        //死时间:16bit
                        unsigned short deathtime = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);
                        deathtime *= 10;
                        ptrOffset += 2;

                        //幅度:16bit
                        unsigned short amplitude = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);
                        ptrOffset += 2;

                        if(ref == 1) firsttime_temp = t;
                        if(t>0) lasttime_temp = t; //一直更新最后一个数值，但是要确保t不是空值

                        if (t != 0x00 && amplitude != 0x00)
                            temp.push_back(TimeEnergy(t, deathtime, amplitude));
                    }

                    //对丢包情况进行修正处理
                    //考虑到实际丢包总是在大计数率下，网口传输响应不过来，两个通道的不响应时间长度基本相同，这里直接以探测器1的丢包来修正。
                    if(channel == 0){
                        quint32 losspackageNum = dataNum - SequenceNumber - 1; //注意要减一才是丢失的包个数
                        if(losspackageNum > 0) {
                            firstTime = firsttime_temp;
                            quint64 delataT = firstTime - lastTime;//丢包的时间段长度
                            
                            //检测丢包跨度是否刚好跨过某一秒的前后，
                            //经过初步测试，丢包的时候从来没有连续丢失超过1s的数据。
                            unsigned int leftT = static_cast<unsigned int>(ceil(lastTime*1.0/1e9)); //向上取整
                            unsigned int rightT = static_cast<unsigned int>(ceil(firstTime*1.0/1e9));
                            if((rightT - leftT) == 0){
                                // 记录丢失的数据点个数，考虑到丢包总是发送在大计数率，因此直接认为每次丢一个包损失64个计数。
                                lossData[leftT] += delataT;
                            }
                            else if( rightT  - leftT == 1){
                                unsigned long long t1 = 0LL, t2 = 0LL;
                                t1 = static_cast<unsigned long long>(leftT)*1e9 - lastTime;
                                t2 = firstTime - static_cast<unsigned long long>(leftT)*1e9;
                                if(t1>0 && t2>0)
                                {
                                    lossData[leftT] += t1; //注意计时从1开始，因为是向上取整
                                    lossData[rightT] += t2; //注意计时从1开始
                                }
                            }
                        }

                        //记录数据帧序号
                        SequenceNumber = dataNum;
                        //记录数据帧最后时间
                        lastTime = lasttime_temp;
                    }

                    //数据分拣完毕
                    {
                        DetTimeEnergy detTimeEnergy;
                        detTimeEnergy.channel = channel;
                        detTimeEnergy.timeEnergy.swap(temp);
                        callback(detTimeEnergy, progress, filesize, false, &interrupted);

                        if (interrupted){
                            fclose(input_file);
                            input_file = nullptr;
                            callback(DetTimeEnergy(), progress, filesize, true, &interrupted);
                            return;
                        }
                    }

                    cache_buffer.erase(cache_buffer.begin(), cache_buffer.begin() + packetSize);
                }
            } else {
                cache_buffer.erase(cache_buffer.begin());
            }

            buffer_size = cache_buffer.size();
        }
    }

    fclose(input_file);
    input_file = nullptr;
    callback(DetTimeEnergy(), progress, filesize, true, nullptr);
}

void SysUtils::readNetData(QString filename, std::function<void(DetTimeEnergy,
    unsigned long long progress/*文件进度*/, unsigned long long filesize/*文件大小*/,bool/*结束标识*/, bool */*是否被终止*/)> callback)
{
    //定义包头包尾
    const QByteArray header = QByteArray::fromHex("0000aab3");
    const QByteArray tailer = QByteArray::fromHex("0000ccd3");

    //初始化
    SequenceNumber = 0;
    lossData.clear();
    bool interrupted = false;

    unsigned long long progress = 0/*文件进度*/;//字节数
    qint64 filesize = getFileSize(filename);/*文件大小*/;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << file.errorString();
        interrupted = true;
        callback(DetTimeEnergy(), 1, filesize, true, &interrupted);
        return ;
    }

    uint32_t packetSize = (PARTICLE_NUM_ONE_PAKAGE +1) * 16;; // 每次读取1040字节
    QByteArray cache_buffer;
    QByteArray read_buffer;
    QByteArray onePackage;
    while (!file.atEnd()) {
        read_buffer = file.read(packetSize);
        progress += static_cast<unsigned long long>(read_buffer.size());
        cache_buffer.append(read_buffer);

        uint32_t buffer_size = cache_buffer.size();
        while (buffer_size >= packetSize){
            // 查找包头
            int headerPos = cache_buffer.indexOf(header);
            if (headerPos == -1) {
                cache_buffer.clear();
                break;
            }
            // 查找包尾
            int tailPos = cache_buffer.indexOf(tailer, headerPos + header.size());
            if (tailPos == -1) break;

            // 提取完整数据包
            int packetLength = tailPos + tailer.size() - headerPos;
            if(packetLength == packetSize){
                onePackage = cache_buffer.mid(headerPos, packetLength);
                // packets.append(onePackage);
            }

            // 移除已处理的数据
            cache_buffer = cache_buffer.mid(headerPos + packetLength);

            //提取数据
            const unsigned char *ptrOffset = (const unsigned char *)onePackage.constData();

            //包头4字节
            ptrOffset += 4;

            //通道号(4字节)
            quint32 channel = static_cast<quint32>(ptrOffset[0]) << 24 |
                              static_cast<quint32>(ptrOffset[1]) << 16 |
                              static_cast<quint32>(ptrOffset[2]) << 8 |
                              static_cast<quint32>(ptrOffset[3]);
            //通道值转换
            channel = (channel == 0xFFF1) ? 0 : 1;
            ptrOffset += 4;

            //序号（4字节）
            quint32 dataNum = static_cast<quint32>(ptrOffset[0]) << 24 |
                              static_cast<quint32>(ptrOffset[1]) << 16 |
                              static_cast<quint32>(ptrOffset[2]) << 8 |
                              static_cast<quint32>(ptrOffset[3]);
            ptrOffset += 4;

            int ref = 0;
            quint64 firsttime_temp = 0;
            quint64 lasttime_temp = 0;
            std::vector<TimeEnergy> temp;
            while (ref++ < PARTICLE_NUM_ONE_PAKAGE){
                //空置48bit
                ptrOffset += 6;

                //时间:48bit
                uint64_t t =  static_cast<uint64_t>(ptrOffset[0]) << 40 |
                             static_cast<uint64_t>(ptrOffset[1]) << 32 |
                             static_cast<uint64_t>(ptrOffset[2]) << 24 |
                             static_cast<uint64_t>(ptrOffset[3]) << 16 |
                             static_cast<uint64_t>(ptrOffset[4]) << 8 |
                             static_cast<uint64_t>(ptrOffset[5]);
                t *= 10;
                ptrOffset += 6;

                //死时间:16bit
                unsigned short deathtime = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);
                deathtime *= 10;
                ptrOffset += 2;

                //幅度:16bit
                unsigned short amplitude = static_cast<uint16_t>(ptrOffset[0]) << 8 | static_cast<uint16_t>(ptrOffset[1]);
                ptrOffset += 2;

                if(ref == 1) firsttime_temp = t;
                if(t>0) lasttime_temp = t; //一直更新最后一个数值，单是要确保t不是空值

                if (t != 0x00 && amplitude != 0x00)
                    temp.emplace_back(t, deathtime, amplitude);
            }

            //对丢包情况进行修正处理
            //考虑到实际丢包总是在大计数率下，网口传输响应不过来，两个通道的不响应时间长度基本相同，这里直接以探测器1的丢包来修正。
            if(channel == 0){
                quint32 losspackageNum = dataNum - SequenceNumber - 1; //注意要减一才是丢失的包个数
                if(losspackageNum > 0) {
                    firstTime = firsttime_temp;
                    quint64 delataT = firstTime - lastTime;//丢包的时间段长度

                    //检测丢包跨度是否刚好跨过某一秒的前后，
                    //经过初步测试，丢包的时候从来没有连续丢失超过1s的数据。
                    unsigned int leftT = static_cast<unsigned int>(ceil(lastTime*1.0/1e9)); //向上取整
                    unsigned int rightT = static_cast<unsigned int>(ceil(firstTime*1.0/1e9));

                    //暂时不考虑连续多秒丢包的修正问题
                    if((rightT - leftT) == 0){
                        // 记录丢失的数据点个数，考虑到丢包总是发送在大计数率，因此直接认为每次丢一个包损失64个计数。
                        lossData[leftT] += delataT;
                    }
                    else if( rightT - leftT == 1){
                        unsigned long long t1 = 0LL, t2 = 0LL;
                        t1 = static_cast<unsigned long long>(leftT)*1e9 - lastTime; //左端丢失的时间，单位ns
                        t2 = firstTime - static_cast<unsigned long long>(leftT)*1e9; //右端丢失的时间，单位ns
                        if(t1>0 && t2>0)
                        {
                            lossData[leftT] += t1; //注意计时从1开始，因为是向上取整
                            lossData[rightT] += t2; //注意计时从1开始
                        }
                    }
                }

                //记录数据帧序号
                SequenceNumber = dataNum;
                //记录数据帧最后时间
                lastTime = lasttime_temp;
            }

            //数据分拣完毕
            {
                DetTimeEnergy detTimeEnergy;
                detTimeEnergy.channel = channel;
                detTimeEnergy.timeEnergy.swap(temp);
                callback(detTimeEnergy, progress, filesize, false, &interrupted);

                if (interrupted){
                    file.close();
                    callback(DetTimeEnergy(), progress, filesize, true, &interrupted);
                    return;
                }
            }
        }
    }

    file.close();
    callback(DetTimeEnergy(), progress, filesize, true, nullptr);
}

void SysUtils::realQuickAnalyzeTimeEnergy(const char* filename, std::function<void(DetTimeEnergy, unsigned long long/*文件进度*/, unsigned long long/*文件大小*/, bool, bool*)> callback)
{
    lossData.clear();

    FILE* input_file = fopen(filename, "rb");
    if (ferror(input_file))
        return ;

    unsigned long long progress = 0/*文件进度*/;//字节数
    _fseeki64(input_file, 0, SEEK_END);
    unsigned long long filesize = _ftelli64(input_file)/*文件大小*/;
    _fseeki64(input_file, 0, SEEK_SET);
    bool interrupted = false;
    //识别是否为有效数据文件
    unsigned int FileHead = 0xFFFFFFFF; //文件包头，有效数据的识别码
    progress = 4;//字节数
    if(!feof(input_file)){
        fread(reinterpret_cast<unsigned char*>(&FileHead), 1, sizeof(FileHead), input_file);
        if(FileHead != 0xFFFFFFFF) {
            //不是合法文件，提醒用户
            fclose(input_file);
            input_file = nullptr;
            interrupted = true;
            callback(DetTimeEnergy(), 1, filesize, true, &interrupted);
            return;
        }
    }

    while (!feof(input_file)){
        // 先获取时间能量对个数
        uint32_t size = 0;
        fread(reinterpret_cast<uint32_t*>(&size), 1, sizeof(size), input_file);
        if (size == 0){
            break;//文件数据读完了
        }

        // 再获取时间能量对数据信息
        DetTimeEnergy detTimeEnergy;
        detTimeEnergy.channel = 0;
        detTimeEnergy.timeEnergy.resize(size);
        fread(reinterpret_cast<unsigned char*>(&detTimeEnergy.channel), 1, sizeof(detTimeEnergy.channel), input_file);
        size_t readSize = fread(reinterpret_cast<unsigned char*>(detTimeEnergy.timeEnergy.data()), 1, sizeof(TimeEnergy)*size, input_file);

        progress += 5/*sizeof(size)+sizeof(detTimeEnergy.channel)*/ + readSize;
        if (readSize != sizeof(TimeEnergy)*size){
            fclose(input_file);
            input_file = nullptr;
            interrupted = true;
            callback(DetTimeEnergy(), progress, filesize, true, &interrupted);
            std::vector<TimeEnergy>().swap(detTimeEnergy.timeEnergy);
            return;
        }

        callback(detTimeEnergy, progress, filesize, false, &interrupted);
        if (interrupted){
            fclose(input_file);
            input_file = nullptr;
            callback(DetTimeEnergy(), progress, filesize, true, &interrupted);
            std::vector<TimeEnergy>().swap(detTimeEnergy.timeEnergy);
            return;
        }

        std::vector<TimeEnergy>().swap(detTimeEnergy.timeEnergy);
    }

    fclose(input_file);
    input_file = nullptr;
    callback(DetTimeEnergy(), progress, filesize, true, nullptr);
}

// 报文完整性检查
bool SysUtils::isValidPacket(const QByteArray &packet)
{
    // 检查报文是否完整 是否以0x0000AAB3开头 是否以0x0000CCD3 结尾
    uint32_t packetSize = (PARTICLE_NUM_ONE_PAKAGE +1) * 16;
    QByteArray header = QByteArray::fromHex("0000aab3");
    QByteArray tailer = QByteArray::fromHex("0000ccd3");

    return packet.size() == packetSize &&
           packet.startsWith(header) &&
           packet.endsWith(tailer);
}

qint64 SysUtils::getFileSize(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << filePath;
        return -1;
    }
    return fileInfo.size();  // 返回文件大小(字节)
}
