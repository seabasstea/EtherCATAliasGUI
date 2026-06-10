#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("EtherCAT Alias Tool"));
    app.setOrganizationName(QStringLiteral("EtherCATAliasGUI"));
    app.setWindowIcon(QIcon(QStringLiteral(":/ethercat-alias-tool.png")));

    MainWindow w;
    w.show();

    return app.exec();
}
