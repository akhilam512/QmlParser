#include "qmlparser.h"

QmlParser::QmlParser(QObject *parent)
    : QObject(parent)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_objectCounter(0)
{
    m_qmlEngine = new QQmlEngine();
}

QStringList QmlParser::m_supportedQMLClasses = { "QQuickRectangle", "QQuickText", "Image", "QQuickSequentialAnimation", "QQuickParallelAnimation", "QQuickNumberAnimation", "QQuickRectangle_QML_0" };

static bool openAsQFile(QFile &file, QIODevice::OpenMode mode)
{
    return file.open(mode);
}

void QmlParser::loadFile(const QString path)
{
    m_qmlFilePath = path;
    // Load the qml
    if(!path.contains("qml") && QFile(path).exists()) {
        qDebug() << " Incorrect QML Path\n";
        return;
    }
    m_qmlComponent = new QQmlComponent(m_qmlEngine, path, QQmlComponent::PreferSynchronous);

    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qDebug() << "QML Component Error: " << error.url() << error.line() << error;
        return;
    }

    // Get the root object
    m_qmlRootObject = m_qmlComponent->create();
}

QVariant QmlParser::getObjectPropertyValue(QObject* object, const QString &propertyName)
{
    for(int i=0; i<object->metaObject()->propertyCount(); i++) {
         QMetaProperty prop = object->metaObject()->property(i);
         if(QString::compare(prop.name(), propertyName, Qt::CaseInsensitive) == 0) {
             QVariant propertyValue = prop.read(object);
             if(propertyValue.isValid()) {
                 // If duration, convert from milliseconds to seconds
                 if(propertyName == QString("duration"))
                     propertyValue = QVariant(propertyValue.toInt()/1000);

                 return propertyValue;
             }
         }
     }
     return QVariant();
}

void QmlParser::writeMetaData()
{
    m_objectCounter = 0;

    if(!openAsQFile(m_quickFile, QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << " Temporary File could not be created \n";
        return;
    }

    m_quickFile.setTextModeEnabled(true);

    writeTraverse(m_qmlRootObject);
    m_quickFile.close();
    if(!m_quickFile.open()) {
        qDebug() << " Temporary File could not be created \n";
        return;
    }
    QFile qmlFile(m_qmlFilePath);
    QTextStream stream(&qmlFile);

    if(!qmlFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << " Failed attempt on opening " << m_qmlFilePath;
        return;
    }
    qmlFile.write("/*\n");
    while(!m_quickFile.atEnd()) {
        QByteArray line = m_quickFile.readLine();
        qmlFile.write(line);
    }
    qmlFile.write("\n*/");
    qmlFile.close();
    m_quickFile.close();
}

void QmlParser::writeTraverse(QObject *rootObject)
{
    if(!rootObject) {
        return;
    }
    QTextStream stream(&m_quickFile);
    QString objectName = rootObject->metaObject()->className();

    if(objectName.contains("QQuickItem")) {
        m_objectCounter++;
        int childCount = 0;
        QObjectList childernList = rootObject->children();
        foreach(QObject* child, rootObject->children()) {
            // ensure we don't count stray qml classes
            if(QmlParser::m_supportedQMLClasses.contains(child->metaObject()->className())) {
                    childCount++;
            }
        }
        stream << "QQuickItem\n" << childCount << "\n";
        qreal width = getObjectPropertyValue(rootObject, "width").toReal();
        qreal height = getObjectPropertyValue(rootObject, "height").toReal();
        qreal x = getObjectPropertyValue(rootObject, "x").toReal();
        qreal y = getObjectPropertyValue(rootObject, "y").toReal();
        stream << x << "," << y << "," << width << "," << height << "," << "\n";
        stream.flush();

        foreach(QObject* child, rootObject->children()) {
            writeTraverse(child);
        }
    }
    else if(objectName.contains("Rectangle")) {
        m_objectCounter++;
        int childCount = 0;
        QObjectList childernList = rootObject->children();
        foreach(QObject* child, rootObject->children()) {
            // ensure we don't count stray qml classes
            if(QmlParser::m_supportedQMLClasses.contains(child->metaObject()->className())) {
                    childCount++;
            }
        }
        stream << "QQuickRectangle\n";
        stream << "child," << childCount << "\n";
        qreal width = getObjectPropertyValue(rootObject, "width").toReal();
        qreal height = getObjectPropertyValue(rootObject, "height").toReal();
        qreal x = getObjectPropertyValue(rootObject, "x").toReal();
        qreal y = getObjectPropertyValue(rootObject, "y").toReal();
        QString color = getObjectPropertyValue(rootObject, "color").toString();
        stream << "values," << x << "," << y << "," << width << "," << height << "," << color << "\n";
        stream.flush();

        foreach(QObject* child, rootObject->children()) {
            writeTraverse(child);
        }
    }
    else if(objectName.contains("QQuickShape")) {
        m_objectCounter++;
    }
    else if(objectName.contains("TextItem")) {
        m_objectCounter++;
    }
    else if(objectName.contains("Image")) {
        m_objectCounter++;
    }
    else if(objectName.contains("Animation")) {
        m_objectCounter++;
        if(objectName.contains("Sequential") || objectName.contains("Paralell")) {
            int childCount = 0;
            QObjectList childernList = rootObject->children();
            foreach(QObject* child, rootObject->children()) {
                // ensure we don't count stray qml classes
                if(QmlParser::m_supportedQMLClasses.contains(child->metaObject()->className())) {
                        childCount++;
                }
            }
            stream << objectName << "\n";
            stream << "children," << childCount << "\n";
            bool running = getObjectPropertyValue(rootObject, "running").toBool();
            stream << "values," << running << "\n";
            stream.flush();
            foreach(QObject* child, rootObject->children()) {
                writeTraverse(child);
            }
        }
        if(objectName.contains("Number")) {
            stream << "NumberAnimation\n";
            qreal from = getObjectPropertyValue(rootObject, "from").toReal();
            qreal to = getObjectPropertyValue(rootObject, "to").toReal();
            qreal duration = getObjectPropertyValue(rootObject, "duration").toReal();
            stream << "values," << from << "," << to << "," << duration << "\n";
        }
    }
    else {
        foreach(QObject* child, rootObject->children()) {
            writeTraverse(child);
        }
    }
}

void QmlParser::readMetaData()
{
    // TODO
    QFile qmlFile(m_qmlFilePath);
    QTextStream stream(&qmlFile);
    if(!qmlFile.open(QIODevice::ReadOnly | QIODevice::Append)) {
        qDebug() << " Failed attempt on opening " << m_qmlFilePath;
        return;
    }

    while(!qmlFile.atEnd()) {
        QByteArray line = qmlFile.readLine();
        if(line.contains("/*"))
            break;
    }

}

QQuickItem* QmlParser::createRectangle()
{
    // TODO
    QQmlEngine* engine = new QQmlEngine();
    QQmlComponent component(engine);
    component.setData("import QtQuick 2.0\nRectangle {}", QUrl());
    QQuickItem* childItem = qobject_cast<QQuickItem*>(component.create());

    if (childItem == nullptr)
    {
        qDebug() << component.errorString();
        return nullptr;
    }

    childItem->setWidth(50);
    childItem->setHeight(50);

    return childItem;
}

void QmlParser::addItem()
{
    // TODO
    writeMetaData();
    QQuickItem *rectItem = createRectangle();
    rectItem->setParent(m_qmlRootObject);
    writeMetaData();
}
