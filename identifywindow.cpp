#include "identifywindow.h"
#include "ui_identifywindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>

identifywindow::identifywindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::identifywindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
}

identifywindow::~identifywindow()
{
    delete ui;
}
QString fileName;
QString fileNameOnly;
int episodeId;
QStringList Danmu;


/*删除文件名内的多余信息*/
QString removeNonNumericBracketedContent(QString input) {
    // 匹配中括号包裹的内容，只要括号内包含非纯数字的字符就删除
    QRegularExpression re("\\[[^\\[\\]]*[^\\d\\[\\]][^\\[\\]]*\\]");
    return input.replace(re,"");
}
/*Post请求*/
QString identifywindow::postUrl(const QString &input)
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
/*选择文件*/
void identifywindow::on_pushButton_select_file_clicked()
{
    /*文件获取*/
    fileName = QFileDialog::getOpenFileName(this,
                                            tr("选择视频文件"),
                                            "",
                                            tr("All Files (*.*)"));
    /*选择文件*/
    fileNameOnly = QFileInfo(fileName).baseName();
    /*文件名处理*/
    fileNameOnly = removeNonNumericBracketedContent(fileNameOnly);
    fileNameOnly.replace("[","").replace("]","");
    fileNameOnly = fileNameOnly.trimmed();
    ui->label_file_source_name->setText(fileName);
    ui->lineEdit_file_name->setText(fileNameOnly);
    if(!fileNameOnly.isEmpty())
    {
        ui->lineEdit_file_name->setEnabled(1);
        ui->pushButton_next->setEnabled(1);
    }
    else
    {
        ui->lineEdit_file_name->setEnabled(0);
        ui->pushButton_next->setEnabled(0);
    }
}
/*手动修改名词*/
void identifywindow::on_lineEdit_file_name_textChanged(const QString &arg1)
{
    fileNameOnly = arg1;
}
/*下一步*/
void identifywindow::on_pushButton_next_clicked()
{
    ui->pushButton_next->setEnabled(0);
    ui->pushButton_back->setEnabled(1);
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
    {
        ui->stackedWidget->setCurrentIndex(1);
        /*获取番剧id*/
        QString fileNameEp = postUrl("https://api.dandanplay.net/api/v2/search/episodes?anime=" +fileNameOnly);
        /*提取番剧id*/
        // 将JSON字符串转换为QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(fileNameEp.toUtf8());
        // 转换为QJsonObject
        QJsonObject obj = doc.object();
        // 从QJsonObject中获取"animes"数组
        QJsonArray animesArray = obj["animes"].toArray();
        // 遍历animes数组
        for (const QJsonValue &animeValue : animesArray)
        {
            QJsonObject animeObj = animeValue.toObject();
            ui->label_anime_name->setText(animeObj["animeTitle"].toString());
            // 从每个anime对象中获取"episodes"数组
            QJsonArray episodesArray = animeObj["episodes"].toArray();
            // 遍历episodes数组
            for (const QJsonValue &episodeValue : episodesArray)
            {
                QJsonObject episodeObj = episodeValue.toObject();
                // 提取episodeId
                episodeId = episodeObj["episodeId"].toInt();
                // 打印结果
                ui->label_ep_id->setText(QString::number(episodeId));
                ui->label_ep_name->setText(episodeObj["episodeTitle"].toString());
                ui->pushButton_next->setEnabled(1);
                ui->pushButton_back->setEnabled(1);
            }
        }
    }
        break;
    case 1:
    {
        ui->stackedWidget->setCurrentIndex(2);
        QString fileNameRelated = postUrl("https://api.dandanplay.net/api/v2/related/"+QString::number(episodeId));
        /*检索网址*/
        // 将JSON字符串转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(fileNameRelated.toUtf8());
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
                    fileNameRelated = postUrl("https://api.dandanplay.net/api/v2/extcomment?url="+url);
                    QJsonDocument doc = QJsonDocument::fromJson(fileNameRelated.toUtf8());
                    // 获取顶层对象
                    QJsonObject topLevelObj = doc.object();
                    // 获取"comments"数组
                    QJsonArray commentsArray = topLevelObj["comments"].toArray();
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
                            Danmu.append(secondsToTimeString(pl[0])+"|"+m);
                            ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+secondsToTimeString(pl[0]).replace(".",":")+","+secondsToTimeString(pl[0]+6).replace(".",":")+",R2L,,20,20,2,,{\\move(585,"+QString::number(x*20)+",-25,"+QString::number(x*20)+")}"+m.replace(".","。"));
                            x++;
                            if(x==7) x=0;
                        }
                    }
                }
            }
        }
        ui->pushButton_next->setEnabled(1);
        break;
    }
    case 2:
    {
        qDebug()<<"1111";
        QFile file(QFileInfo(fileName).absolutePath()+"\\"+QFileInfo(fileName).baseName()+".ass");
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
        break;
    default :
        //***
        break;
    }
}


void identifywindow::on_pushButton_back_clicked()
{
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
        break;
    case 1:
        ui->stackedWidget->setCurrentIndex(0);
        ui->pushButton_next->setEnabled(1);
        ui->pushButton_back->setEnabled(0);
        break;
    case 2:
    {
        ui->stackedWidget->setCurrentIndex(1);
    }
    default :
        //***
        break;
    }
}




void identifywindow::on_pushButton_change_danmu_clicked()
{

    ui->plainTextEdit_2->clear();
    ui->plainTextEdit_2->setPlainText("[Script Info]\nTitle: Zaochen\nOriginal Script: \nScriptType: v4.00+\nCollisions: Normal\nPlayResX: 560\nPlayResY: 420\nTimer: 10.0000\n\n[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\nStyle: Fix,Microsoft YaHei UI,20,&H66FFFFFF,&H66FFFFFF,&H66000000,&H66000000,1,0,0,0,100,100,0,0,1,2,0,2,20,20,2,0\nStyle: R2L,Microsoft YaHei UI,20,&H66FFFFFF,&H66FFFFFF,&H66000000,&H66000000,1,0,0,0,100,100,0,0,1,2,0,2,20,20,2,0\n\n[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text");
    int x;
    for(QString danmu1 : Danmu)
    {
        x++;
        ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+danmu1.split("|")[0].replace(".",":")
                                                +","+
                                                 QString::number(danmu1.split("|")[0].toInt()+6).replace(".",":")
                                                +",R2L,,20,20,2,,{\\move(585,"+QString::number(x*20)+",-25,"+QString::number(x*20)+")}"+danmu1.split("|")[1].replace(".","。"));
        if(x==7) x=0;
    }
}


