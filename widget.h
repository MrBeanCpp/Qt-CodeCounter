#ifndef WIDGET_H
#define WIDGET_H

#include <QApplication>
#include <QCheckBox>
#include <QDirIterator>
#include <QListWidget>
#include <QString>
#include <QStringList>
#include <QWidget>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    void analyzeCode(void);
    QStringList getFileList(const QString& path, const QStringList& suffixFilter, QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);
    void addCheckBoxToListWidget(QListWidget* lw, const QString& str);
    QCheckBox* checkBoxPtr(int row);
    bool isRepeat(const QString& str);
    void initPosition(void);
    void writePath(void);
    void readPath(void);
    void writeSetting(void);
    void readSetting(void);

private:
    Ui::Widget* ui;

    QStringList defaultSuffix { "h", "cpp", "c" };
    QString defaultCommentSym = "//";
    const int Normal_W = 700;
    const int Extra_W = 150;

    QPoint preMousePos;
    bool isMousePress = false;

    const QString iniPath = QApplication::applicationDirPath() + "/settings.ini";

    // QObject interface
public:
    bool eventFilter(QObject* watched, QEvent* event) override;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
};
#endif // WIDGET_H
