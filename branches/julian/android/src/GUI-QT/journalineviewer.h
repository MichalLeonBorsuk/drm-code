#ifndef JOURNALINEVIEWER_H
#define JOURNALINEVIEWER_H

#include <QWidget>
#include <QTimer>
#include <QTextDocument>
#include <../Parameter.h>
#include <../util/Settings.h>
#include <../datadecoding/Journaline.h>

namespace Ui {
class JournalineViewer;
}

class JournalineViewer : public QWidget
{
    Q_OBJECT

public:
    explicit JournalineViewer(CParameter&, CJournaline*, CSettings&, int, QWidget *parent = 0);
    ~JournalineViewer();

private:
    Ui::JournalineViewer *ui;
    QTimer Timer;
    QTextDocument   document;
    CParameter&     Parameters;
    CSettings&      settings;
    bool            decoderSet;
    int             short_id;

    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);

private slots:
    void OnTimer();
    void on_buttonClearCache_clicked();
    void on_buttonDecode_toggled(bool);
    void OnSetFont();

public slots:
    void deactivate();
signals:
    void activated(int);
};

#endif // JOURNALINEVIEWER_H
