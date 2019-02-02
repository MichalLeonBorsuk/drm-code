#ifndef RXSIGNALLER_H
#define RXSIGNALLER_H

#include <QObject>
#include "../SDC/audioparam.h"

class RxSignaller : public QObject
{
    Q_OBJECT
public:
    explicit RxSignaller(QObject *parent = nullptr);
    void signalAudioConfig(const CAudioParam&);

signals:
    void audioConfigSignalled(const CAudioParam&);

public slots:
};

#endif // RXSIGNALLER_H
