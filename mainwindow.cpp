#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QZXing.h"
#include "alignedSqlQueryModel.h"

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>


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

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F1)
    {
        qDebug() << "F1 - Ayuda";
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
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

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::showAboutInfo()
{    
    QString name = "<html><b>Zaín v2</b></html>";
    QString year = QDateTime::currentDateTime().toString("yyyy");

    QString about = "Sistema de captura de incapacidades\n\n" \
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

void MainWindow::resizeColumnsAndRows()
{
    for (int i = 0; i <= model->columnCount(); i++)
    {
        ui->tblDataOutput->resizeColumnToContents(i);
        ui->tblDataOutput->setColumnWidth(i, ui->tblDataOutput->columnWidth(i) + 20);
    }

    for (int i = 0; i <= model->rowCount(); i++)
        ui->tblDataOutput->resizeRowToContents(i);
}

void MainWindow::showAllData()
{
    db.open();

    QSqlQuery *query = new QSqlQuery(db.databaseName());
    query->prepare("SELECT * FROM incapacidades ORDER BY nss");
    query->exec();

    model = new AlignedSqlQueryModel(this);
    model->setQuery(*query);

    query->clear();
    db.close();

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(model);
    proxy->setSourceModel(model);

    ui->tblDataOutput->setSortingEnabled(true);
    ui->tblDataOutput->setModel(proxy);
    ui->tblDataOutput->hideColumn(0);

    resizeColumnsAndRows();

    ui->lblInformation->setText(QString::number(model->rowCount()) + " registros");
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::showCheckDigit(QString nss)
{
    if (nssIsValid(nss))
    {        
        QString digit = QString::number(getCheckDigit(nss));

        nss.insert(4, "-");
        nss.insert(7, "-");

        QString message = "Cálculo de dígito verificador\n\n" \
                          "NSS:\t" + nss + "\n" \
                          "Dígito:\t" + digit;

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
    QString folio = getFolio();

    QPixmap QrCode(getQrCode(folio));
    ui->lblQrCode->setPixmap(QrCode);
    ui->lblQrCode->setScaledContents(true);

    ui->lblFolio->setText(folio);

    ui->lblInformation->setText("Listo");
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::showManual(QString cmd)
{
    QFile file(":help/" + cmd + ".md");
    QString manual;
    QString info;

    if (!file.exists())
    {
        manual = "man: Sin entrada en el manual para <" + cmd + ">";
        info = "Error";
    }
    else
    {
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");

        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            stream.setCodec(codec);
            manual = stream.readAll();
            file.close();

            info = "Manual del comando <" + cmd + ">";
        }
    }    

    ui->edtOutput->setPlainText(manual);
    ui->lblInformation->setText(info);
    ui->stackedWidget->setCurrentIndex(0);
}

QString MainWindow::getFolio()
{    
    QDateTime now = QDateTime::currentDateTime();
    QLocale local = QLocale(QLocale::Spanish, QLocale::Mexico);

    QString folio = "JSMF/" + local.toString(now, "yyyy.MM.dd") + "/"
                    + QString::number(now.date().dayOfWeek()) + "."
                    + local.toString(now, "hhmmss");

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(folio);

    return folio;
}

QString MainWindow::getQrCode(QString folio)
{
    QImage qrCode = QZXing::encodeData(folio);

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
    int tmp = 0;
    int sumatory = 0;
    int topTen = 0;
    int checkDigit = 0;

    for (int i = 0; i<= 9; i++)
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
            {"d", 2},   {"digit", 2},   {"dígito", 2},
            {"c", 3},   {"cls", 3},     {"clear", 3},   {"limpiar", 3},
            {"q", 4},   {"quit", 4},    {"exit", 4},    {"salir", 4},
            {"f", 5},   {"folio", 5},
            {"man", 6}, {"help", 6},            
        };

    const int numArgs[] = {0, 0, 1, 0, 0, 0, 1, 0};

    lastInput = ui->edtInput->text().simplified().toLower();

    if (lastInput.length() == 0)
    {
        ui->edtInput->clear();
        return;
    }

    QStringList input = lastInput.split(" ");
    QString cmd = input.value(0);

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
        case 6: showManual(input.value(1)); break;        
    }
}
