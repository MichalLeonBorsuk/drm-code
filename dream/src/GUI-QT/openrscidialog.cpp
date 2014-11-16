#include "openrscidialog.h"
#include "ui_openrscidialog.h"
#include <QFileDialog>

OpenRSCIDialog::OpenRSCIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenRSCIDialog)
{
    ui->setupUi(this);
    connect(ui->pushButtonFile, SIGNAL(clicked()), this, SLOT(OnChooseFile()));
}

OpenRSCIDialog::~OpenRSCIDialog()
{
    delete ui;
}

void OpenRSCIDialog::accept()
{
    bool ok;
    int n = ui->lineEditPort->text().toInt(&ok);
    if(ok) {
        if(path == "") {
            file = ui->lineEditAddress->text().append(QString(":%1").arg(n));
            if(ui->radioButtonTCP->isChecked())
                file = QString("t").append(file);
        }
        else {
            file = path.append(QString("#%1").arg(n));
        }
    }
    else {
        file = path;
    }
    QDialog::accept();
}

#ifdef HAVE_LIBPCAP
# define PCAP_FILES " *.pcap"
#else
# define PCAP_FILES ""
#endif
#define RSCI_FILES "*.rsA *.rsB *.rsC *.rsD *.rsQ *.rsM" PCAP_FILES
#define RSCI_FILE1 RSCI_FILES " "
#define RSCI_FILE2 "MDI/RSCI Files (" RSCI_FILES ");;"
#
void OpenRSCIDialog::OnChooseFile()
{
    path = QFileDialog::getOpenFileName(this, tr("Open File"), file, tr("Supported Files (" RSCI_FILE1 " );; " RSCI_FILE2 " All Files (*)"));
}
