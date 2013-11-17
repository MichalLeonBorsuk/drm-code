#ifndef SERVICESELECTOR_H
#define SERVICESELECTOR_H

#include <QWidget>
#include <QButtonGroup>
#include <QLabel>
#include <vector>

namespace Ui {
class ServiceSelector;
}

class ServiceSelector : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceSelector(QWidget *parent = 0);
    ~ServiceSelector();
    void setLabel(int, const QString&);
    void check(int i);
    void disableAll();

private:
    Ui::ServiceSelector* ui;
    QButtonGroup*		 pButtonGroup;
    std::vector<QLabel*> serviceLabels;
signals:
    void audioServiceSelected(int);
    void dataServiceSelected(int);
};

#endif // SERVICESELECTOR_H
