#pragma once

#include <QMap>
#include <QString>
#include <cstdint>

class AliasConfig
{
public:
    AliasConfig() = default;

    bool load(const QString &path);

    QStringList labels() const;
    bool hasLabel(const QString &label) const;
    uint16_t aliasFor(const QString &label) const;

    QString path() const { return m_path; }

private:
    QMap<QString, uint16_t> m_map;
    QString m_path;
};
