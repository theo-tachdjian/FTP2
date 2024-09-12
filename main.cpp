#include "ftpclient.h"

#include <QApplication>

#include "serverauthdialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ServerAuthDialog dialog;
    int retval = dialog.exec();

    if (!retval) { return 0; }

    QString ip = dialog.ip;
    int port = dialog.port;
    QString username = dialog.username;
    QString password = dialog.password;

    // QString ip = QString("127.0.0.1");
    // int port = 12345;
    // QString username = QString("Qt");
    // QString password = QString("root");

    FTPClient w (nullptr, ip, port, username, password);
    w.show();
    return app.exec();
}
