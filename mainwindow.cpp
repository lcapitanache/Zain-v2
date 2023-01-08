#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QStringList>

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

    getAllData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getAllData()
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

void MainWindow::getAboutInfo()
{
    QString about;

    about = "Zaín mark 2\n"
            "Sistema de captura de incapacidades\n\n" \
            "Instituto Mexicano del Seguro Social\n" \
            "Unidad de Medicina Familiar N.° 36\n\n" \
            "© 2023 Luis Capitanache\n" \
            "lcapitanache@gmail.com\n" \
            "https://lcapitanache.github.io/\n\n" \
            "Qt 5.9.9 | MinGW 32 bits";

    ui->edtOutput->setText(about);
    ui->lblInformation->setText("Acerca de Zaín v2");
}

void MainWindow::unknownCommand(QString s)
{
    ui->edtOutput->setText("Comando no reconocido: " + s);
    ui->lblInformation->setText("Error");
}

void MainWindow::wrongNumberOfArguments()
{
    ui->edtOutput->setText("Número de argumentos equivocado");
    ui->lblInformation->setText("Error");
}

void MainWindow::on_edtInput_returnPressed()
{
    QStringList options;
    QStringList input;

    options << "a" << "about" << "acerca";
    options << "l" << "list" << "listar";

    input = ui->edtInput->text().split(" ");

    ui->edtInput->clear();
    ui->edtOutput->clear();

    switch(options.indexOf(input.value(0))){
      case 0 ...2:
        if (input.size() > 1)
            wrongNumberOfArguments();
        else
            getAboutInfo();

        break;
      case 3 ... 5:
        if (input.size() > 1)
            wrongNumberOfArguments();
        else
            getAllData();
        break;
      default:
        unknownCommand(input.value(0));
        break;
    }
}
