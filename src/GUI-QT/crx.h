#ifndef CRX_H
#define CRX_H

#include <QThread>
#include "../DrmTransceiver.h"
#include <vector>

class CRx : public QThread, public CDRMTransceiver
{
    Q_OBJECT
public:
    explicit CRx(CDRMTransceiver& nRx, QThread *parent = nullptr);
    void run() override;
    CDRMTransceiver*                GetTRX() { return &trx; }

private:
    CDRMTransceiver& trx;

signals:

public slots:
    virtual void LoadSettings() override;
    virtual void SaveSettings() override;
    virtual void Start() override;
    virtual void SetInputDevice(const string&) override;
    virtual void SetOutputDevice(const string&) override;
    virtual string GetInputDevice() override;
    virtual string GetOutputDevice() override;
    virtual void EnumerateInputs(std::vector<std::string>& names, std::vector<std::string>& descriptions) override;
    virtual void EnumerateOutputs(std::vector<std::string>& names, std::vector<std::string>& descriptions) override;
    virtual void Restart() override;
    virtual void Stop() override;
    virtual CSettings*				GetSettings() override;
    virtual void					SetSettings(CSettings* pNewSettings) override;
    virtual CParameter*				GetParameters() override;
    virtual _BOOLEAN				IsReceiver() const override;
    virtual _BOOLEAN				IsTransmitter() const override;
};

#endif // CRX_H
