#include "coolingtimewidget.h"
#include "ui_coolingtimewidget.h"

CoolingTimeWidget::CoolingTimeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CoolingTimeWidget)
{
    ui->setupUi(this);
    connect(this, SIGNAL(sigUpdateTimeLength(int)), this, SLOT(on_update_timelength(int)));
}

CoolingTimeWidget::~CoolingTimeWidget()
{
    delete ui;
}

void CoolingTimeWidget::on_pushButton_clicked()
{
    qint32 time = ui->spinBox->value();
    emit sigAskTimeLength(time);
    this->close();
}

void CoolingTimeWidget::on_pushButton_2_clicked()
{
    this->close();
}

void CoolingTimeWidget::on_update_timelength(int value)
{
    ui->spinBox->setValue(value);
}
