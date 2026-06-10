#pragma once

#include "AliasConfig.h"
#include "EtherCATWorker.h"

#include <QFile>
#include <QMainWindow>
#include <QList>
#include <QUrl>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class UpdateChecker;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;
class QLineEdit;
class QThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanClicked();
    void onWriteAliasClicked();
    void onReloadConfigClicked();
    void onSlavesScanned(QList<SlaveInfo> slaves);
    void onAliasWritten(int slave, uint16_t alias, bool success, const QString &message);
    void onLogMessage(const QString &message);
    void onTableSelectionChanged();
    void onLabelComboChanged(const QString &label);
    void onTableContextMenu(const QPoint &pos);
    void onUpdateAvailable(const QString &latestVersion, const QUrl &releaseUrl, bool interactive);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void populateAdapters();
    void populateLabelCombo();
    void setControlsEnabled(bool enabled);
    void applyNovantaColumns(bool show);
    QString selectedAdapterName() const;

    // Widgets
    QComboBox      *m_adapterCombo      = nullptr;
    QPushButton    *m_refreshAdapterBtn = nullptr;
    QPushButton    *m_scanBtn           = nullptr;
    QCheckBox      *m_novantaCheck      = nullptr;
    QTableWidget   *m_table         = nullptr;
    QComboBox      *m_labelCombo    = nullptr;
    QLineEdit      *m_aliasEdit     = nullptr;
    QPushButton    *m_writeBtn      = nullptr;
    QPushButton    *m_reloadBtn     = nullptr;
    QPlainTextEdit *m_log           = nullptr;

    // Worker
    QThread          *m_thread  = nullptr;
    EtherCATWorker   *m_worker  = nullptr;

    // Updates
    UpdateChecker *m_updateChecker = nullptr;

    // Config
    AliasConfig m_config;

    // State
    QList<SlaveInfo> m_slaves;
    int m_selectedSlave = -1;
    bool m_opInFlight = false;

    // Persistent operation log (for diagnosing field crashes)
    QFile m_logFile;
};
