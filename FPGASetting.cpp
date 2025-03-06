#include "FPGASetting.h"
#include "ui_FPGASetting.h"

FPGASetting::FPGASetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FPGASetting)
{
    ui->setupUi(this);
}

FPGASetting::~FPGASetting()
{
    delete ui;
}
