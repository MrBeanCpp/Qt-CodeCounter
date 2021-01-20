#ifndef CODECOUNTER_H
#define CODECOUNTER_H

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>
class CodeCounter
{
public:
    CodeCounter();
    CodeCounter(const QString& fileName, const QString& comment_sym);

    void bind(const QString& fileName, const QString& comment_sym);

    void run(void);
    int Line(void);
    int Blank(void);
    int Comment(void);

private:
    void clearData(void);

private:
    QString fileName;
    QString comment_sym;
    int line, blank, comment;
};

#endif // CODECOUNTER_H
