#include "spectrumModel.h"
#include "ui_spectrumModel.h"

SpectrumModel::SpectrumModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpectrumModel)
    , measuring(false)
{
    ui->setupUi(this);
}

SpectrumModel::~SpectrumModel()
{
    delete ui;
}

void SpectrumModel::on_pushButton_start_clicked()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    if (measuring){
        measuring = false;
        btn->setText(tr("开始测量"));
    } else {
        measuring = true;
        btn->setText(tr("停止测量"));
    }
}

void SpectrumModel::on_pushButton_save_clicked()
{
    // 保存数据
}
