#include "chartdialog.h"
#include "ui_chartdialog.h"

ChartDialog::ChartDialog(QWidget *parent) :
    QDialog(parent), plot(CDRMPlot::createPlot(parent)),
    ui(new Ui::ChartDialog)
{
    ui->setupUi(this);
    plot->widget()->setParent(this);
    ui->gridLayout->removeWidget(ui->widget);
    ui->gridLayout->addWidget(plot->widget());
}

ChartDialog::~ChartDialog()
{
    delete ui;
}
