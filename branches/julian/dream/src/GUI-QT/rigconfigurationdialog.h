#ifndef RIGCONFIGURATIONDIALOG_H
#define RIGCONFIGURATIONDIALOG_H

#include <QtGui/QDialog>
#include "../util/Settings.h"

namespace Ui {
    class RigConfigurationDialog;
}

class RigConfigurationDialog : public QDialog {
    Q_OBJECT
public:
    RigConfigurationDialog(CSettings& s, const std::string& sec, QWidget *parent = 0);
    ~RigConfigurationDialog();

protected:
    void changeEvent(QEvent *e);
    void showEvent(QShowEvent *e);

private:
    string section;
    CSettings& settings;
    Ui::RigConfigurationDialog *m_ui;

private slots:
    void OnButtonTestRig();
    void OnTimerRig();
    void OnSubmit();
};

#endif // RIGCONFIGURATIONDIALOG_H
