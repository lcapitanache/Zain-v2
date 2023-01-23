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


/********************************************************************
 *
 * Main Window
 *
 * ******************************************************************
 */

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

    setCurrentLineInHistory();
}

MainWindow::~MainWindow()
{
    delete ui;
}


/********************************************************************
 *
 * Historial de comandos
 *
 * ******************************************************************
 */

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->edtInput->hasFocus())
    {
        if (event->key() == Qt::Key_Up)
        {
            if (currentLineInHistory == 1) { return; }

            if (currentLineInHistory > 0)
            {
                currentLineInHistory--;
                readFromHistory();
            }
        }
        else if (event->key() == Qt::Key_Down)
        {
            if (currentLineInHistory == getHistoryTotalLines() - 1)
            {
                currentLineInHistory++;
                ui->edtInput->clear();
                return;
            }

            if (currentLineInHistory < getHistoryTotalLines() - 1)
            {
                currentLineInHistory++;
                readFromHistory();
            }
        }
        else
        {
            QWidget::keyPressEvent(event);
        }
    }

    if (event->key() == Qt::Key_F1)
    {
        showHelp();
    }
}

void MainWindow::addToHistory(QString line)
{
    QFile file("history");

    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream in(&file);
        in << endl << line;
        file.close();
    }
}

int MainWindow::getHistoryTotalLines()
{
    int count = 0;
    QFile file("history");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);

        while (!in.atEnd())
        {
            in.readLine();
            count++;
        }

        file.close();
    }

    return count;
}

void MainWindow::readFromHistory()
{
    QFile file("history");
    QString line = "";

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString allText = in.readAll();
        QStringList lines = allText.split("\n");

        line = lines[currentLineInHistory];
        ui->edtInput->setText(line);
        file.close();
    }
}

void MainWindow::setCurrentLineInHistory()
{
    currentLineInHistory = getHistoryTotalLines();
}

void MainWindow::showHistory()
{
    QFile file("history");
    QString history;
    QString info;

    if (!file.exists())
    {
        history = "err: Historial de comandos no encontrado.";
        info = "Error";
    }
    else
    {
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");

        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            in.setCodec(codec);
            history = in.readAll();
            file.close();

            info = "Historial de comandos";
        }
    }

    ui->edtOutput->setText(history);
    ui->lblInformation->setText(info);
    ui->stackedWidget->setCurrentIndex(0);
}


/********************************************************************
 *
 * Entrada de comandos
 *
 * ******************************************************************
 */

void MainWindow::on_edtInput_returnPressed()
{
    lastInput = ui->edtInput->text().simplified().toLower();
    ui->edtInput->clear();

    if (lastInput.length() == 0) { return; }

    addToHistory(lastInput);    
    setCurrentLineInHistory();

    checkCommand();    
}

void MainWindow::checkCommand()
{
    QMap <QString, int> commands = {
            {"uname", 0},      {"itt", 1},
            {"dof", 2},        {"cls", 3},
            {"quit", 4},       {"fol", 5},
            {"man", 6},        {"history", 7}
        };

    const int numArgs[] = {0, 0, 1, 0, 0, 0, 1, 0};

    QStringList input = lastInput.split(" ");
    QString cmd = input.value(0);

    if (!commandIsOK(commands, cmd))
        return;

    int argumentsRequired = numArgs[commands[cmd]];
    int argumentsPassed = input.size() - 1;

    if (!numOfArgsIsOK(cmd, argumentsRequired, argumentsPassed))
        return;

    QString argument = input.value(1);
    executeCommand(commands[cmd], argument);
}

bool MainWindow::commandIsOK(QMap <QString, int> commands, QString cmd)
{
    if (commands.count(cmd) == 0)
    {
        ui->edtOutput->clear();
        ui->edtOutput->setText(cmd + ": Comando no reconocido");
        ui->lblInformation->setText("Error");
        ui->stackedWidget->setCurrentIndex(0);

        return false;
    }

    return true;
}

bool MainWindow::numOfArgsIsOK(QString cmd, int expectedNumArgs, int numArgsPassed)
{
    if (numArgsPassed != expectedNumArgs)
    {
        ui->edtOutput->clear();
        ui->edtOutput->setText(cmd + ": Número de argumentos no válido");
        ui->lblInformation->setText("Error");
        ui->stackedWidget->setCurrentIndex(0);

        return false;
    }
    return true;
}

void MainWindow::executeCommand(int cmd, QString argument)
{
    switch(cmd)
    {
        case 0: showAboutInfo(); break;
        case 1: showAllData(); break;
        case 2: showCheckDigit(argument); break;
        case 3: clearOutput(); break;
        case 4: exitApp(); break;
        case 5: showFolio(); break;
        case 6: showManual(argument); break;
        case 7: showHistory(); break;
    }
}


/********************************************************************
 *
 * Comandos
 *
 * ******************************************************************
 */


// - - - - - - - - -
// uname
// - - - - - - - - -

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


// - - - - - - - - -
// itt
// - - - - - - - - -

void MainWindow::showAllData()
{
    db.open();

    QSqlQuery *query = new QSqlQuery(db.databaseName());
    query->prepare("SELECT * FROM incapacidades");

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


// - - - - - - - - -
// dof
// - - - - - - - - -

void MainWindow::showCheckDigit(QString nss)
{
    if (nssIsOK(nss))
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

bool MainWindow::nssIsOK(QString nss)
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


// - - - - - - - - -
// clear
// - - - - - - - - -

void MainWindow::clearOutput()
{
    ui->edtOutput->clear();
    ui->lblInformation->clear();

    ui->stackedWidget->setCurrentIndex(0);
}


// - - - - - - - - -
// quit
// - - - - - - - - -

void MainWindow::exitApp()
{
    if (db.open())
        db.close();

    QApplication::quit();
}


// - - - - - - - - -
// fol
// - - - - - - - - -

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


// - - - - - - - - -
// man
// - - - - - - - - -

void MainWindow::showManual(QString cmd)
{
    QFile file(":help/" + cmd);
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
            QTextStream in(&file);
            in.setCodec(codec);
            manual = in.readAll();
            file.close();

            info = "Manual del comando <" + cmd + ">";
        }
    }

    ui->edtOutput->setHtml(manual);
    ui->lblInformation->setText(info);
    ui->stackedWidget->setCurrentIndex(0);
}


/********************************************************************
 *
 * Ayuda
 *
 * ******************************************************************
 */

void MainWindow::showHelp()
{
    QFile file(":help/help");
    QString ayuda;
    QString info;

    if (!file.exists())
    {
        ayuda = "err: Archivo de ayuda no encontrado.";
        info = "Error";
    }
    else
    {
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");

        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            in.setCodec(codec);
            ayuda = in.readAll();
            file.close();

            info = "Ayuda";
        }
    }

    ui->edtOutput->setHtml(ayuda);
    ui->lblInformation->setText(info);
    ui->stackedWidget->setCurrentIndex(0);
}
