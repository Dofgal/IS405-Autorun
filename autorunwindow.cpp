#include "autorunwindow.h"
#include "ui_autorunwindow.h"
#include "funcs.cpp"
#include <tchar.h>
#include <winreg.h>

autoRunWindow::autoRunWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::autoRunWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("AutoRuns"));
}

autoRunWindow::~autoRunWindow()
{
    delete ui;
}

// Logon_Reg按钮
void autoRunWindow::on_logonRegBtn_clicked()
{
    // 设置表头
    QStandardItemModel  *model = new QStandardItemModel();
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Reg Path"));
    model->setHeaderData(3,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询注册表
    const TCHAR *sub1 = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    const TCHAR *sub2 = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run");
    const TCHAR *sub3 = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
    const TCHAR *sub4 = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx");
    logonReg(HKEY_LOCAL_MACHINE, sub1, model);
    logonReg(HKEY_LOCAL_MACHINE, sub2, model);
    logonReg(HKEY_LOCAL_MACHINE, sub3, model);
    logonReg(HKEY_LOCAL_MACHINE, sub4, model);
    logonReg(HKEY_CURRENT_USER, sub1, model);
    logonReg(HKEY_CURRENT_USER, sub2, model);
    logonReg(HKEY_CURRENT_USER, sub3, model);
    logonReg(HKEY_CURRENT_USER, sub4, model);

    const TCHAR *sub32_1 = _T("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run");
    const TCHAR *sub32_2 = _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run");
    const TCHAR *sub32_3 = _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
    const TCHAR *sub32_4 = _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx");
    logonReg(HKEY_LOCAL_MACHINE, sub32_1, model);
    logonReg(HKEY_LOCAL_MACHINE, sub32_2, model);
    logonReg(HKEY_LOCAL_MACHINE, sub32_3, model);
    logonReg(HKEY_LOCAL_MACHINE, sub32_4, model);
    logonReg(HKEY_CURRENT_USER, sub32_1, model);
    logonReg(HKEY_CURRENT_USER, sub32_2, model);
    logonReg(HKEY_CURRENT_USER, sub32_3, model);
    logonReg(HKEY_CURRENT_USER, sub32_4, model);
}

// Logon_Dir按钮
void autoRunWindow::on_logonDirBtn_clicked()
{
    // 设置表头
    QStandardItemModel *model = new QStandardItemModel();
    model->setColumnCount(3);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询目录
    TCHAR *dir1 = _T("%USERPROFILE%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
    TCHAR *dir2 = _T("%ProgramData%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
    logonDir(dir1, model);
    logonDir(dir2, model);
}

// Services按钮
void autoRunWindow::on_serviceBtn_clicked()
{
    // 设置表头
    QStandardItemModel *model = new QStandardItemModel();
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Start Type"));
    model->setHeaderData(3,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询服务
    service(model);
}

// Drivers按钮
void autoRunWindow::on_driverBtn_clicked()
{
    // 设置表头
    QStandardItemModel *model = new QStandardItemModel();
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Start Type"));
    model->setHeaderData(3,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询驱动
    driver(model);
}


// Scheduled Tasks按钮
void autoRunWindow::on_schdTaskBtn_clicked()
{
    // 设置表头
    QStandardItemModel *model = new QStandardItemModel();
    model->setColumnCount(3);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询计划任务
    TCHAR *dir = _T("C:\\Windows\\System32\\Tasks");
    schdTask(dir, model);
}

// Internet Explorer按钮
void autoRunWindow::on_ieExpBtn_clicked()
{
    // 设置表头
    QStandardItemModel *model = new QStandardItemModel();
    model->setColumnCount(3);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Reg/Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询浏览器对象
    TCHAR *sub1 = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects");
    TCHAR *sub2 = _T("SOFTWARE\\Microsoft\\Internet Explorer\\Extensions");
    TCHAR *sub3 = _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects");
    TCHAR *sub4 = _T("SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\Extensions");

    ieExplorer(HKEY_LOCAL_MACHINE, sub1, model);
    ieExplorer(HKEY_LOCAL_MACHINE, sub2, model);
    ieExplorer(HKEY_LOCAL_MACHINE, sub3, model);
    ieExplorer(HKEY_LOCAL_MACHINE, sub4, model);
    ieExplorer(HKEY_CURRENT_USER, sub1, model);
    ieExplorer(HKEY_CURRENT_USER, sub2, model);
    ieExplorer(HKEY_CURRENT_USER, sub3, model);
    ieExplorer(HKEY_CURRENT_USER, sub4, model);
}

// Known DLLs按钮
void autoRunWindow::on_knDllBtn_clicked()
{
    // 设置表头
    QStandardItemModel  *model = new QStandardItemModel();
    model->setColumnCount(3);
    model->setHeaderData(0,Qt::Horizontal,QString::fromLocal8Bit("Name"));
    model->setHeaderData(1,Qt::Horizontal,QString::fromLocal8Bit("Last Modified Time"));
    model->setHeaderData(2,Qt::Horizontal,QString::fromLocal8Bit("Image Path"));

    ui->autoRunTable->setModel(model);
    ui->autoRunTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 查询知名DLL
    knownDll(_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs"), model);
}
