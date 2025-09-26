#ifndef ENERGYCALIBRATIONFORM_H
#define ENERGYCALIBRATIONFORM_H

#include <QWidget>

namespace Ui {
class EnergyCalibrationForm;
}

typedef class Matrix
{
public:
    int m, n;
    qreal maze[100][100]; // 100 X 100

    int mm()
    {
        return m;
    }
    int nn()
    {
        return n;
    }
    Matrix (int a, int b)
    {
        m = a;
        n = b;
        memset(maze, 0, sizeof (maze));
    }
    Matrix ()
    {

    }
    qreal& at(int x, int y)
    {
        return maze[x][y];
    }

    Matrix operator * (Matrix s)
    {
        Matrix K (this->m, s.n);
        for(int i = 0; i < this->m; i++)
        {
            for(int k = 0; k < s.n; k++)
            {
                qreal sum = 0.0;
                for(int j = 0; j < s.m; j++)
                {
                    sum += this->at(i, j) * s.at(j, k);
                }
                K.at(i, k) = sum;
            }
        }
        return K;
    }

    Matrix reverse()
    {
        Matrix k(this->n, this->m);

        for(int i = 0; i < k.m; i++)
        {
            for(int j = 0; j < k.n; j++)
            {
                k.at(i, j) = this->at(j, i);
            }
        }
        return k;
    }

    Matrix append(Matrix s)
    {
        Matrix K(this->m, this->n + s.n);
        memcpy(K.maze, this->maze, sizeof (this->maze));
        //拷贝S到K
        for(int i = 0; i < K.m; i++)
        {
            for(int j = 0; j < s.n; j++)
            {
                K.at(i, this->n + j) = s.at(i, j);
            }
        }
        return K;
    }

}Matrix;

class QCustomPlot;
class QCPItemText;
class EnergyCalibrationForm : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyCalibrationForm(QWidget *parent = nullptr);
    ~EnergyCalibrationForm();

signals:
    void doit(int x, int y);

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_del_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_clicked();

private:
    void initCustomPlot();

private:
    Ui::EnergyCalibrationForm *ui;
    QCustomPlot* customPlot;
    QCPItemText *fixedTextTtem;

    double minx, miny;
    double maxx, maxy;
    bool loadStatus;
    bool datafit;
    QVector<QPointF> points;
    void calculate(int);
    qreal C[10]; //拟合函数参数
    void solveC(Matrix);
    bool addData;
    int func;
};

#endif // ENERGYCALIBRATIONFORM_H
