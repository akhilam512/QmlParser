#ifndef QMLPARSER_H
#define QMLPARSER_H

#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QFile>
#include <QDebug>
#include <QQuickItem>
#include <QAnimationGroup>
#include <QTemporaryFile>

class QmlParser : public QObject
{
    Q_OBJECT
public:
    explicit QmlParser(QObject *parent = nullptr);
    ~QmlParser() = default;

    void loadFile(const QString path);
    void setQmlEngine(QQmlEngine* engine);
    QObject* getRootObject() { return m_qmlRootObject; }
    QVariant getObjectPropertyValue(QObject* object, const QString &propertyName);
    void writeMetaData();
    void readMetaData();
    void addItem();
    QQuickItem* createRectangle();

private:
    void writeTraverse(QObject*);
    int m_objectCounter;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QObject *m_qmlRootObject;
    QTemporaryFile m_quickFile;
    QTextStream m_textStream;
    QString m_qmlFilePath;
    static QStringList m_supportedQMLClasses;
};

#endif // QMLPARSER_H
