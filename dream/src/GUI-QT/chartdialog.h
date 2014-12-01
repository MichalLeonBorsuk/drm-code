#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include "DRMPlot.h"

namespace Ui {
class ChartDialog;
}

class ChartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChartDialog(QWidget *parent = 0);
    ~ChartDialog();
    void SetupChart(const ECharType eNewType) { plot->SetupChart(eNewType); }
    CDRMPlot *GetPlot() { return plot; }
    void SetPlotStyle(const int iNewStyleID) { plot->SetPlotStyle(iNewStyleID); }
    ECharType getChartType() const { return plot->getChartType(); }
public slots:
    void update(ReceiverController* rc) { plot->update(rc); }

private:
    CDRMPlot*       plot;
    Ui::ChartDialog *ui;
};

#endif // CHARTDIALOG_H
