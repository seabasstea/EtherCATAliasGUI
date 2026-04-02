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
    float    busVoltage       = 0.0f;   // 0x2060:00, V
    uint8_t  errorRegister    = 0;      // 0x1001:00, bit flags
    int32_t  lastError        = 0;      // 0x200F:00, error code
    int32_t  outputPosition   = 0;      // 0x2051:00, encoder counts
    bool     busVoltageValid      = false;
    bool     errorRegisterValid   = false;
    bool     lastErrorValid       = false;
    bool     outputPositionValid  = false;
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
