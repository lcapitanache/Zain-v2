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
    QString lastInput; //TODO: Implementar último comando

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    //Eventos
    void on_edtInput_returnPressed();

    //Flags
    bool nssIsValid(QString s);

    //Funciones
    int getCheckDigit(QString s);
    QString getFolio();
    QImage getQrCode(QString s);

    //Métodos
    void clearOutput();
    void exitApp();

    //Métodos de mensaje
    void showAboutInfo();
    void showAllData();
    void showCheckDigit(QString s);
    void showFolio();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
