#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QSqlDatabase db;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void getAllData();
    void getAboutInfo();
    void unknownCommand(QString s);
    void wrongNumberOfArguments();

    void on_edtInput_returnPressed();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
