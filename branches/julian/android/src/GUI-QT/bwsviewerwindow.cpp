#include "bwsviewerwindow.h"

BWSViewerWindow::BWSViewerWindow(CService& service, CMOTDABDec* dec, CSettings& s, QWidget* parent):
    CWindow(parent, Settings, "BWS"),
    viewer(dec, s, this)
{
    viewer.setServiceInfo(service);
    //container->connect(ui->buttonDecode, SIGNAL(clicked()), this, SLOT(close()));
    //container->connect(actionClear_Cache, SIGNAL(triggered()), SLOT(OnClearCache()));
    //container->connect(actionClose, SIGNAL(triggered()), SLOT(close()));
    //container->connect(actionAllow_External_Content, SIGNAL(triggered(bool)), SLOT(OnAllowExternalContent(bool)));
    //container->connect(actionRestricted_Profile_Only, SIGNAL(triggered(bool)), SLOT(OnSetProfile(bool)));
    //container->connect(actionSave_File_to_Disk, SIGNAL(triggered(bool)), SLOT(OnSaveFileToDisk(bool)));
    //container->connect(actionClear_Cache_on_New_Service, SIGNAL(triggered(bool)), SLOT(OnClearCacheOnNewService(bool)));
}

BWSViewerWindow::~BWSViewerWindow()
{
}

void BWSViewerWindow::showEvent(QShowEvent*)
{
    viewer.eventShow();
}

void BWSViewerWindow::hideEvent(QShowEvent*)
{
    viewer.eventHide();
}
