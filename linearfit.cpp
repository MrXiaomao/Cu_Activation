
#include "linearfit.h"


/////////////////////////////////////////////////////////////////////////
/**
 * @brief GoodNess::calcGoodNess 计算拟合优度(不知道为什么只有多项式的优度和Excel的计算结果一致)
 * @param x 样本x
 * @param y 样本y
 * @param count 样本数量
 */
void GoodNess::calcGoodNess(double *x, double *y, int count)
{
    double sum0 = 0.0,dy=0.0;
    double ESS = 0.0; // 回归平方和
    double RSS = 0.0; // 残差平方和
    double TSS = 0.0; // 总体平方和
	int i = 0;

    for(i=0;i<count;i++)
        sum0 += y[i];
    dy = sum0/count;

    for(i=0;i<count;i++)
    {
        ESS += pow(func(x[i]) - dy, 2);
        RSS += pow(y[i] - func(x[i]) ,2);
    }
    TSS = ESS + RSS;
    if(TSS == 0.0)
        goodness = 0.0;
    else
        goodness = ESS / TSS ;
}

double GoodNess::getGoodNess()
{
    return goodness;
}


/////////////////////////////////////////////////////////////////////////
/**
 * 多项式  y=(Pn*x^n)+(Pn-1*x^n-1)+...+P1*x+P0
 */
// 构造函数
Polynomial::Polynomial() :
    PolynomialFit() ,
    GoodNess() ,
    osx(NULL) ,
    osy(NULL) ,
    ocount(0)
{
}

// 析构函数
Polynomial::~Polynomial()
{
    if(osx != NULL)
        delete [] osx;

    if(osy != NULL)
        delete [] osy;
}

//设置样本--x:x轴数据，y:y轴数据，w:w权重系数，count样本数量
bool Polynomial::setSample(double *x,
                              double *y,
                              int count ,
                              bool enableWeight ,
                              double *w )
{
    copy(x, y, count);
    return PolynomialFit::setSample(x,y,count,enableWeight ,w);
}

// 曲线的原型--y=(Pn*x^n)+(Pn-1*x^n-1)+...+P1*x+P0
double Polynomial::func(double x)
{
    double result = 0;
    if(!isIntercedeEnable){
        for(int i(0) ; i<=degree ; i++)
        {
            result += getResult(i)*pow(x , degree-i);
        }
    }else{
        for(int i(0) ; i<degree ; i++)
        {
            result += getResult(i)*pow(x , degree-i);
        }
        result += intercede;
    }

    return result;
}

// 处理，添加拟合优度的计算
bool Polynomial::process()
{
    bool ret = PolynomialFit::process();
    if(ret == false)
        return false;

    calcGoodNess(osx,osy,ocount);
    return true;
}

//留取一份样本
void Polynomial::copy(double *x, double *y, int count)
{
    ocount = count;
    osx = new double[count] , osy = new double[count];
    for(int i(0) ; i < count ; i++)
    {
        osx[i] = x[i] ;
        osy[i] = y[i] ;
    }
}

//////////////////////////////////////////////////////////////////////////////
/**
 *  高斯函数拟合 y(x) = a*exp[-4*ln2*(x-u)^2/FWHM^2]
 */
Gaussian::Gaussian()
{
    setAttribute(2,false);
}

bool Gaussian::setSample(std::vector<double> x, std::vector<double> y, int count,  double* result)
{
    int degree = 2;
    double* weight = new double[count];
    //处理
    double *px = new double[count];
    double *py = new double[count];
    // 参考[1]陈伟,方方,莫磊,等.一种单能高斯峰解析方法[J].太赫兹科学与电子信息学报,2020,18(03):527-530.
    // 核能谱单能峰扣除背景后，其净计数近似服从高斯分布：y(x) = a*exp[-4*ln2*(x-u)^2/FWHM^2]
    // a为峰值，u为峰位道址，FWHM 为半高宽。
    // 将高斯函数变化为多项式，取对数,整理为二次多项式形式，b=c1x^2+c2x+c3;
    // b = lny, c1=-4ln2/(FWHM^2);c2=8ln2*u/FWHM^2; c2=-4ln2*u^2/FWHM^2+lna
    // **需要注意的是，由于这里取对数，需要保证sy的值大于0. 因此当 输入的sy中存在0时，拟合结果会可能是错误，
    // **这里简单的将拟合数据y为0的时候直接设置为1e-20.这对于能谱拟合是可以的，但是对于非能谱领域，需要重新考虑。
    
    // 复制数据
    std::copy(x.begin(), x.end(), px);

    for(int i=0; i<count; i++)
    {
        if(y[i]<1e-20) y[i] = 1e-20;
        py[i] = log(y[i]); //log()函数返回数字的自然对数。
        weight[i] = 1.0;
    }

    bool ret = PolynomialFit::setSample(px, py, count, false, weight);
    
    if(ret && process()) {
        result[0] = getResult(0);
        result[1] = getResult(1);
        result[2] = getResult(2);
    }

    delete [] weight;
    delete [] px;
    delete [] py;
    return ret;
}

double Gaussian::func(double x)
{
    return getResult(2) * exp(-4 * log(2) * (x-getResult(1)) * (x-getResult(1))/getResult(0)/getResult(0));
}


// 系数result[0]为半高宽FWHM, result[1] 为峰位，result[2]为峰值。
float Gaussian::getResult(int y)
{
    float c1,c2,c3;
    c1 = PolynomialFit::getResult(0);
    c2 = PolynomialFit::getResult(1);
    c3 = PolynomialFit::getResult(2);

    float FWHM = sqrt(-4*log(2)/c1);
    float mean = -c2/2/c1;
    float a = exp(c3+4*log(2)*mean*mean/(FWHM*FWHM));
    float result[3];
    result[0] = FWHM;
    result[1] = mean;
    result[2] = a;
    return result[y];
}

//////////////////////////////////////////////////////////////////////////////
/**
 * 对数  y = P2*ln(x) + P1
 */
Logarithm::Logarithm()
    : Polynomial()
{
    setAttribute(1,false);
}


bool Logarithm::setSample(double *x, double *y, int count, bool enableWeight, double *w)
{
    //留取一份样本
    copy(x,y,count);

    //处理
    double *px = new double[count];
    for(int i(0) ; i<count ; i++){
        if(x[i] <= 0.0){
            delete [] px;
            return false;
        }

        px[i] = log(x[i]);
    }

    bool ret = PolynomialFit::setSample(px,y,count,enableWeight,w);
    delete [] px;
    return ret;
}

double Logarithm::func(double x)
{
    return getResult(0) * log(x) + getResult(1);
}


//////////////////////////////////////////////////////////////////////////////
/**
 * 指数 y = P2*exp(P1*x)
 */
Exponent::Exponent() :
    Polynomial()
{
    setAttribute(1,false);
}

//设置样本
bool Exponent::setSample(double *x, double *y, int count, bool enableWeight, double *w)
{
    //留取一份样本
    copy(x,y,count);

    //处理
    double *py = new double[count];
    for(int i(0) ; i<count ; i++){
        if(y[i] <= 0.0){
            delete [] py;
            return false;
        }

        py[i] = log(y[i]);
    }

    bool ret = PolynomialFit::setSample(x,py,count,enableWeight,w);
    delete [] py;
    return ret;
}

//
double Exponent::func(double x)
{
    return getResult(0) * exp(x*getResult(1));
}

// 系数从左到右分别对应0、1、2...y
float Exponent::getResult(int y)
{
    if(y==0)
        return exp(PolynomialFit::getResult(1)) ;
    else
        return PolynomialFit::getResult(0);
}


//////////////////////////////////////////////////////////////////////////////
/**
 * 幂函数 y = P2*x^P1
 */
Power::Power() : Polynomial()
{
    setAttribute(1,false);
}

//设置样本
bool Power::setSample(double *x, double *y, int count, bool enableWeight, double *w)
{
    //留取一份样本
    copy(x,y,count);

    //处理
    double *px = new double[count];
    double *py = new double[count];
    for(int i(0) ; i<count ; i++){
        if(y[i] <= 0.0 || x[i] <= 0.0){
            delete [] px;
            delete [] py;
            return false;
        }

        px[i] = log(x[i]);
        py[i] = log(y[i]);
    }

    bool ret = PolynomialFit::setSample(px,py,count,enableWeight,w);
    delete [] px;
    delete [] py;
    return ret;
}

//
double Power::func(double x)
{
    return getResult(0) * pow(x ,(double)getResult(1));
}

// 系数从左到右分别对应0、1、2...y
float Power::getResult(int y)
{
    if(y==0)
        return exp(PolynomialFit::getResult(1)) ;
    else
        return PolynomialFit::getResult(0);
}

