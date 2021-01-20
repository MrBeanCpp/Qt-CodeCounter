#include "widget.h"
#include "codecounter.h"
#include "setwindowblur.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsBlurEffect>
#include <QInputDialog>
#include <QKeyEvent>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QStyle>
#include <QTime>
#include <QTimeLine>
#include <QTimer>
#include <Qtwin>
#include <windows.h>
Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    //setWindowOpacity(0.9);

    setWindowFlags(Qt::FramelessWindowHint); //与Qt::WA_NoSystemBackground一起用 否则Windows报错
    setAttribute(Qt::WA_NoSystemBackground); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    setAttribute(Qt::WA_TranslucentBackground); //加上这句使得客户区也生效Blur
    setWindowBlur(HWND(winId())); //磨砂效果

    ui->label_title->setText("CodeCounter v2.5 [by MrBeanC]");
    setFixedSize(QSize(Normal_W, height()));
    initPosition();
    ui->listWidget->installEventFilter(this);

    ui->btn_openDir->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->btn_openFile->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->btn_run->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->btn_clear->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    QGraphicsBlurEffect* effect = new QGraphicsBlurEffect(this);
    effect->setBlurRadius(1.5);
    ui->label_nowFile->setGraphicsEffect(effect); //胡乱试水

    connect(ui->btn_openDir, &QToolButton::clicked, [=]() {
        QString dirName = QFileDialog::getExistingDirectory(this, "Choose a Folder");
        if (!isRepeat(dirName)) //去重
            addCheckBoxToListWidget(ui->listWidget, dirName);
        QTimer::singleShot(100, [=]() { writePath(); });
    });
    connect(ui->btn_openFile, &QToolButton::clicked, [=]() {
        QStringList fileList = QFileDialog::getOpenFileNames(this, "Choose Files");
        for (const QString& fileName : fileList)
            if (!isRepeat(fileName)) //去重
                addCheckBoxToListWidget(ui->listWidget, fileName);
        QTimer::singleShot(100, [=]() { writePath(); });
    });
    connect(ui->btn_clear, &QToolButton::clicked, [=]() {
        ui->listWidget->clear();
        QTimer::singleShot(100, [=]() { writePath(); }); //防止阻塞UI
    });
    connect(ui->btn_run, &QToolButton::clicked, this, &Widget::analyzeCode);
    connect(ui->btn_run, &QToolButton::clicked, this, &Widget::writeSetting);

    QTimeLine* timeLine = new QTimeLine(150, this); //伸缩动画//动画老祖，比QAnimation类好用多了
    timeLine->setUpdateInterval(10); //default 40ms 25fps
    connect(timeLine, &QTimeLine::frameChanged, this, &Widget::setFixedWidth);
    connect(timeLine, &QTimeLine::finished, [=]() {
        QTimer::singleShot(10, [=]() { writeSetting(); }); //防止阻塞最后一帧
    });
    connect(ui->checkBox_setting, &QCheckBox::clicked, [=](bool checked) { //hhh
        timeLine->stop(); //stop whenever click
        if (checked) {
            //setFixedWidth(Normal_W + Extra_W);
            timeLine->setFrameRange(width(), Normal_W + Extra_W);
        } else {
            //setFixedWidth(Normal_W);
            timeLine->setFrameRange(width(), Normal_W);
            //QTimer::singleShot(100, [=]() { writeSetting(); });//I/O会阻塞动画，移至finished↑
        }
        timeLine->start();
    });

    connect(ui->btn_min, &QPushButton::clicked, this, &Widget::showMinimized);
    connect(ui->btn_close, &QPushButton::clicked, this, &Widget::close);

    connect(ui->lineEdit_suffix, &QLineEdit::returnPressed, [=]() { //回车 展开多行输入框
        bool ok;
        QString oText = ui->lineEdit_suffix->text().simplified();
        QString text = QInputDialog::getMultiLineText(nullptr, "Suffix Filter", "Input Suffixes", oText.replace(' ', '\n'), &ok);
        text.replace('\n', ' ');
        if (ok) ui->lineEdit_suffix->setText(text);
    });

    QTimer::singleShot(0, [=]() { readPath();readSetting(); }); //防止阻塞UI
}
Widget::~Widget()
{
    delete ui;
}

void Widget::analyzeCode()
{
    int line = 0, blank = 0, comment = 0;
    int nowFile = 0, sumFile = 0;
    CodeCounter codeCnt;
    QStringList fileList, suffix;
    QString commentSym;

    QString text_suffix = ui->lineEdit_suffix->text().simplified();
    QString text_commentSym = ui->lineEdit_comment->text().simplified();
    if (!text_suffix.isEmpty()) {
        suffix = text_suffix.split(' ', QString::SkipEmptyParts);
        for (QString& str : suffix)
            str = "*." + str;
    } else {
        QMessageBox::critical(nullptr, "Error", "[Suffix] is Empty");
        return;
    }

    if (!text_commentSym.isEmpty())
        commentSym = text_commentSym;
    else {
        QMessageBox::critical(nullptr, "Error", "[Comment] is Empty");
        return;
    }
    qDebug() << suffix;
    qDebug() << commentSym;

    //codeCnt.setSuffixFilter(suffixFilter);//delete//在文件夹遍历时Filter Suffix，单独添加文件时不检查
    auto updateData = [&](const QString& path) {
        ui->label_line->setText(QString::number(line));
        ui->label_blank->setText(QString::number(blank));
        ui->label_comment->setText(QString::number(comment));
        ui->label_nowFile->setText(path);
        ui->progressBar->setValue(sumFile ? 100.0 * nowFile / sumFile : 0);
        qApp->processEvents(); //更新UI
    };

    ui->btn_run->setDisabled(true); //锁定Run，防止重复搞事情
    updateData("#Loading...");

    int nItem = ui->listWidget->count();
    for (int i = 0; i < nItem; i++) {
        QCheckBox* checkBox = checkBoxPtr(i);
        QString path = checkBox->toolTip();
        if (!checkBox->isChecked() || path.isEmpty()) continue;
        if (ui->checkBox_subdir->isChecked()) //是否递归子文件夹
            fileList << getFileList(path, suffix, QDirIterator::Subdirectories);
        else
            fileList << getFileList(path, suffix, QDirIterator::NoIteratorFlags);
    }
    sumFile = fileList.size();

    for (const QString& filePath : fileList) {
        codeCnt.bind(filePath, commentSym);
        codeCnt.run();
        line += codeCnt.Line();
        blank += codeCnt.Blank();
        comment += codeCnt.Comment();
        nowFile++;
        updateData(filePath);
    }
    updateData(QString("#CodeFile:[%1]").arg(sumFile));

    ui->btn_run->setDisabled(false); //解除锁定
}

QStringList Widget::getFileList(const QString& path, const QStringList& suffixFilter, QDirIterator::IteratorFlags flags) //const QStringList& suffixFilter
{
    QStringList fileList;
    QFileInfo fileInfo(path);
    if (path.isEmpty()) return fileList;
    if (fileInfo.isFile()) return fileList << path;
    if (!fileInfo.isDir()) return fileList;

    QDirIterator iter(path, suffixFilter, QDir::Files | QDir::NoSymLinks, flags);
    while (iter.hasNext()) {
        iter.next();
        fileList << iter.filePath(); //包含文件名的文件的全路径
    }
    return fileList;
}

void Widget::addCheckBoxToListWidget(QListWidget* lw, const QString& str)
{
    if (lw == nullptr || str.isEmpty()) return;

    QListWidgetItem* item = new QListWidgetItem;
    QCheckBox* checkBox = new QCheckBox;
    QFileInfo info(str);

    lw->addItem(item); //在ListWidget中添加一个条目
    lw->setItemWidget(item, checkBox); //在这个条目中放置CheckBox
    if (info.isDir())
        checkBox->setText(QDir(str).dirName());
    else if (info.isFile())
        checkBox->setText(info.fileName());
    else
        checkBox->setText("#Error# Neither Dir nor File");
    //checkBox->setStyleSheet("QCheckBox{color:white;font-family:Consolas}");//QCheckBox Qss也在ListWidget Qss表中一起写，才会有选中效果，很神奇
    checkBox->setToolTip(str);
    checkBox->setChecked(true);
}

QCheckBox* Widget::checkBoxPtr(int row)
{
    return static_cast<QCheckBox*>(ui->listWidget->itemWidget(ui->listWidget->item(row)));
}

bool Widget::isRepeat(const QString& str)
{
    int row = ui->listWidget->count();
    for (int i = 0; i < row; i++)
        if (checkBoxPtr(i)->toolTip() == str)
            return true;
    return false;
}

void Widget::initPosition()
{
    static auto setCenterX = [](QWidget* w, int cx) {
        w->move(cx - w->width() / 2, w->y());
    };
    const int Margin = 80;
    setCenterX(ui->widget_main, Normal_W / 2);
    setCenterX(ui->widget_line, Normal_W / 2);
    setCenterX(ui->widget_blank, Margin);
    setCenterX(ui->widget_comment, Normal_W - Margin);

    ui->checkBox_setting->move(Normal_W - ui->checkBox_setting->width(), ui->checkBox_setting->y());
}

void Widget::writePath()
{
    QStringList paths;
    int nItem = ui->listWidget->count();
    for (int i = 0; i < nItem; i++)
        paths << checkBoxPtr(i)->toolTip();

    QSettings IniSet(iniPath, QSettings::IniFormat);
    IniSet.setValue("PathTemp", paths);
}

void Widget::readPath()
{
    QSettings IniSet(iniPath, QSettings::IniFormat);
    QStringList paths = IniSet.value("PathTemp").toStringList();
    for (const QString& path : paths)
        addCheckBoxToListWidget(ui->listWidget, path);
}

void Widget::writeSetting()
{
    QSettings IniSet(iniPath, QSettings::IniFormat);
    IniSet.setValue("Suffix", ui->lineEdit_suffix->text().simplified());
    IniSet.setValue("Comment", ui->lineEdit_comment->text().simplified());
}

void Widget::readSetting()
{
    QSettings IniSet(iniPath, QSettings::IniFormat);
    QString suffix = IniSet.value("Suffix").toString().simplified();
    QString comment = IniSet.value("Comment").toString().simplified();
    ui->lineEdit_suffix->setText(suffix.isEmpty() ? defaultSuffix.join(' ') : suffix);
    ui->lineEdit_comment->setText(comment.isEmpty() ? defaultCommentSym : comment);
}

bool Widget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->listWidget && (event->type() == QEvent::KeyPress || event->type() == QEvent::ContextMenu)) { //右键或BackSpace delete item
        if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Backspace) return false;
        QListWidgetItem* item = ui->listWidget->itemAt(ui->listWidget->mapFromGlobal(QCursor::pos()));
        if (item != nullptr) {
            ui->listWidget->takeItem(ui->listWidget->row(item));
            QTimer::singleShot(100, [=]() { writePath(); });
        }
        return true;
    }
    return false;
}

void Widget::paintEvent(QPaintEvent* event)//不绘制会导致鼠标穿透背景
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setPen(Qt::NoPen);//取消边框//pen决定边框颜色
    painter.setBrush(QColor(10, 10, 10, 125));
    painter.drawRect(rect());
}

void Widget::resizeEvent(QResizeEvent* event)
{
    ui->widget_captain->setFixedWidth(event->size().width());
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    QPoint curPos = event->screenPos().toPoint();
    if (ui->widget_captain->geometry().contains(mapFromGlobal(curPos))) {
        isMousePress = true;
        preMousePos = curPos;
    }
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    isMousePress = false;
}

void Widget::mouseMoveEvent(QMouseEvent* event)
{
    if (isMousePress) {
        QPoint curPos = event->screenPos().toPoint(); //QCursor::pos()更新相对滞后，不准确
        move(pos() + curPos - preMousePos);
        preMousePos = curPos;
    }
}
