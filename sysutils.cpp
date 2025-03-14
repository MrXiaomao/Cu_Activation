#include "sysutils.h"

SysUtils::SysUtils()
{

}

// 计算直方图
vector<int> computeHistogram(const vector<unsigned int>& data, const vector<int>& binEdges) {
    vector<int> histogram(binEdges.size() - 1, 0); // 初始化直方图，大小为 binEdges.size() - 1

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
vector<int> GetSpectrum(const vector<unsigned int>& data, unsigned int maxEnergy, int ch)
{
    // 自动算出多道计数器的能量bin。
    vector<int> binEdges;
    int binWidth = maxEnergy/ch;
    for(int i=0; i< ch+1; i++){
        binEdges.push_back(binWidth*i);
    }
    // 计算能谱
    vector<int> spectrum = computeHistogram(data, binEdges);
    return spectrum;
}

// 计算data中数据在指定能量区间的数据个数
// data：输入的能量数据点
// leftE：左区间
// rightE：右区间
// stepT统计的时间步长, 单位s
// return double：计数率,cps
double GetCount(const vector<unsigned int> &data, int stepT, int leftE, int rightE)
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
vector<TimeCountRate> GetCountRate(const vector<long long> &dataT, const vector<unsigned int> &dataE, int stepT, int leftE, int rightE)
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
        cout << "Bin "<<i<<"[" << binEdges[i] << ", " << binEdges[i + 1] << "): " << histogram[i] << std::endl;
    }
}

// count  待拟合数据长度
// sx 待拟合数据x数组，
// sy 待拟合数据y数组
// 返回值result, result[0]为半高宽FWHM,result[1] 为峰位，result[2]为峰值。
#include "polynomialfit.h"
void fit_GaussCurve(int count, std::vector<double> sx, std::vector<double> sy, double* result)
{
    int degree = 2;
    double* weight = new double[count];
    // 赋初值
    result[0] = 0.;
    result[1] = 0.;
    result[2] = 0.;
    // 参考[1]陈伟,方方,莫磊,等.一种单能高斯峰解析方法[J].太赫兹科学与电子信息学报,2020,18(03):527-530.
    // 核能谱单能峰扣除背景后，其净计数近似服从高斯分布：y(x) = a*exp[-4ln2(x-u)^2/FWHM^2]
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
    }else
        std::cout << "failed" ;

    delete[] weight;
}
