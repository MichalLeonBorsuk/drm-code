#include "rigconfigurationdialog.h"
#include "ui_rigconfigurationdialog.h"
#include "../util/Utilities.h"
#include <map>
#include "ReceiverSettingsDlg.h"
#include <QMessageBox>
using namespace std;

RigConfigurationDialog::RigConfigurationDialog(CSettings& s, const string& sec, QWidget *parent) :
    QDialog(parent), section(sec), settings(s),
    m_ui(new Ui::RigConfigurationDialog)
{
    m_ui->setupUi(this);
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(OnSubmit()));
    connect(m_ui->pushButtonTestRig, SIGNAL(clicked()), this, SLOT(OnButtonTestRig()));
    // TODO:
    m_ui->pushButtonTestRig->setEnabled(false);
    m_ui->CheckBoxEnableSMeter->setEnabled(false);
}

void RigConfigurationDialog::showEvent(QShowEvent *)
{
    m_ui->comboBoxPort->clear();
    rig_model_t model = settings.Get(section, "model", int(0));
	string port = settings.Get(section+"-conf", "rig_pathname", string(""));
	cout << "rig_pathname: " << port << endl;
    CRig rig(model);
    //rig.LoadSettings(section, settings);
	if(rig.port_type() == RIG_PORT_SERIAL)
	{
		m_ui->labelPort->setText(tr("Com Port"));
		map<string,string> ports;
		GetComPortList(ports);
		for(map<string,string>::const_iterator i=ports.begin();
			i!=ports.end(); i++)
		{
			m_ui->comboBoxPort->addItem(i->first.c_str(), i->second.c_str());
		}
    }
    else if(rig.port_type() == RIG_PORT_USB)
	{
		m_ui->labelPort->setText(tr("USB Port"));
		// TODO - populate USB list
	}
	else
	{
		m_ui->comboBoxPort->hide();
		m_ui->labelPort->hide();
	}
	m_ui->comboBoxPort->setCurrentIndex(0);
	if(port != "")
	{
		int n = m_ui->comboBoxPort->findData(port.c_str());
	cout << "rig_pathname index: " << n << endl;
		if(n>=0)
			m_ui->comboBoxPort->setCurrentIndex(n);
	}
}

RigConfigurationDialog::~RigConfigurationDialog()
{
    delete m_ui;
}

void RigConfigurationDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
	m_ui->retranslateUi(this);
	break;
    default:
	break;
    }
}

void RigConfigurationDialog::OnTimerRig()
{
}

void
RigConfigurationDialog::OnSubmit()
{
	int n = m_ui->comboBoxPort->currentIndex();
	QString data = m_ui->comboBoxPort->itemData(n).toString();
	settings.Put(section+"-conf", "rig_pathname", string(data.toUtf8()));
}

void
RigConfigurationDialog::OnButtonTestRig()
{
    QMessageBox::information(this, tr("Test Rig"), tr("Not done"), QMessageBox::Ok);

#if 0
	int n = treeViewRigs->currentIndex().row();
	CRig* rig = Rigs.rigs[n].rig;
	if(rig==NULL)
		return;
	if(rig->caps && rig->caps->port_type == RIG_PORT_SERIAL)
	{
	    int index = 0;// TODO comboBoxComPort->currentIndex();
	    string strPort = "";// TODO comboBoxComPort->itemData(index).toString().toStdString();
	    if(strPort!="")
	    {
		rig->setConf("rig_pathname", strPort.c_str());
	    }
	}
	labelRigInfo->setText(tr("waiting"));
	try
	{
	    rig->open();
	} catch(RigException e)
	{
	    labelRigInfo->setText(tr(e.message));
	}
#endif
	//labelRigInfo->setText(rig->getInfo());
//	TimerRig.start(500);
}
