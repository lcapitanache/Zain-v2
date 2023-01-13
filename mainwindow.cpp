#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QZXing.h"

#include <QMessageBox>
#include <QDebug>
#include <QStringList>
#include <QDateTime>
#include <QClipboard>


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
    QString name;
    QString year;

    year = QDateTime::currentDateTime().toString("yyyy");

    name = "<html><b>Zaín v2</b></html>";

    about = "Sistema de captura de incapacidades\n\n" \
            "Instituto Mexicano del Seguro Social\n" \
            "Unidad de Medicina Familiar N.° 36\n\n" \
            "© " + year + " Luis Capitanache\n" \
            "lcapitanache@gmail.com\n" \
            "https://lcapitanache.github.io/\n\n" \
            "Qt " + QT_VERSION_STR + " | MinGW 53 | x86 | 32-bits";

    ui->edtOutput->setText(name);
    ui->edtOutput->append(about);

    ui->lblInformation->setText("Acerca de Zaín v2");

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::showAllData()
{
    QSqlQuery query;
    QString data;
    QTextCursor cursor;

    int i = 0;

    db.open();

    query.prepare("SELECT * FROM incapacidades ORDER BY Tipo, NSS");
    query.exec();

    ui->edtOutput->clear();

    while(query.next())
    {
        i++;

        data = query.value(1).toString() + "   " + \
               query.value(2).toString() + "   " + \
               query.value(3).toString() + "   " + \
               query.value(4).toString() + "   " + \
               query.value(5).toString() + "\t" + \
               query.value(8).toString() + "   " + \
               query.value(9).toString();

        ui->edtOutput->append(data);
    }

    cursor = ui->edtOutput->textCursor();
    cursor.setPosition(0);
    ui->edtOutput->setTextCursor(cursor);

    ui->lblInformation->setText(QString::number(i) + " registros");

    query.clear();
    db.close();

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::showCheckDigit(QString nss)
{
    if (nssIsValid(nss))
    {
        QString message;

        message = "Cálculo de dígito verificador\n\n" \
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

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::showFolio()
{
    QString folio;
    QString message;

    ui->stackedWidget->setCurrentIndex(1);

    folio = getFolio();

    message = folio;

    ui->lblFolio->setText(folio);
    ui->lblInformation->setText("Listo");

    QPixmap QrCode(getQrCode(folio));

    ui->lblQrCode->setPixmap(QrCode);
    ui->lblQrCode->setScaledContents(true);
    ui->lblQrCode->show();
}

QString MainWindow::getFolio()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QDateTime now;
    QLocale local;
    QString folio;

    local = QLocale(QLocale::Spanish, QLocale::Mexico);
    now = QDateTime::currentDateTime();

    folio = "JSMF/" + local.toString(now, "yyyy.MM.dd") + "/"
            + QString::number(now.date().dayOfWeek()) + "."
            + local.toString(now, "hhmmss");

    clipboard->setText(folio);

    return folio;
}

QString MainWindow::getQrCode(QString folio)
{
    QImage qrCode;
    qrCode = QZXing::encodeData(folio);

    folio.replace(QString("/"), QString("-"));
    qrCode.save(folio + ".jpg");

    return folio + ".jpg";
}

bool MainWindow::nssIsValid(QString nss)
{
    QRegExp re("\\d*");

    return (re.exactMatch(nss) && (nss.size() == 10));
}

int MainWindow::getCheckDigit(QString nss)
{
    int i;
    int tmp = 0;
    int sumatory = 0;
    int topTen = 0;
    int checkDigit = 0;

    for (i = 0; i<= 9; i++)
    {
        tmp = nss.at(i).digitValue();

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

    lastInput = ui->edtInput->text().simplified().toLower();

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
        ui->stackedWidget->setCurrentIndex(0);

        return;
    }

    int expectedNumArgs = numArgs[commands[cmd]];

    if (input.size() - 1 != expectedNumArgs) {
        ui->edtOutput->clear();
        ui->edtOutput->setText(cmd + ": Número de argumentos no válido");
        ui->lblInformation->setText("Error");
        ui->stackedWidget->setCurrentIndex(0);

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
