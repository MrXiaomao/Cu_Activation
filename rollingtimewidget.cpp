#include "rollingtimewidget.h"

RollingTimeWidget::RollingTimeWidget(QWidget *parent)
    :QWidget(parent)
{
    /* 去边框,同时保留窗口原有的属性 */
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());;
    /* 设置背景样式 */
    this->setStyleSheet("background:white;");
    /* 设置动画目标 */
    rollingAni.setTargetObject(this);
    /* 设置动画目标属性 */
    rollingAni.setPropertyName("posYShifting");
    /* 设置动画持续时间 */
    rollingAni.setDuration(500);
    /* 设置动画曲线样式 */
    rollingAni.setEasingCurve(QEasingCurve::OutCirc);
}

RollingTimeWidget::~RollingTimeWidget()
{

}

void RollingTimeWidget::setTimeRange(int min, int max)
{
    min_Range = min;
    max_Range = max;
    update();
}

int RollingTimeWidget::PosYShifting()
{
    /* 注册属性取值函数 */
    return posYShifting;
}

void RollingTimeWidget::setPosYShifting(int posYShifting)
{
    /* 注册属性赋值函数 */
    this->posYShifting = posYShifting;
    update();
}

void RollingTimeWidget::setCurrTimeVal(int val)
{
    currentTime = val;
    rollAnimation();
    update();
}


void RollingTimeWidget::mousePressEvent(QMouseEvent *event)
{
    rollingAni.stop();
    /* 开启鼠标拖动滚动判决 */
    rollingJudgment = true;
    /* 刷新按下时鼠标位置 */
    currentPosY = event->pos().y();
}

void RollingTimeWidget::wheelEvent(QWheelEvent *event)
{
    /* 以滚轮delta值正负性作为判决条件进行判决 */
    if(event->delta()>0)
    {
        if(currentTime > min_Range){
            posYShifting = this->height()/4;
        }
    }
    else{
        if(currentTime < max_Range){
            posYShifting = - this->height()/4;
        }
    }
    rollAnimation();
    update();
}

void RollingTimeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(rollingJudgment)
    {
        if((currentTime == min_Range && event->pos().y() >= currentPosY) || (currentTime == max_Range && event->pos().y() <= currentPosY)){
            return;
        }else{
            posYShifting = event->pos().y() - currentPosY;
        }
        update();
    }
}

void RollingTimeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    /*拖动判决归位*/
    if(rollingJudgment)
    {
        rollingJudgment = false;
        /* 开启动画 */
        rollAnimation();
    }
}

void RollingTimeWidget::paintEvent(QPaintEvent *paint)
{
    Q_UNUSED(paint);
    /* 创建画笔对象 */
    QPainter painter(this);
    /* 画笔抗锯齿操作 */
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing, true);

    /* 偏移量检测，以1/4高度和取值范围为边界 */
    if(posYShifting >= height() / 4 && currentTime > min_Range)
    {
        /* 鼠标起始位置归位，即加上1/4的高度 */
        currentPosY += height()/4;
        /* 偏移量归位，即减去1/4的高度 */
        posYShifting -= height()/4;
        /* currentTime自减 */
        currentTime -= 1;
    }
    if(posYShifting <= -height() / 4 && currentTime < max_Range)
    {
        currentPosY -= height() / 4;
        posYShifting += height() / 4;
        currentTime += 1;
    }

    /* 调用函数画出数字currentTime */
    drawNumber(painter,currentTime,posYShifting);

    /* 调用函数画出两侧数字 */
    if(currentTime != min_Range){
        drawNumber(painter,currentTime - 1,posYShifting - height() / 4);
    }
    if(currentTime != max_Range){
        drawNumber(painter,currentTime + 1,posYShifting + height() / 4);
    }

    /* 调用函数画出两侧数字 */
    if(posYShifting >= 0 && currentTime - 2 >= min_Range){
        drawNumber(painter,currentTime - 2,posYShifting - height() / 2);
    }

    if(posYShifting <= 0 && currentTime + 2 <= max_Range){
        drawNumber(painter,currentTime + 2,posYShifting + height() /  2);
    }

    /* 画出数字currentTime两侧边框 */
    painter.setPen(QPen(QColor(70,144,249),2));
    painter.drawLine(0,height() / 8 * 3,width(),height() / 8 * 3);
    painter.drawLine(0,height() / 8 * 5,width(),height() / 8 * 5);

    emit signal_update_cur_time_val(currentTime);
}

void RollingTimeWidget::drawNumber(QPainter &painter, int value, int offset)
{
    /* 通过偏移量控制数字大小size */
    int size = (this->height() - abs(offset)) / numberSize;
    /* 通过偏移量控制数字透明度transparency */
    int transparency = 255 - 510 * abs(offset) / height();
    /* 通过偏移量控制数字在ui界面占据空间高度height */
    int height = this->height() / 2 - 3 * abs(offset) / 5;
    /* 计算数字显示位置 */
    int y = this->height() / 2 + offset - height / 2;

    QFont font;
    /* 设置字体大小 */
    font.setPixelSize(size);
    /* 画笔设置字体 */
    painter.setFont(font);
    /* 画笔设置颜色 */
    painter.setPen(QColor(0,0,0,transparency));
    /* 画笔painter画出文本*/
    painter.drawText(QRectF(0,y,width(),height),Qt::AlignCenter,(QString::number(value)));
}

void RollingTimeWidget::rollAnimation()
{
    /* 动画判决，达到条件开启相应动画 */
    if(posYShifting > height() / 8)
    {
        /* 设置属性动画初始value */
        rollingAni.setStartValue(height() / 8 - posYShifting);
        /* 设置属性动画终止value */
        rollingAni.setEndValue(0);
        /* currentTime值变更 */
        currentTime--;
    }
    else if(posYShifting > -height() / 8)
    {
        /* 设置属性动画初始value */
        rollingAni.setStartValue(posYShifting);
        /* 设置属性动画终止value */
        rollingAni.setEndValue(0);
    }
    else if(posYShifting < -height() / 8)
    {
        /* 设置属性动画初始value */
        rollingAni.setStartValue(-height() / 8 - posYShifting);
        /* 设置属性动画终止value */
        rollingAni.setEndValue(0);
        /* currentTime值变更 */
        currentTime++;
    }
    /* 动画开始 */
    rollingAni.start();
    update();
}

