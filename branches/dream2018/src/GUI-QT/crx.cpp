#include "crx.h"
#include "../GlobalDefinitions.h"

CRx::CRx(CDRMTransceiver& nRx, QThread *parent) : QThread(parent), trx(nRx)
{

}

void
CRx::run()
{
    qDebug("Working thread started");
    try
    {
        /* Call main routine */
        trx.Start();
    }
    catch (CGenErr GenErr)
    {
        ErrorMessage(GenErr.strError);
    }
    catch (string strError)
    {
        ErrorMessage(strError);
    }
    qDebug("Working thread complete");
}

void CRx::LoadSettings()
{
    trx.LoadSettings();
}

void CRx::SaveSettings()
{
    trx.SaveSettings();
}

void CRx::SetInputDevice(const string& s)
{
    trx.SetInputDevice(s);
}

void CRx::SetOutputDevice(const string& s)
{
    trx.SetOutputDevice(s);
}

void CRx::GetInputDevice(string& s)
{
    trx.GetInputDevice(s);
}

void CRx::GetOutputDevice(string& s)
{
    trx.GetOutputDevice(s);
}

void CRx::EnumerateInputs(std::vector<std::string>& names, std::vector<std::string>& descriptions)
{
    trx.EnumerateInputs(names, descriptions);
}

void CRx::EnumerateOutputs(std::vector<std::string>& names, std::vector<std::string>& descriptions)
{
    trx.EnumerateOutputs(names, descriptions);
}

void CRx::Start()
{
    trx.Start();
}

void CRx::Restart()
{
    trx.Restart();
}

void CRx::Stop()
{
    trx.Stop();
}

CSettings* CRx::GetSettings()
{
    return trx.GetSettings();
}

void CRx::SetSettings(CSettings* s)
{
    trx.SetSettings(s);
}

CParameter*	CRx::GetParameters()
{
    return trx.GetParameters();
}

_BOOLEAN CRx::IsReceiver() const
{
    return trx.IsReceiver();
}

_BOOLEAN CRx::IsTransmitter() const
{
    return trx.IsTransmitter();
}
