#include "serverauthdialog.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QIntValidator>
#include <QFormLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

ServerAuthDialog::ServerAuthDialog(QWidget *parent)
    :QDialog(parent)
{
    this->setWindowTitle("Server Auth");

    QVBoxLayout *main_layout = new QVBoxLayout();

    QFormLayout *form_layout = new QFormLayout();

    ip_le = new QLineEdit();

    QString IpRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression IpRegex ("^" + IpRange
                               + "(\\." + IpRange + ")"
                               + "(\\." + IpRange + ")"
                               + "(\\." + IpRange + ")$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(IpRegex);
    ip_le->setValidator(ipValidator);

    port_le = new QLineEdit();
    port_le->setValidator(new QIntValidator(0, 65535, port_le));

    username_le = new QLineEdit();

    password_le = new QLineEdit();
    password_le->setEchoMode(QLineEdit::EchoMode::Password);

    form_layout->addRow("IP Address", ip_le);
    form_layout->addRow("Port", port_le);
    form_layout->addRow("Username", username_le);
    form_layout->addRow("Password", password_le);

    QHBoxLayout *btn_lay = new QHBoxLayout();
    QPushButton *accept_btn = new QPushButton("Connect");
    QPushButton *cancel_btn = new QPushButton("Cancel");

    connect(accept_btn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));

    btn_lay->addWidget(accept_btn);
    btn_lay->addWidget(cancel_btn);

    main_layout->addLayout(form_layout);
    main_layout->addLayout(btn_lay);

    this->setLayout(main_layout);
}

ServerAuthDialog::~ServerAuthDialog()
{
    // delete ip_le;
    // delete port_le;
    // delete username_le;
    // delete password_le;
}

void ServerAuthDialog::accept()
{
    if (ip_le->hasAcceptableInput() && port_le->hasAcceptableInput()
        && !username_le->text().isEmpty() && !password_le->text().isEmpty()) {

        ip = ip_le->text();
        port = atoi(port_le->text().toStdString().c_str());
        username = username_le->text();
        password = password_le->text();

        QDialog::accept();
    } else {
        QMessageBox::warning(this, "Invalid fields", "One or more fields are empty or invalid !");
    }
}
