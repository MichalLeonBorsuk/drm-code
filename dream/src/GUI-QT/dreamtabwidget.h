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
    void on_engineeringMode(bool);
private slots:
    void on_currentChanged(int);
private:
    ReceiverController* controller;
    bool eng;

    QWidget* makeDataApp(int short_id, const CService& service) const;
    QWidget* makePacketApp(int short_id, const CService& service) const;
};

#endif // DREAMTABWIDGET_H
