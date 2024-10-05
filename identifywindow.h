#ifndef IDENTIFYWINDOW_H
#define IDENTIFYWINDOW_H

#include <QWidget>
#include <QNetworkAccessManager>

namespace Ui {
class identifywindow;
}

class identifywindow : public QWidget
{
    Q_OBJECT

public:
    explicit identifywindow(QWidget *parent = nullptr);
    ~identifywindow();

private slots:

    void on_pushButton_select_file_clicked();

    void on_lineEdit_file_name_textChanged(const QString &arg1);

    void on_pushButton_next_clicked();

    void on_pushButton_back_clicked();

    void on_pushButton_change_danmu_clicked();

private:
    Ui::identifywindow *ui;
    QNetworkAccessManager *m_manager;
    QString postUrl(const QString &input);
    void loading(bool switch_load);
};

#endif // IDENTIFYWINDOW_H
