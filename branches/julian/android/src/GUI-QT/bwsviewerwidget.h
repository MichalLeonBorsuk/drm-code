#ifndef BWSVIEWER_H
#define BWSVIEWER_H

#include "DialogUtil.h"
#include <string>
#include "../util/Utilities.h"
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWidget>
#include "BWSViewer.h" // for helper classes - TODO - move these to own files

namespace Ui {
class BWSViewerWidget;
}

class CMOTObject;
class CDRMReceiver;
class CDataDecoder;
class CWebsiteObject;
class CWebsiteCache;
class CNetworkReplyCache;
class CNetworkAccessManager;

class BWSViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BWSViewerWidget(CDRMReceiver&, CSettings&, int, QWidget* parent = 0);
    ~BWSViewerWidget();

private:
    Ui::BWSViewerWidget *ui;
    int short_id;
    CSettings&      settings;
    CNetworkAccessManager nam;
    QTimer          Timer;
    CDRMReceiver&   receiver;
    CDataDecoder*   decoder;
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
    uint32_t        iLastServiceID;
    uint32_t        iCurrentDataServiceID;
    bool            bLastServiceValid;
    uint32_t        iLastValidServiceID;
    QString         strLastLabel;

    bool Changed();
    void SaveMOTObject(const QString& strObjName, const CMOTObject& obj);
    bool RemoveDir(const QString &dirName, int level = 0);
    void SetupSavePath(QString& strSavePath);
    void UpdateButtons();
    void UpdateStatus();
    void UpdateWindowTitle(const uint32_t iServiceID, const bool bServiceValid, QString strLabel);
    QString ObjectStr(unsigned int count);
    void GetServiceParams(uint32_t* iServiceID=NULL, bool* bServiceValid=NULL, QString* strLabel=NULL, ETypeRxStatus* eStatus=NULL);
    virtual void eventShow(QShowEvent*);
    virtual void eventHide(QHideEvent*);

public slots:
    void OnTimer();
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
signals:
    void activated(int);
};

#endif // BWSVIEWER_H
