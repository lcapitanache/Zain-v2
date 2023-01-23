#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "alignedSqlQueryModel.h"

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
    QString lastInput;
    int currentLineInHistory;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:

    //Eventos
    void on_edtInput_returnPressed();

    //Flags    
    bool commandIsOK(QMap <QString, int> commands, QString cmd);
    bool numOfArgsIsOK(QString cmd, int expectedNumArgs, int numArgsPassed);
    bool nssIsOK(QString s);

    //Funciones            
    int getCheckDigit(QString s);
    int getHistoryTotalLines();
    QString getFolio();
    QString getQrCode(QString folio);        

    //Métodos
    void addToHistory(QString line);
    void checkCommand();
    void clearOutput();
    void executeCommand(int cmd, QString argument);
    void exitApp();        
    void readFromHistory();
    void resizeColumnsAndRows();
    void setCurrentLineInHistory();

    //Métodos de mensaje
    void showAboutInfo();
    void showAllData();
    void showCheckDigit(QString s);
    void showFolio();
    void showHelp();
    void showHistory();
    void showManual(QString cmd);

private:
    Ui::MainWindow *ui;    
    AlignedSqlQueryModel *model;
};
#endif // MAINWINDOW_H
