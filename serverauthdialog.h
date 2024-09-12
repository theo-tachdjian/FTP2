#ifndef SERVERAUTHDIALOG_H
#define SERVERAUTHDIALOG_H

#include <QDialog>
#include <QLineEdit>

class ServerAuthDialog : public QDialog
{
    Q_OBJECT
public:
    ServerAuthDialog(QWidget *parent = nullptr);
    ~ServerAuthDialog();

    QString ip;
    int port;
    QString username;
    QString password;

public Q_SLOTS:
    void accept();

private:
    QLineEdit *ip_le;
    QLineEdit *port_le;
    QLineEdit *username_le;
    QLineEdit *password_le;
};

#endif // SERVERAUTHDIALOG_H
