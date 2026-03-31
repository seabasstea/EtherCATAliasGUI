#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("EtherCAT Alias Tool"));
    app.setOrganizationName(QStringLiteral("EtherCATAliasGUI"));

    MainWindow w;
    w.show();

    return app.exec();
}
