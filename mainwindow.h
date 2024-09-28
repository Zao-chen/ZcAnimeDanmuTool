#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include "identifywindow.h"
#include "ElaWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public ElaWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    identifywindow *identify_window;
    ~MainWindow();

private slots:
    void on_pushButton_main_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *m_manager;
    QString postUrl(const QString &input);

};
#endif // MAINWINDOW_H
