#pragma once

#include "AliasConfig.h"
#include "EtherCATWorker.h"

#include <QMainWindow>
#include <QList>

class QComboBox;
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

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void populateAdapters();
    void populateLabelCombo();
    void setControlsEnabled(bool enabled);
    QString selectedAdapterName() const;

    // Widgets
    QComboBox      *m_adapterCombo      = nullptr;
    QPushButton    *m_refreshAdapterBtn = nullptr;
    QPushButton    *m_scanBtn           = nullptr;
    QTableWidget   *m_table         = nullptr;
    QComboBox      *m_labelCombo    = nullptr;
    QLineEdit      *m_aliasEdit     = nullptr;
    QPushButton    *m_writeBtn      = nullptr;
    QPushButton    *m_reloadBtn     = nullptr;
    QPlainTextEdit *m_log           = nullptr;

    // Worker
    QThread          *m_thread  = nullptr;
    EtherCATWorker   *m_worker  = nullptr;

    // Config
    AliasConfig m_config;

    // State
    QList<SlaveInfo> m_slaves;
    int m_selectedSlave = -1;
};
