#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <cstdint>

struct SlaveInfo
{
    int      pos;
    QString  name;
    uint32_t vendor;
    uint32_t product;
    uint32_t revision;
    uint16_t alias;
    QString  serialNumber;
};

class EtherCATWorker : public QObject
{
    Q_OBJECT

public:
    explicit EtherCATWorker(QObject *parent = nullptr);

public slots:
    void scanSlaves(const QString &adapterName);
    void writeAlias(const QString &adapterName, int slave, uint16_t alias);

signals:
    void slavesScanned(QList<SlaveInfo> slaves);
    void aliasWritten(int slave, uint16_t alias, bool success, const QString &message);
    void logMessage(const QString &message);
};
