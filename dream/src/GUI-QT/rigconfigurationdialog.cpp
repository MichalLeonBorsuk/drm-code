#include "rigconfigurationdialog.h"
#include "ui_rigconfigurationdialog.h"

RigConfigurationDialog::RigConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RigConfigurationDialog)
{
    m_ui->setupUi(this);
    connect(m_ui->pushButtonTestRig, SIGNAL(clicked()), this, SLOT(OnButtonTestRig()));
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
/*
    if(r.rig->caps->port_type == RIG_PORT_SERIAL)
    {
	// TODO
    }
	stackedWidget->setEnabled(true);
	//stackedWidget->setpage(0);
	comboBoxComPort->clear();
	map<string,string> ports;
	GetComPortList(ports);
	for(map<string,string>::const_iterator i=ports.begin();
		i!=ports.end(); i++)
	{
	    comboBoxComPort->addItem(i->first.c_str(), i->second.c_str());
	}
	char port[200];
	r.rig->getConf("rig_pathname", port);
	if(port[0] != 0)
	{
		int n = comboBoxComPort->findData(port);
		comboBoxComPort->setCurrentIndex(n);
	}
    }
    else
    {
	comboBoxComPort->clear();
    	if(r.rig->caps->port_type == RIG_PORT_USB)
	{
	    stackedWidget->setEnabled(true);
	    //stackedWidget->setpage(1);
	}
	else
	{
	    stackedWidget->setEnabled(false);
	    //stackedWidget->setpage(0);
	}
*/

void RigConfigurationDialog::OnTimerRig()
{
}

void
RigConfigurationDialog::OnButtonTestRig()
{

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
