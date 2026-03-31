#include "EtherCATWorker.h"

extern "C" {
#include "soem/soem.h"
}

#include <cstring>

// ---------------------------------------------------------------------------
// Internal helpers (mirrors eepromtool.c logic)
// ---------------------------------------------------------------------------

static void calc_crc(uint8_t *crc, uint8_t b)
{
    *crc ^= b;
    for (int j = 0; j <= 7; j++) {
        if (*crc & 0x80)
            *crc = (*crc << 1) ^ 0x07;
        else
            *crc = (*crc << 1);
    }
}

static uint16_t SIIcrc(uint8_t *buf)
{
    uint8_t crc = 0xff;
    for (int i = 0; i <= 13; i++)
        calc_crc(&crc, buf[i]);
    return static_cast<uint16_t>(crc);
}

// Read `length` bytes from slave EEPROM starting at byte offset `start`.
// Returns true on success.
static bool eepromRead(ecx_contextt &ctx, int slave, int start, int length,
                       uint8_t *ebuf, int ebufSize)
{
    if (ctx.slavecount < slave || slave <= 0 || (start + length) > ebufSize)
        return false;

    uint16_t aiadr = static_cast<uint16_t>(1 - slave);
    uint8_t eepctl = 2;
    ecx_APWR(&ctx.port, aiadr, ECT_REG_EEPCFG, sizeof(eepctl), &eepctl, EC_TIMEOUTRET);
    eepctl = 0;
    ecx_APWR(&ctx.port, aiadr, ECT_REG_EEPCFG, sizeof(eepctl), &eepctl, EC_TIMEOUTRET);

    uint16_t estat = 0;
    ecx_APRD(&ctx.port, aiadr, ECT_REG_EEPSTAT, sizeof(estat), &estat, EC_TIMEOUTRET);
    estat = etohs(estat);

    if (estat & EC_ESTAT_R64) {
        for (int i = start; i < (start + length); i += 8) {
            uint64_t b8 = ecx_readeepromAP(&ctx, aiadr, i >> 1, EC_TIMEOUTEEP);
            for (int k = 0; k < 8 && (i + k) < ebufSize; k++)
                ebuf[i + k] = static_cast<uint8_t>((b8 >> (k * 8)) & 0xFF);
        }
    } else {
        for (int i = start; i < (start + length); i += 4) {
            uint32_t b4 = static_cast<uint32_t>(
                ecx_readeepromAP(&ctx, aiadr, i >> 1, EC_TIMEOUTEEP) & 0xFFFFFFFF);
            for (int k = 0; k < 4 && (i + k) < ebufSize; k++)
                ebuf[i + k] = static_cast<uint8_t>((b4 >> (k * 8)) & 0xFF);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

EtherCATWorker::EtherCATWorker(QObject *parent) : QObject(parent) {}

void EtherCATWorker::scanSlaves(const QString &adapterName)
{
    emit logMessage(QStringLiteral("Scanning on %1…").arg(adapterName));

    ecx_contextt ctx;
    memset(&ctx, 0, sizeof(ctx));

    QByteArray ifName = adapterName.toLocal8Bit();

    if (!ecx_init(&ctx, ifName.data())) {
        emit logMessage(QStringLiteral("Failed to open adapter %1. Run as Administrator?").arg(adapterName));
        emit slavesScanned({});
        return;
    }

    if (ecx_config_init(&ctx) <= 0) {
        emit logMessage(QStringLiteral("No slaves found on %1.").arg(adapterName));
        ecx_close(&ctx);
        emit slavesScanned({});
        return;
    }

    // Configure sync managers and mailboxes (required for CoE SDO access).
    // We do NOT wait for SAFE_OP — PRE_OP is sufficient for SDO reads.
    static uint8_t IOmap[4096];
    ecx_config_map_group(&ctx, IOmap, 0);

    // Read slave info
    static const int EEPBUFSIZE = 128;
    uint8_t ebuf[EEPBUFSIZE];

    QList<SlaveInfo> slaves;
    for (int i = 1; i <= ctx.slavecount; i++) {
        SlaveInfo si;
        si.pos      = i;
        si.name     = QString::fromLocal8Bit(ctx.slavelist[i].name);
        si.vendor   = static_cast<uint32_t>(ctx.slavelist[i].eep_man);
        si.product  = static_cast<uint32_t>(ctx.slavelist[i].eep_id);
        si.revision = static_cast<uint32_t>(ctx.slavelist[i].eep_rev);
        si.alias    = 0;

        // Read alias from EEPROM word 0x04 (byte offset 8)
        memset(ebuf, 0, EEPBUFSIZE);
        if (eepromRead(ctx, i, 0, 16, ebuf, EEPBUFSIZE)) {
            uint16_t *wbuf = reinterpret_cast<uint16_t *>(ebuf);
            si.alias = wbuf[0x04];
        }

        // Read serial number via CoE SDO 0x26E6:0x00 (UINT32)
        si.serialNumber = QStringLiteral("No info");
        if (ctx.slavelist[i].mbx_proto & ECT_MBXPROT_COE) {
            uint32_t sn = 0;
            int snSize = static_cast<int>(sizeof(sn));
            int wkc = ecx_SDOread(&ctx, static_cast<uint16_t>(i),
                                  0x26E6, 0x00, FALSE, &snSize, &sn, EC_TIMEOUTRXM);
            if (wkc > 0 && !ctx.ecaterror)
                si.serialNumber = QString::number(sn);
            else
                ecx_elist2string(&ctx); // drain sticky error so next slave reads cleanly
        }

        slaves.append(si);
    }

    emit logMessage(QStringLiteral("Found %1 slave(s).").arg(ctx.slavecount));
    ecx_close(&ctx);
    emit slavesScanned(slaves);
}

void EtherCATWorker::writeAlias(const QString &adapterName, int slave, uint16_t alias)
{
    emit logMessage(QStringLiteral("Writing alias %1 to slave %2…")
                        .arg(alias)
                        .arg(slave));

    ecx_contextt ctx;
    memset(&ctx, 0, sizeof(ctx));

    QByteArray ifName = adapterName.toLocal8Bit();

    if (!ecx_init(&ctx, ifName.data())) {
        emit aliasWritten(slave, alias, false,
                          QStringLiteral("Failed to open adapter. Run as Administrator?"));
        return;
    }

    // Count slaves via broadcast read (no full config needed)
    uint16_t w = 0;
    int wkc = ecx_BRD(&ctx.port, 0x0000, ECT_REG_TYPE, sizeof(w), &w, EC_TIMEOUTSAFE);
    ctx.slavecount = wkc;

    if (wkc <= 0 || slave > wkc) {
        ecx_close(&ctx);
        emit aliasWritten(slave, alias, false,
                          QStringLiteral("Slave %1 not found (total: %2).").arg(slave).arg(wkc));
        return;
    }

    // Read first 14 bytes of EEPROM (covers words 0x00–0x06)
    static const int CRCBUFSIZE = 16; // a bit more than 14 to stay word-aligned
    uint8_t ebuf[CRCBUFSIZE];
    memset(ebuf, 0, CRCBUFSIZE);
    if (!eepromRead(ctx, slave, 0, CRCBUFSIZE, ebuf, CRCBUFSIZE)) {
        ecx_close(&ctx);
        emit aliasWritten(slave, alias, false, QStringLiteral("Could not read slave EEPROM."));
        return;
    }

    // Update alias word (0x04) in local buffer for CRC calculation
    uint16_t *wbuf = reinterpret_cast<uint16_t *>(ebuf);
    wbuf[0x04] = alias;

    uint16_t crc = SIIcrc(ebuf);

    // Write alias at word 0x04 and CRC at word 0x07
    uint16_t aiadr = static_cast<uint16_t>(1 - slave);
    uint8_t eepctl = 2;
    ecx_APWR(&ctx.port, aiadr, ECT_REG_EEPCFG, sizeof(eepctl), &eepctl, EC_TIMEOUTRET);
    eepctl = 0;
    ecx_APWR(&ctx.port, aiadr, ECT_REG_EEPCFG, sizeof(eepctl), &eepctl, EC_TIMEOUTRET);

    int ret = ecx_writeeepromAP(&ctx, aiadr, 0x04, alias, EC_TIMEOUTEEP);
    if (ret)
        ret = ecx_writeeepromAP(&ctx, aiadr, 0x07, crc, EC_TIMEOUTEEP);

    ecx_close(&ctx);

    if (ret) {
        emit aliasWritten(slave, alias, true,
                          QStringLiteral("Alias %1 written to slave %2 (CRC=0x%3).")
                              .arg(alias)
                              .arg(slave)
                              .arg(crc, 4, 16, QLatin1Char('0')));
    } else {
        emit aliasWritten(slave, alias, false,
                          QStringLiteral("EEPROM write failed for slave %1.").arg(slave));
    }
}
