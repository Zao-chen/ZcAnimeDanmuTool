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
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*删除文件名内的多余信息*/
QString removeNonNumericBracketedContent(QString input) {
    // 匹配中括号包裹的内容，只要括号内包含非纯数字的字符就删除
    QRegularExpression re("\\[[^\\[\\]]*[^\\d\\[\\]][^\\[\\]]*\\]");
    return input.replace(re,"");
}
QString MainWindow::postUrl(const QString &input)
{
    /*使用dandanplay进行刮刮乐*/
    m_manager = new QNetworkAccessManager(this);//新建QNetworkAccessManager对象
    QString read;
    QEventLoop loop;
    QNetworkReply *reply = m_manager->get(QNetworkRequest(QUrl(input)));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    read = reply->readAll();
    reply->deleteLater();       //记得释放内存
    return read;
}
/*秒的转换*/
QString secondsToTimeString(double seconds) {

    // 提取整秒部分
    int totalSeconds = static_cast<int>(seconds);
    // 提取毫秒部分（注意，这里进行了放大以得到整数毫秒）
    int milliseconds = static_cast<int>((seconds - totalSeconds) * 1000);

    // 计算小时、分钟和剩余的秒
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    // 使用QString进行格式化
    QString timeStr = QString("%1:%2:%3:%4")
            .arg(hours, 2, 10, QChar('0'))
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(secs, 2, 10, QChar('0'))
                          .arg(milliseconds, 2, 10, QChar('0'));


    return timeStr;
}
/*点击读取*/
void MainWindow::on_pushButton_clicked()
{
    /*文件获取*/
    QString fileName = QFileDialog::getOpenFileName(this,
                                                tr("选择视频文件"),
                                                "",
                                                tr("All Files (*.*)"));
    if (!fileName.isEmpty())
    {
        /*选择文件*/
        QString fileNameOnly = QFileInfo(fileName).baseName();
        ui->plainTextEdit->appendPlainText("选择文件："+fileNameOnly);
        /*文件名处理*/
        fileNameOnly = removeNonNumericBracketedContent(fileNameOnly);
        fileNameOnly.replace("[","").replace("]","");
        fileNameOnly = fileNameOnly.trimmed();
        ui->plainTextEdit->appendPlainText("文件名处理："+fileNameOnly);
        /*获取番剧id*/
        fileNameOnly = "https://api.dandanplay.net/api/v2/search/episodes?anime=" + fileNameOnly;
        ui->plainTextEdit->appendPlainText("发送post请求："+fileNameOnly);
        fileNameOnly = postUrl(fileNameOnly);
        ui->plainTextEdit->appendPlainText("获取到番剧id值: " + fileNameOnly);
        /*提取番剧id*/
        // 将JSON字符串转换为QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(fileNameOnly.toUtf8());
        // 转换为QJsonObject
        QJsonObject obj = doc.object();
        // 从QJsonObject中获取"animes"数组
        QJsonArray animesArray = obj["animes"].toArray();
        // 遍历animes数组
        for (const QJsonValue &animeValue : animesArray)
        {
            QJsonObject animeObj = animeValue.toObject();
            // 从每个anime对象中获取"episodes"数组
            QJsonArray episodesArray = animeObj["episodes"].toArray();
            // 遍历episodes数组
            for (const QJsonValue &episodeValue : episodesArray)
            {
                QJsonObject episodeObj = episodeValue.toObject();
                // 提取episodeId
                qint64 episodeId = episodeObj["episodeId"].toInt();
                // 打印结果
                ui->plainTextEdit->appendPlainText("获取到番剧id："+QString::number(episodeId));
                fileNameOnly = postUrl("https://api.dandanplay.net/api/v2/related/"+QString::number(episodeId));
            }
        }
        ui->plainTextEdit->appendPlainText("获取到番剧检索值: " + fileNameOnly);
        /*检索网址*/
        // 将JSON字符串转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(fileNameOnly.toUtf8());
        // 获取顶层对象
        QJsonObject jsonObject = jsonDoc.object();
        // 尝试从顶层对象中获取"relateds"数组
        if (jsonObject.contains("relateds") && jsonObject["relateds"].isArray())
        {
            QJsonArray jsonArray = jsonObject["relateds"].toArray();
            // 遍历数组中的每个元素
            for (int i = 0; i < jsonArray.size(); ++i)
            {
                QJsonObject item = jsonArray.at(i).toObject();
                // 尝试从每个元素中获取"url"字符串
                if (item.contains("url") && item["url"].isString())
                {
                    QString url = item["url"].toString();
                    ui->plainTextEdit->appendPlainText("获取到番剧网址: " + url);
                    ui->plainTextEdit->appendPlainText("发送获取弹幕请求: " + url);
                    fileNameOnly = postUrl("https://api.dandanplay.net/api/v2/extcomment?url="+url);
                    ui->plainTextEdit->appendPlainText("获取到弹幕: " + fileNameOnly);

                    QJsonDocument doc = QJsonDocument::fromJson(fileNameOnly.toUtf8());
                    // 获取顶层对象
                    QJsonObject topLevelObj = doc.object();
                    // 获取"comments"数组
                    QJsonArray commentsArray = topLevelObj["comments"].toArray();
                    // 遍历数组
                    int x=0;
                    for (int i = 0; i < commentsArray.size(); ++i) {
                        QJsonObject commentObj = commentsArray[i].toObject();
                        // 提取"p"和"m"
                        if (commentObj.contains("p") && commentObj.contains("m")) {
                            QString p = commentObj["p"].toString();
                            double pl[3];
                            for(int i=0;i!=3;i++)
                            {
                                pl[i] = p.split(",")[i].toDouble();
                            }
                            QString m = commentObj["m"].toString();
                            ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+secondsToTimeString(pl[0]).replace(".",":")+","+secondsToTimeString(pl[0]+6).replace(".",":")+",R2L,,20,20,2,,{\\move(585,"+QString::number(x*20)+",-25,"+QString::number(x*20)+")}"+m.replace(".","。"));
                            x++;
                            if(x==7) x=0;
                        }
                    }
                }
            }
        }
        QFile file(QFileInfo(fileName).absolutePath()+"\\"+QFileInfo(fileName).baseName()+".ass");
        ui->plainTextEdit->appendPlainText("写入文件: " +QFileInfo(fileName).absolutePath()+"\\"+QFileInfo(fileName).baseName()+".ass");
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "无法打开文件进行写入:" << file.errorString();
        return;
        }
        QTextStream out(&file);
        out << ui->plainTextEdit_2->toPlainText();
        file.close();

        QUrl url = QUrl::fromLocalFile(QFileInfo(fileName).filePath());
        QDesktopServices::openUrl(url);
    }
}
