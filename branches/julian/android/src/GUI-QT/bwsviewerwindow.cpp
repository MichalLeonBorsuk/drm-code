#include "bwsviewerwindow.h"

BWSViewerWindow::BWSViewerWindow(CDRMReceiver& rx, CMOTDABDec* dec, CSettings& s, int sid, QWidget* parent):
    CWindow(parent, Settings, "BWS"),
    viewer(rx, dec, s, sid, this)
{
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
