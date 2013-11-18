#ifndef DREAMTABWIDGET_H
#define DREAMTABWIDGET_H

#include <QTabWidget>
class CService;

class DreamTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit DreamTabWidget(QWidget *parent = 0);
signals:
    void audioServiceSelected(int);
    void dataServiceSelected(int);

public slots:
    void onServiceChanged(int, const CService&);
private slots:
    void on_currentChanged(int);
};

#endif // DREAMTABWIDGET_H
