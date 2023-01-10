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

    QString data = "Oficio de incapacidades iniciales " \
                   "folio JSMF/2023.01.08/7.212247. " \
                   "Creado por Luis Capitanache el día " \
                   "domingo, 08 de enero de 2023 a las 21:22:47.";
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

void MainWindow::showCheckDigit(QString nss)
{
    if (nssIsValid(nss))
    {
        QString message;

        message = "Cálculo de dígito verificador\n" \
                  "-----------------------------\n\n" \
                  "NSS:\t" + nss + "\n" \
                  "Dígito:\t" + QString::number(getCheckDigit(nss));

        ui->edtOutput->setText(message);
        ui->lblInformation->setText("Listo");
    }
    else
    {
        ui->edtOutput->clear();
        ui->edtOutput->setText(nss + ": No es un NSS válido");
        ui->lblInformation->setText("Error");
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

void MainWindow::on_edtInput_returnPressed()
{
    QMap <QString, int> commands = {
            {"a", 0},   {"about", 0},   {"acerca", 0},
            {"l", 1},   {"ls", 1},      {"list", 1},    {"listar", 1},
            {"d", 2},   {"digit", 2},   {"digito", 2},
            {"c", 3},   {"cls", 3},     {"clear", 3},   {"limpiar", 3},
            {"q", 4},   {"quit", 4},    {"exit", 4},    {"salir", 4},
            {"f", 5},   {"folio", 5}
        };

    const int numArgs[] = {0, 0, 1, 0, 0, 0};

    QStringList input;
    QString cmd;

    lastInput = ui->edtInput->text().simplified();

    if (lastInput.length() == 0)
    {
        ui->edtInput->clear();
        return;
    }

    input = lastInput.split(" ");
    cmd = input.value(0);

    ui->edtInput->clear();
    ui->edtOutput->clear();

    if (commands.count(cmd) == 0) {
        ui->edtOutput->clear();
        ui->edtOutput->setText(cmd + ": Comando no reconocido");
        ui->lblInformation->setText("Error");
        return;
    }

    int expectedNumArgs = numArgs[commands[cmd]];

    if (input.size() - 1 != expectedNumArgs) {
        ui->edtOutput->clear();
        ui->edtOutput->setText(cmd + ": Número de argumentos no válido");
        ui->lblInformation->setText("Error");
        return;
    }

    switch(commands[cmd]) {
        case 0: showAboutInfo(); break;
        case 1: showAllData(); break;
        case 2: showCheckDigit(input.value(1)); break;
        case 3: clearOutput(); break;
        case 4: exitApp(); break;
        case 5: showFolio(); break;
    }
}
