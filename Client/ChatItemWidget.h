#ifndef CHATITEMWIDGET_H
#define CHATITEMWIDGET_H

#include <QWidget>

namespace Ui {
class ChatItemWidget;
}

class ChatItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatItemWidget(QWidget *parent = nullptr);
    ~ChatItemWidget();
    void setMessage(const QString &message, const QString &sender, const QString &receiver, bool isMyMessage = false); // Изменено

private:
    Ui::ChatItemWidget *ui;
};

#endif // CHATITEMWIDGET_H
