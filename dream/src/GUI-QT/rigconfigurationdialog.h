#ifndef RIGCONFIGURATIONDIALOG_H
#define RIGCONFIGURATIONDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
    class RigConfigurationDialog;
}

class RigConfigurationDialog : public QDialog {
    Q_OBJECT
public:
    RigConfigurationDialog(QWidget *parent = 0);
    ~RigConfigurationDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::RigConfigurationDialog *m_ui;

private slots:
    void OnButtonTestRig();
    void OnTimerRig();
};

#endif // RIGCONFIGURATIONDIALOG_H
