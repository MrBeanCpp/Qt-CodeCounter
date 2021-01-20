#include "codecounter.h"

CodeCounter::CodeCounter()
{
    clearData();
}

CodeCounter::CodeCounter(const QString& fileName, const QString& comment_sym)
    : CodeCounter()
{
    bind(fileName, comment_sym);
}

void CodeCounter::bind(const QString& fileName, const QString& comment_sym)
{
    this->fileName = fileName;
    this->comment_sym = comment_sym.simplified();
    clearData();
}

void CodeCounter::run()
{
    clearData();
    if (comment_sym.isEmpty()) return;
    QFile file(fileName);
    if (file.open(QFile::Text | QFile::ReadOnly)) {
        QTextStream text(&file);
        QString sLine;
        while (!text.atEnd()) {
            sLine = text.readLine().simplified();
            if (sLine.isEmpty())
                blank++;
            else if (sLine.startsWith(comment_sym))
                comment++;
            else
                line++;
        }
    }
}

int CodeCounter::Line()
{
    return line;
}

int CodeCounter::Blank()
{
    return blank;
}

int CodeCounter::Comment()
{
    return comment;
}

void CodeCounter::clearData()
{
    line = blank = comment = 0;
}
