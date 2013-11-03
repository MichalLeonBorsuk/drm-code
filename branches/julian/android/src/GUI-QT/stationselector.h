#ifndef STATIONSELECTOR_H
#define STATIONSELECTOR_H

#include <QWidget>
#include <QIcon>
#include <QTimer>
#include "Schedule.h"

namespace Ui {
class StationSelector;
}

class StationSelector : public QWidget
{
    Q_OBJECT

public:
    explicit StationSelector(QWidget *parent = 0);
    ~StationSelector();

public slots:
    void            on_SwitchMode(int);
    void            on_newFrequency(int);

private:
    Ui::StationSelector *ui;
    virtual void	closeEvent(QCloseEvent* pEvent);
    virtual void	hideEvent(QHideEvent* pEvent);
    virtual void	showEvent(QShowEvent* pEvent);
    void			LoadSettings();
    void			SaveSettings();
    void			LoadSchedule();
    bool            showAll();
    void			LoadScheduleView();
    void			UpdateTransmissionStatus();
    void			LoadFilters();
    void			AddWhatsThisHelp();
    void			EnableSMeter();
    void			DisableSMeter();
    bool            GetSortAscending();
    void			SetSortAscending(bool b);
    void			ColumnParamFromStr(const QString& strColumnParam);
    void			ColumnParamToStr(QString& strColumnParam);
    int				currentSortColumn();
    bool            bCurrentSortAscendingdrm;
    bool            bCurrentSortAscendinganalog;
    int				iSortColumndrm;
    int				iSortColumnanalog;
    QString			strColumnParamdrm;
    QString			strColumnParamanalog;

    bool            bIsDRM;
    CSchedule		schedule;
    QIcon			greenCube;
    QIcon			redCube;
    QIcon			orangeCube;
    QIcon			pinkCube;
    QNetworkAccessManager *manager;
    QTimer			Timer;

    QMutex			ListItemsMutex;

    QString			okMessage, badMessage;

signals:
    void subscribeRig();
    void unsubscribeRig();
    void tuningRequest(int);

private slots:
    void OnSigStr(double);
    void OnTimer();
    void OnUrlFinished(QNetworkReply*);
    void on_frequency_valueChanged(int);
    void Onstations_header_sectionClicked(int c);
    void on_update_clicked();
    void on_stations_itemSelectionChanged();
    void on_comboBoxFilterTime_activated(int);
    void on_comboBoxFilterTarget_activated(const QString&);
    void on_comboBoxFilterCountry_activated(const QString&);
    void on_comboBoxFilterLanguage_activated(const QString&);
};

#endif // STATIONSELECTOR_H
