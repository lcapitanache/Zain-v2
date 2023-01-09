#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QZXing.h"

#include <QMessageBox>
#include <QDebug>
#include <QStringList>
#include <QDateTime>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowIcon(QIcon(":img/logo-96.png"));
    this->setStyleSheet("background-color: white;");

    ui->setupUi(this);
    ui->edtOutput->setReadOnly(true);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("athena.db");

    showAllData();

    QString data = "Oficio de incapacidades iniciales" \
                   " folio JSMF/2023.01.08/7.212247." \
                   " Creado por Luis Capitanache el día" \
                   " domingo, 08 de enero de 2023 a las 21:22:47.";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::exitApp()
{
    if (db.open())
        db.close();

    QApplication::quit();
}

void MainWindow::clearOutput()
{
    ui->edtOutput->clear();
    ui->lblInformation->clear();
}

void MainWindow::showAboutInfo()
{
    QString about;

    about = "Zaín v2\n" \
            "Sistema de captura de incapacidades\n\n" \
            "Instituto Mexicano del Seguro Social\n" \
            "Unidad de Medicina Familiar N.° 36\n\n" \
            "© 2023 Luis Capitanache\n" \
            "lcapitanache@gmail.com\n" \
            "https://lcapitanache.github.io/\n\n" \
            "Qt 5.9.9 | MinGW 53 | x86 | 32-bits";

    ui->edtOutput->setText(about);
    ui->lblInformation->setText("Acerca de Zaín v2");
}

void MainWindow::showAllData()
{
    QSqlQuery query;
    QString data = "";
    int i = 0;

    db.open();
    query.exec("SELECT * FROM incapacidades ORDER BY Tipo, NSS");

    while(query.next())
    {
        i++;
        //data += QString::number(i) + "\t";
        data += query.value(0).toString() + "   ";
        data += query.value(1).toString() + "   ";
        data += query.value(2).toString() + "   ";
        data += query.value(3).toString() + "   ";
        data += query.value(4).toString() + "\t";
        data += query.value(7).toString() + "   ";
        data += query.value(8).toString() + "\n";
    }

    ui->edtOutput->setText(data);
    ui->lblInformation->setText(QString::number(i) + " registros encontrados");

    query.clear();
    db.close();
}

void MainWindow::showCheckDigit(QString s)
{
    if (nssIsValid(s))
    {
        QString message;

        message = "Cálculo de dígito verificador\n" \
                  "-----------------------------\n\n" \
                  "NSS:\t" + s + "\n" \
                  "Dígito:\t" + QString::number(getCheckDigit(s));

        ui->edtOutput->setText(message);
        ui->lblInformation->setText("Listo");
    }
    else
    {
        errBadNss(s);
    }
}

void MainWindow::showFolio()
{
    QString folio;
    QString message;

    folio = getFolio();

    message = "Número de folio\n" \
              "---------------\n\n";

    message += folio;

    ui->edtOutput->setText(message);
    ui->lblInformation->setText("Listo");

    getQrCode(folio);

    //QLabel label("<img src='..\\QrCode.jpg'/>");
    //label.show();
}

QString MainWindow::getFolio()
{
    QDateTime now;
    QLocale local;
    QString folio;

    local = QLocale(QLocale::Spanish, QLocale::Mexico);
    now = QDateTime::currentDateTime();

    folio = "JSMF/";
    folio += local.toString(now, "yyyy.MM.dd");
    folio += "/" + QString::number(now.date().dayOfWeek());
    folio += "." + local.toString(now, "hhmmss");

    return folio;
}

QImage MainWindow::getQrCode(QString s)
{
    QImage qrCode = QZXing::encodeData(s);
    qrCode.save("QrCode.jpg");

    return qrCode;
}

bool MainWindow::nssIsValid(QString s)
{
    QRegExp re("\\d*");

    return (re.exactMatch(s) && (s.size() == 10));
}

int MainWindow::getCheckDigit(QString s)
{
    int i;
    int tmp = 0;
    int sumatory = 0;
    int topTen = 0;
    int checkDigit = 0;

    for (i = 0; i<= 9; i++)
    {
        tmp = s.at(i).digitValue();

        if (i % 2 != 0)
        {
            tmp *= 2;

            if (tmp > 9)
                tmp -= 9;
        }

        sumatory += tmp;
    }

    topTen = ((sumatory/10) + 1) * 10;

    checkDigit = topTen - sumatory;

    if (checkDigit == 10)
        checkDigit = 0;

    return checkDigit;
}

void MainWindow::errBadNss(QString s)
{
    ui->edtOutput->setText(s + " no es un NSS válido");
    ui->lblInformation->setText("Error");
}

void MainWindow::errUnknownCommand(QString s)
{
    ui->edtOutput->setText(s + ": Comando no reconocido");
    ui->lblInformation->setText("Error");
}

void MainWindow::errWrongNumberOfArguments(QString s)
{
    ui->edtOutput->setText(s + ": Número de argumentos equivocado");
    ui->lblInformation->setText("Error");
}

void MainWindow::on_edtInput_returnPressed()
{
    QStringList commands;
    QStringList input;
    QString cmd;

    commands << "a" << "about" << "acerca";  //  0-2
    commands << "l" << "list" << "listar";   //  3-5
    commands << "d" << "digit" << "digito";  //  6-8
    commands << "c" << "clear" << "limpiar"; //  9-11
    commands << "q" << "exit" << "salir";    // 12-14
    commands << "f" << "folio";              // 15-16

    lastInput = ui->edtInput->text();
    input = lastInput.split(" ");
    cmd = input.value(0);

    ui->edtInput->clear();
    ui->edtOutput->clear();

    switch(commands.indexOf(cmd)){
      case 0 ...2:
        if (input.size() > 1)
            errWrongNumberOfArguments(cmd);
        else
            showAboutInfo();
        break;
      case 3 ... 5:
        if (input.size() > 1)
            errWrongNumberOfArguments(cmd);
        else
            showAllData();
        break;
      case 6 ... 8:
        if (input.size() != 2)
            errWrongNumberOfArguments(cmd);
        else
            showCheckDigit(input.value(1));
        break;
      case 9 ... 11:
        if (input.size() > 1)
            errWrongNumberOfArguments(cmd);
        else
            clearOutput();
        break;
      case 12 ... 14:
        if (input.size() > 1)
            errWrongNumberOfArguments(cmd);
         else
             exitApp();
        break;
      case 15 ... 16:
        if (input.size() > 1)
          errWrongNumberOfArguments(cmd);
        else
           showFolio();
        break;
      default:
        errUnknownCommand(cmd);
        break;
    }
}
