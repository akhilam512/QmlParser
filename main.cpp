#include <QCoreApplication>
#include "qmlparser.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QmlParser *parser = new QmlParser;

    parser->loadFile("path/to/qml");
    parser->writeMetaData();
    return a.exec();
}
