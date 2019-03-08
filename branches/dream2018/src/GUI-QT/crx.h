#ifndef CRX_H
#define CRX_H

#include "ctrx.h"
#include <QObject>

class CDRMReceiver;

class CRx : public CTRx
{
    Q_OBJECT
public:
    explicit CRx(CDRMReceiver& nRx, CTRx *parent = nullptr);
    virtual ~CRx();
public slots:
};

#endif // CRX_H
