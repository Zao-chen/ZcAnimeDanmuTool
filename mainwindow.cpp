#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : ElaWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    identify_window = new identifywindow(ui->widget);

    QHBoxLayout *layout = new QHBoxLayout(this);
    ui->widget->setLayout(layout);
    ui->widget->layout()->addWidget(identify_window);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_main_clicked()
{
    qDebug()<<"show";
    identify_window->show();
}

