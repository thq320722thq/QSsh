#include "Widget.h"
#include <QDebug>
#include <QGridLayout>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

Widget::~Widget()
{

}

void Widget::initUI()
{
    int nItemHeight = 30;
    m_bConnectState = false;
    resize(400,250);

    QLabel *pLabelIp = new QLabel(this);
    pLabelIp->setText("IP地址");
    pLabelIp->setMinimumHeight(nItemHeight);
    QLabel *pLabelUser = new QLabel(this);
    pLabelUser->setText("用户名");
    pLabelUser->setMinimumHeight(nItemHeight);
    QLabel *pLabelPort = new QLabel(this);
    pLabelPort->setText("密 码");
    pLabelPort->setMinimumHeight(nItemHeight);

    m_pComBoxIp = new QComboBox(this);
    m_pComBoxIp->addItem("192.168.1.122");
    m_pComBoxIp->setMinimumHeight(nItemHeight);
    m_pComBoxIp->setEditable(true);

    m_pComBoxUser = new QComboBox(this);
    m_pComBoxUser->addItem("root");
    m_pComBoxUser->setEditable(true);

    m_pLineEditPwd = new QLineEdit(this);
    m_pLineEditPwd->setEchoMode(QLineEdit::Password);
    m_pLineEditPwd->setText("17909");

    m_pLabelState = new QLabel(this);
    m_pLabelState->setFixedSize(10,10);
    setConnectState(m_bConnectState);

    m_pBtnConnect = new QPushButton(this);
    m_pBtnConnect->setText("连接");
    connect(m_pBtnConnect,SIGNAL(clicked()),this,SLOT(slotSshConnect()));

    m_pComBoxCmd = new QComboBox(this);
    m_pComBoxCmd->addItem("astparam dump");
    m_pComBoxCmd->addItem("ls -la");
    m_pComBoxCmd->addItem("cd /tmp");
    m_pComBoxCmd->setEditable(true);

    m_pBtnSend = new QPushButton(this);
    m_pBtnSend->setText("发送");
    connect(m_pBtnSend,SIGNAL(clicked()),this,SLOT(slotSshSendCmd()));

    m_pTextEdit = new QTextEdit(this);

    m_pBtnClearEdit = new QPushButton(this);
    m_pBtnClearEdit->setText("清除");

    connect(m_pBtnClearEdit,SIGNAL(clicked()),this,SLOT(slotClearEdit()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(pLabelIp,     0,0,1,1); //IP地址Label  坐标第0行0列  所占空间大小 1行1列
    layout->addWidget(pLabelUser,   1,0,1,1);
    layout->addWidget(pLabelPort,   2,0,1,1);
    layout->addWidget(m_pLabelState,3,0,1,1);//连接状态Label  坐标第3行0列  所占空间大小 1行1列

    layout->addWidget(m_pComBoxIp,   0,1,1,2);
    layout->addWidget(m_pComBoxUser, 1,1,1,2);
    layout->addWidget(m_pLineEditPwd,2,1,1,2);
    layout->addWidget(m_pBtnConnect, 3,1,1,2);

    layout->addWidget(m_pComBoxCmd,4,0,1,2);
    layout->addWidget(m_pBtnSend,  4,2,1,1);

    layout->addWidget(m_pTextEdit,    0,3,4,5);
    layout->addWidget(m_pBtnClearEdit,4,3,1,5);

    this->setLayout(layout);
}

void Widget::createConnection()
{
    QString strIp = m_pComBoxIp->currentText();
    QString strUser = m_pComBoxUser->currentText();
    QString strPwd = m_pLineEditPwd->text();
    m_sshSocket = new CConnectionForSshClient(strIp,22,strPwd,strUser);
    m_sshSocket->init();
    connect(m_sshSocket,SIGNAL(sigConnectStateChanged(bool,QString,int)),
            this,SLOT(slotConnectStateChanged(bool,QString,int)));
    connect(m_sshSocket,SIGNAL(sigDataArrived(QString ,QString , int )),
            this,SLOT(slotDataArrived(QString ,QString , int )));
    connect(this,SIGNAL(sigSend(QString)),m_sshSocket,SLOT(slotSend(QString)));
    connect(this,SIGNAL(sigDisconnected()),m_sshSocket,SLOT(slotDisconnected()));

}

void Widget::disConnection()
{
    emit sigDisconnected();
}

void Widget::setConnectState(bool bState)
{
    if(!bState)
        m_pLabelState->setStyleSheet("QLabel{background-color:#ff0000;border-radius:5px;}");
    else
        m_pLabelState->setStyleSheet("QLabel{background-color:#00ff00;border-radius:5px;}");
}

void Widget::slotSshConnect()
{
    if(!m_bConnectState){
        m_pBtnConnect->setText("连接中...");
        createConnection();  //发起连接
    }else{
        m_pBtnConnect->setText("连接");
        m_bConnectState = false;
        emit sigDisconnected();//断开连接
        setConnectState(m_bConnectState);
    }
}

void Widget::slotSshSendCmd()
{
    if(m_bConnectState){
        QString strCmd = m_pComBoxCmd->currentText();
        strCmd += "\n"; //添加回车
        emit sigSend(strCmd);
    }
}

void Widget::slotClearEdit()
{
    m_pTextEdit->clear();
}

void Widget::slotConnectStateChanged(bool bState, QString strIp, int nPort)
{
    Q_UNUSED(strIp)
    Q_UNUSED(nPort)

    m_bConnectState = bState;
    setConnectState(m_bConnectState);
    if(m_bConnectState)
        m_pBtnConnect->setText("断开");
    else
        m_pBtnConnect->setText("连接");
}

void Widget::slotDataArrived(QString strMsg, QString strIp, int nPort)
{
    Q_UNUSED(strIp)
    Q_UNUSED(nPort)

    m_pTextEdit->append(strMsg);
    m_pTextEdit->moveCursor(QTextCursor::End);
}
