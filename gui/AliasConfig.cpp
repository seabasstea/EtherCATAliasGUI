#include "AliasConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

bool AliasConfig::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    m_map.clear();
    const QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.value().isDouble())
            m_map.insert(it.key(), static_cast<uint16_t>(it.value().toInt()));
    }

    m_path = path;
    return true;
}

QStringList AliasConfig::labels() const
{
    return m_map.keys();
}

bool AliasConfig::hasLabel(const QString &label) const
{
    return m_map.contains(label);
}

uint16_t AliasConfig::aliasFor(const QString &label) const
{
    return m_map.value(label, 0);
}
