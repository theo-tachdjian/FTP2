#include "ftpclient.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString username = QString("Qt");
    QString password = QString("root");

    FTPClient w (nullptr, "127.0.0.1", 12345, username, password);
    w.show();
    return a.exec();
}
