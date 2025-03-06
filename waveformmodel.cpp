#include "waveformmodel.h"
#include "ui_waveformmodel.h"

WaveformModel::WaveformModel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WaveformModel)
{
    ui->setupUi(this);
}

WaveformModel::~WaveformModel()
{
    delete ui;
}
