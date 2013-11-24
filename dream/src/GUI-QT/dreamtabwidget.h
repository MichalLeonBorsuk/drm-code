#ifndef DREAMTABWIDGET_H
#define DREAMTABWIDGET_H

#include <QTabWidget>
class CService;
class ReceiverController;

class DreamTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit DreamTabWidget(ReceiverController*, QWidget *parent = 0);
signals:
    void audioServiceSelected(int);
    void dataServiceSelected(int);

public slots:
    void onServiceChanged(int, const CService&);
    void setText(int, QString);
private slots:
    void on_currentChanged(int);
private:
    ReceiverController* controller;

    void addAudioTab(int, const CService&);
    void addDataTab(int, const CService&, int);
};

#endif // DREAMTABWIDGET_H
