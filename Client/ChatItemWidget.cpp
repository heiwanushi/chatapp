#include "ChatItemWidget.h"
#include "ui_ChatItemWidget.h"
#include <QTime>

ChatItemWidget::ChatItemWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatItemWidget)
{
    ui->setupUi(this);
}

ChatItemWidget::~ChatItemWidget()
{
    delete ui;
}

void ChatItemWidget::setMessage(const QString &message, const QString &sender, const QString &receiver, bool isMyMessage)
{
    if (isMyMessage) {
        ui->lblMessage->setAlignment(Qt::AlignRight);
        ui->lblName->setText(QString("%1 -> %2").arg(sender, receiver)); // Устанавливаем отправителя и получателя
    } else {
        ui->lblMessage->setAlignment(Qt::AlignLeft);
        ui->lblName->setText(QString("%1 -> %2").arg(sender, receiver)); // Устанавливаем отправителя и получателя
    }

    ui->lblMessage->setText(message);
    ui->lblTime->setText(QTime::currentTime().toString("HH:mm"));
}
