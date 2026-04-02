#include "MainWindow.h"

extern "C" {
#include "soem/soem.h"
}

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("EtherCAT Alias Tool"));
    setMinimumSize(900, 550);
    resize(1200, 620);

    // ---- Central widget ----
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ---- Adapter row ----
    {
        auto *row = new QHBoxLayout;
        m_refreshAdapterBtn = new QPushButton(QStringLiteral("Refresh"));
        m_refreshAdapterBtn->setFixedWidth(70);
        row->addWidget(m_refreshAdapterBtn);
        row->addWidget(new QLabel(QStringLiteral("Adapter:")));
        m_adapterCombo = new QComboBox;
        m_adapterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(m_adapterCombo);
        m_scanBtn = new QPushButton(QStringLiteral("Scan"));
        m_scanBtn->setFixedWidth(80);
        row->addWidget(m_scanBtn);
        root->addLayout(row);
    }

    // ---- Slave table ----
    m_table = new QTableWidget(0, 11);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("#"),
        QStringLiteral("Name"),
        QStringLiteral("Vendor ID"),
        QStringLiteral("Product Code"),
        QStringLiteral("Current Alias"),
        QStringLiteral("Matched Label"),
        QStringLiteral("Serial Number"),
        QStringLiteral("Bus Voltage"),
        QStringLiteral("Error Register"),
        QStringLiteral("Last Error"),
        QStringLiteral("Output Position")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QWidget::customContextMenuRequested,
            this, &MainWindow::onTableContextMenu);
    m_table->installEventFilter(this);
    root->addWidget(m_table, 1);

    // ---- Write alias group ----
    {
        auto *grp = new QGroupBox(QStringLiteral("Write Alias"));
        auto *gl  = new QVBoxLayout(grp);

        // Label row
        auto *labelRow = new QHBoxLayout;
        labelRow->addWidget(new QLabel(QStringLiteral("From config:")));
        m_labelCombo = new QComboBox;
        m_labelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        labelRow->addWidget(m_labelCombo);

        m_reloadBtn = new QPushButton(QStringLiteral("Reload Config…"));
        m_reloadBtn->setFixedWidth(130);
        labelRow->addWidget(m_reloadBtn);
        gl->addLayout(labelRow);

        // Manual entry row
        auto *manualRow = new QHBoxLayout;
        manualRow->addWidget(new QLabel(QStringLiteral("Or enter alias:")));
        m_aliasEdit = new QLineEdit;
        m_aliasEdit->setPlaceholderText(QStringLiteral("e.g. 3301"));
        m_aliasEdit->setMaximumWidth(120);
        m_aliasEdit->setValidator(
            new QRegularExpressionValidator(
                QRegularExpression(QStringLiteral("[0-9]{0,5}")),
                m_aliasEdit));
        manualRow->addWidget(m_aliasEdit);
        manualRow->addStretch();

        m_writeBtn = new QPushButton(QStringLiteral("Write Alias to Selected Slave"));
        manualRow->addWidget(m_writeBtn);
        gl->addLayout(manualRow);

        root->addWidget(grp);
    }

    // ---- Log ----
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setFixedHeight(120);
    m_log->setPlaceholderText(QStringLiteral("Status messages will appear here…"));
    root->addWidget(m_log);

    // ---- Worker thread ----
    m_thread = new QThread(this);
    m_worker = new EtherCATWorker;
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &EtherCATWorker::slavesScanned,  this, &MainWindow::onSlavesScanned);
    connect(m_worker, &EtherCATWorker::aliasWritten,   this, &MainWindow::onAliasWritten);
    connect(m_worker, &EtherCATWorker::logMessage,     this, &MainWindow::onLogMessage);

    m_thread->start();

    // ---- Button connections ----
    connect(m_refreshAdapterBtn, &QPushButton::clicked, this, &MainWindow::populateAdapters);
    connect(m_scanBtn,    &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_writeBtn,   &QPushButton::clicked, this, &MainWindow::onWriteAliasClicked);
    connect(m_reloadBtn,  &QPushButton::clicked, this, &MainWindow::onReloadConfigClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onTableSelectionChanged);
    connect(m_labelCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onLabelComboChanged);

    // ---- Initial state ----
    m_writeBtn->setEnabled(false);

    // Load default config from next to executable
    QString defaultConfig = QApplication::applicationDirPath() + QStringLiteral("/MK2_alias_config.json");
    if (m_config.load(defaultConfig)) {
        populateLabelCombo();
        onLogMessage(QStringLiteral("Loaded config: %1").arg(defaultConfig));
    } else {
        onLogMessage(QStringLiteral("No MK2_alias_config.json found at %1").arg(defaultConfig));
    }

    populateAdapters();
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
}

// ---------------------------------------------------------------------------
// Adapter list (called on main thread — ec_find_adapters doesn't need a socket)
// ---------------------------------------------------------------------------

void MainWindow::populateAdapters()
{
    m_adapterCombo->clear();

    ec_adaptert *head = ec_find_adapters();
    ec_adaptert *a = head;
    while (a) {
        m_adapterCombo->addItem(
            QString::fromLocal8Bit(a->desc),
            QString::fromLocal8Bit(a->name));
        a = a->next;
    }
    ec_free_adapters(head);

    if (m_adapterCombo->count() == 0) {
        onLogMessage(QStringLiteral("No network adapters found."));
        return;
    }

    // Auto-select the first USB adapter if present
    for (int i = 0; i < m_adapterCombo->count(); i++) {
        if (m_adapterCombo->itemText(i).contains(QStringLiteral("USB"), Qt::CaseInsensitive)) {
            m_adapterCombo->setCurrentIndex(i);
            break;
        }
    }
}

void MainWindow::populateLabelCombo()
{
    m_labelCombo->clear();
    m_labelCombo->addItem(QStringLiteral("— select label —"), QVariant());
    for (const QString &lbl : m_config.labels())
        m_labelCombo->addItem(
            QStringLiteral("%1  (alias=%2)").arg(lbl).arg(m_config.aliasFor(lbl)),
            lbl);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MainWindow::onScanClicked()
{
    if (m_adapterCombo->count() == 0) return;

    setControlsEnabled(false);
    m_table->setRowCount(0);
    m_selectedSlave = -1;

    QString adapterName = selectedAdapterName();
    QMetaObject::invokeMethod(m_worker, "scanSlaves",
                              Qt::QueuedConnection,
                              Q_ARG(QString, adapterName));
}

void MainWindow::onWriteAliasClicked()
{
    if (m_selectedSlave < 0) {
        QMessageBox::warning(this, QStringLiteral("No slave selected"),
                             QStringLiteral("Please select a slave in the table first."));
        return;
    }

    // Determine alias value: prefer manual entry if filled, otherwise use label combo
    uint16_t alias = 0;
    bool ok = false;

    QString manualText = m_aliasEdit->text().trimmed();
    if (!manualText.isEmpty()) {
        alias = static_cast<uint16_t>(manualText.toUInt(&ok, 10));
    }

    if (!ok) {
        QString lbl = m_labelCombo->currentData().toString();
        if (lbl.isEmpty() || !m_config.hasLabel(lbl)) {
            QMessageBox::warning(this, QStringLiteral("No alias"),
                                 QStringLiteral("Enter a decimal alias value or select a label from the config."));
            return;
        }
        alias = m_config.aliasFor(lbl);
        ok = true;
    }

    setControlsEnabled(false);
    QString adapterName = selectedAdapterName();
    QMetaObject::invokeMethod(m_worker, "writeAlias",
                              Qt::QueuedConnection,
                              Q_ARG(QString, adapterName),
                              Q_ARG(int, m_selectedSlave),
                              Q_ARG(uint16_t, alias));
}

void MainWindow::onReloadConfigClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open Alias Config"),
        m_config.path().isEmpty() ? QApplication::applicationDirPath() : m_config.path(),
        QStringLiteral("JSON files (*.json);;All files (*)"));

    if (path.isEmpty()) return;

    if (m_config.load(path)) {
        populateLabelCombo();
        onLogMessage(QStringLiteral("Reloaded config: %1 (%2 entries)")
                         .arg(path).arg(m_config.labels().size()));
    } else {
        QMessageBox::critical(this, QStringLiteral("Load failed"),
                              QStringLiteral("Could not parse %1").arg(path));
    }
}

void MainWindow::onSlavesScanned(QList<SlaveInfo> slaves)
{
    m_slaves = slaves;
    m_table->setRowCount(0);

    for (const SlaveInfo &si : slaves) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(si.pos)));
        m_table->setItem(row, 1, new QTableWidgetItem(si.name));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QStringLiteral("0x%1").arg(si.vendor, 8, 16, QLatin1Char('0'))));
        m_table->setItem(row, 3, new QTableWidgetItem(
            QStringLiteral("0x%1").arg(si.product, 8, 16, QLatin1Char('0'))));
        m_table->setItem(row, 4, new QTableWidgetItem(
            QString::number(si.alias)));

        // Find a matching label in the config
        QString matchedLabel;
        for (const QString &lbl : m_config.labels()) {
            if (m_config.aliasFor(lbl) == si.alias) {
                matchedLabel = lbl;
                break;
            }
        }
        m_table->setItem(row, 5, new QTableWidgetItem(matchedLabel));
        m_table->setItem(row, 6, new QTableWidgetItem(si.serialNumber));
        m_table->setItem(row, 7, new QTableWidgetItem(
            si.busVoltageValid
                ? QStringLiteral("%1 V").arg(si.busVoltage, 0, 'f', 1)
                : QStringLiteral("N/A")));
        m_table->setItem(row, 8, new QTableWidgetItem(
            si.errorRegisterValid
                ? QStringLiteral("0x%1").arg(si.errorRegister, 2, 16, QLatin1Char('0'))
                : QStringLiteral("N/A")));
        m_table->setItem(row, 9, new QTableWidgetItem(
            si.lastErrorValid
                ? QString::number(si.lastError)
                : QStringLiteral("N/A")));
        m_table->setItem(row, 10, new QTableWidgetItem(
            si.outputPositionValid
                ? QString::number(si.outputPosition)
                : QStringLiteral("N/A")));
    }

    setControlsEnabled(true);
}

void MainWindow::onAliasWritten(int slave, uint16_t alias, bool success, const QString &message)
{
    onLogMessage(message);
    setControlsEnabled(true);

    if (success) {
        // Refresh scan to show updated alias
        onScanClicked();
    }
}

void MainWindow::onLogMessage(const QString &message)
{
    m_log->appendPlainText(message);
}

void MainWindow::onTableSelectionChanged()
{
    auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        m_selectedSlave = -1;
        m_writeBtn->setEnabled(false);
    } else {
        int row = rows.first().row();
        m_selectedSlave = m_slaves.value(row).pos;
        m_writeBtn->setEnabled(true);
    }
}

void MainWindow::onLabelComboChanged(const QString &)
{
    // Clear manual entry when a label is selected, to avoid ambiguity
    QString lbl = m_labelCombo->currentData().toString();
    if (!lbl.isEmpty())
        m_aliasEdit->clear();
}

void MainWindow::onTableContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_table->itemAt(pos);
    if (!item) return;
    QMenu menu;
    QAction *copyAction = menu.addAction(QStringLiteral("Copy"));
    if (menu.exec(m_table->viewport()->mapToGlobal(pos)) == copyAction)
        QApplication::clipboard()->setText(item->text());
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_table && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy)) {
            if (QTableWidgetItem *item = m_table->currentItem())
                QApplication::clipboard()->setText(item->text());
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MainWindow::setControlsEnabled(bool enabled)
{
    m_scanBtn->setEnabled(enabled);
    m_reloadBtn->setEnabled(enabled);
    m_adapterCombo->setEnabled(enabled);

    // Write button only if slave is also selected
    m_writeBtn->setEnabled(enabled && m_selectedSlave >= 0);
}

QString MainWindow::selectedAdapterName() const
{
    return m_adapterCombo->currentData().toString();
}
