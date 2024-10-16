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
#include <QSettings>

identifywindow::identifywindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::identifywindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    ui->progressBar->setVisible(0);

    QSettings *settings = new QSettings("Setting.ini",QSettings::IniFormat);
    ui->lineEdit_danmu_speed->setText(settings->value("GlobelSetting/Danmu_stay_time").toString());
    ui->lineEdit_size->setText(settings->value("GlobelSetting/Danmu_size").toString());
    ui->lineEdit_line->setText(settings->value("GlobelSetting/Danmu_line").toString());
}

identifywindow::~identifywindow()
{
    delete ui;
}
QString fileName;
QString fileNameOnly;
int episodeId;
QList<int> Danmu_time;
QStringList Danmu_msg;


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
    loading(true);
    ui->pushButton_next->setEnabled(0);
    ui->pushButton_back->setEnabled(1);
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
    {
        ui->label_ep_id->setText("Loading...");
        ui->label_ep_name->setText("Loading...");
        ui->label_anime_name->setText("Loading...");
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
        bool find_anime = false;
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
                find_anime = true;
                loading(false);
            }
        }
        if(!find_anime)
        {
            ui->label_anime_name->setText("未找到");
            ui->label_ep_id->setText("未找到");
            ui->label_ep_name->setText("未找到");
            ui->pushButton_next->setEnabled(0);
            ui->pushButton_back->setEnabled(1);
            loading(false);
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
            Danmu_time.clear();
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
                            Danmu_time.append(pl[0]);
                            Danmu_msg.append(m);
                        }
                    }



                    QMetaObject::invokeMethod(ui->pushButton_change_danmu, "clicked", Qt::QueuedConnection);
                }
            }
        }
        ui->pushButton_next->setEnabled(1);
        loading(false);
        break;
    }
    case 2:
    {
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
        loading(false);
        break;
    }
        break;
    default :
        loading(false);
        break;
    }
    change_page(ui->stackedWidget->currentIndex());
}
/*返回上一级*/
void identifywindow::on_pushButton_back_clicked()
{
    loading(true);
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
        break;
    case 1:
        ui->stackedWidget->setCurrentIndex(0);
        ui->pushButton_next->setEnabled(1);
        ui->pushButton_back->setEnabled(0);
        loading(false);
        break;
    case 2:
    {
        ui->stackedWidget->setCurrentIndex(1);
        loading(false);
        break;
    }
    default :
        //***
        break;
    }
    change_page(ui->stackedWidget->currentIndex());

}
/*弹幕样式修改*/
void identifywindow::on_pushButton_change_danmu_clicked()
{
    loading(1);

    ui->plainTextEdit_2->clear();
    ui->plainTextEdit_2->setPlainText("[Script Info]\n"
                                      "Title: Zaochen\n"
                                      "Original Script: \n"
                                      "ScriptType: v4.00+\n"
                                      "Collisions: Normal\n"
                                      "PlayResX: 560\n"
                                      "PlayResY: 420\n"
                                      "Timer: 10.0000\n"
                                      "[V4+ Styles]\n"
                                      "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
                                      "Style: nurmal,Microsoft YaHei UI,"+ui->lineEdit_size->text()+",&H66FFFFFF,&H66FFFFFF,&H66000000,&H66000000,1,0,0,0,100,100,0,0,1,2,0,2,20,20,2,0\n"
                                      "Style: big,Microsoft YaHei UI,"+QString::number(ui->lineEdit_size->text().toInt()+3)+",&H66FFFFFF,&H66FFFFFF,&H66000000,&H66000000,1,0,0,0,100,100,0,0,1,2,0,2,20,20,2,0"
                                      "\n\n[Events]\n"
                                      "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text");
    int x=0;
    int repeat=0;
    for(int i=0;i!=Danmu_time.size();i++)
    {
        x++;
        repeat = 0;
        while(1)
        {
             if(Danmu_msg[i]!=Danmu_msg[i+1]) break;
             else repeat++;
             i++;
        }
        if(repeat==0)
        {
            ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+
                                            secondsToTimeString(Danmu_time[i]).replace(".",":")
                                            +","+
                                            secondsToTimeString(Danmu_time[i]+ui->lineEdit_danmu_speed->text().toInt()).replace(".",":")
                                            +",nurmal,,20,20,2,,{\\move(585,"+
                                            QString::number(ui->lineEdit_line->text().toInt()*x)
                                            +",-25,"+
                                            QString::number(ui->lineEdit_line->text().toInt()*x)
                                            +")}"+
                                            Danmu_msg[i].replace(".","。"));
        }
        else
        {
            ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+
                                                 secondsToTimeString(Danmu_time[i]).replace(".",":")
                                                 +","+
                                                 secondsToTimeString(Danmu_time[i]+ui->lineEdit_danmu_speed->text().toInt()).replace(".",":")
                                                 +",big,,20,20,2,,{\\move(585,"+
                                                 QString::number(ui->lineEdit_line->text().toInt()*x)
                                                 +",-25,"+
                                                 QString::number(ui->lineEdit_line->text().toInt()*x)
                                                 +")}"+
                                                 Danmu_msg[i].replace(".","。")+"×"+QString::number(repeat+1));
        }
        if(x==7) x=0;
    }
    QSettings *settings1 = new QSettings("Setting.ini",QSettings::IniFormat);
    //配置相关
    settings1->beginGroup("GlobelSetting");
    settings1->setValue("Danmu_stay_time",ui->lineEdit_danmu_speed->text().toInt());
    settings1->setValue("Danmu_size",ui->lineEdit_size->text().toInt());
    settings1->setValue("Danmu_line",ui->lineEdit_line->text().toInt());
    settings1->endGroup();
    delete settings1;
    settings1 =nullptr;
    loading(0);
}
/*加载中*/
void identifywindow::loading(bool switch_load)
{
    if(switch_load)
    {
        this->setEnabled(0);
        ui->progressBar->setVisible(1);
    }
    else
    {
        this->setEnabled(1);
        ui->progressBar->setVisible(0);
    }
}
void identifywindow::change_page(int i)
{
    QFont f_b;
    f_b.setBold(1);
    QFont f_nb;
    f_nb.setBold(0);
    switch (i) {
    case 0:
    {
        ui->label_step_0->setFont(f_b);
        ui->label_step_1->setFont(f_nb);
        ui->label_step_2->setFont(f_nb);
        ui->label_step_3->setFont(f_nb);
        break;
    }
    case 1:
    {
        ui->label_step_0->setFont(f_nb);
        ui->label_step_1->setFont(f_b);
        ui->label_step_2->setFont(f_nb);
        ui->label_step_3->setFont(f_nb);
        break;
    }
    case 2:
    {
        ui->label_step_0->setFont(f_nb);
        ui->label_step_1->setFont(f_nb);
        ui->label_step_2->setFont(f_b);
        ui->label_step_3->setFont(f_nb);
        break;
    }
    default :
        break;
    }
    qDebug()<<i;
}
