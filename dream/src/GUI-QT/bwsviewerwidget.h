#ifndef BWSVIEWER_H
#define BWSVIEWER_H

#include <QWidget>
#include <../Parameter.h>
#include "BWSViewer.h"

namespace Ui {
class BWSViewerWidget;
}

class CDataDecoder;
class CMOTObject;
class CMOTDABDec;
class QShowEvent;
class QHideEvent;

class BWSViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BWSViewerWidget(int, QWidget* parent = 0);
    ~BWSViewerWidget();
public slots:
    void setDecoder(CDataDecoder*);
    void setServiceInformation(CService s);
    void setStatus(int, ETypeRxStatus);
    void setSavePath(const QString&);
private:
    Ui::BWSViewerWidget* ui;
    int         short_id;
    CMOTDABDec* decoder;
    CService    service;
    CNetworkAccessManager nam;
    CWebsiteCache   cache;
    bool            bHomeSet;
    bool            bPageLoading;
    bool            bSaveFileToDisk;
    bool            bRestrictedProfile;
    bool            bAllowExternalContent;
    bool            bClearCacheOnNewService;
    bool            bDirectoryIndexChanged;
    unsigned int    iLastAwaitingOjects;
    CCounter        waitobjs;
    const QString   strCacheHost;
    QString         strCurrentSavePath;

    bool Changed();
    void SaveMOTObject(const QString& strObjName, const CMOTObject& obj);
    bool RemoveDir(const QString &dirName, int level = 0);
    void UpdateButtons();
    void UpdateStatus();
    void UpdateWindowTitle(const uint32_t iServiceID, const bool bServiceValid, QString strLabel);
    QString ObjectStr(unsigned int count);

private slots:
    void OnClearCache();
    void OnHome();
    void OnStopRefresh();
    void OnBack();
    void OnForward();
    void OnSetProfile(bool);
    void OnSaveFileToDisk(bool);
    void OnAllowExternalContent(bool);
    void OnClearCacheOnNewService(bool);
    void OnWebViewLoadStarted();
    void OnWebViewLoadFinished(bool ok);
    void OnWebViewTitleChanged(const QString& title);
    void Update();
};

#endif // BWSVIEWER_H
