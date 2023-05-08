#ifndef AUTORUNWINDOW_H
#define AUTORUNWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class autoRunWindow; }
QT_END_NAMESPACE

class autoRunWindow : public QMainWindow
{
    Q_OBJECT

public:
    autoRunWindow(QWidget *parent = nullptr);
    ~autoRunWindow();

private:
    Ui::autoRunWindow *ui;

private slots:
    // 定义信号函数
    void on_logonRegBtn_clicked();
    void on_logonDirBtn_clicked();
    void on_serviceBtn_clicked();
    void on_driverBtn_clicked();
    void on_schdTaskBtn_clicked();
    void on_ieExpBtn_clicked();
    void on_knDllBtn_clicked();
};
#endif // AUTORUNWINDOW_H
