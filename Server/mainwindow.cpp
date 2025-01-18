#include "mainwindow.h"
#include "ui_mainwindow.h"

// Конструктор ServerManager, который принимает номер порта и родительский объект
ServerManager::ServerManager(ushort port, QObject *parent)
    : QObject{parent}
{
    setupServer(port); // Настройка сервера
}

// Уведомление других клиентов об изменении имени клиента
void ServerManager::notifyOtherClients(QString prevName, QString name)
{
    auto message = _protocol.setClientNameMessage(prevName, name); // Создание сообщения об изменении имени
    foreach (auto cl, _clients) {
        auto clientName = cl->property("clientName").toString();
        if (clientName != name) {
            cl->write(message); // Отправка сообщения другим клиентам
        }
    }
}

// Отправка текстового сообщения другим клиентам
void ServerManager::onTextForOtherClients(QString message, QString receiver, QString sender)
{
    auto msg = _protocol.textMessage(message, receiver, sender); // Создание текстового сообщения с отправителем
    if (receiver == "All") {
        foreach (auto cl, _clients) {
            auto clientName = cl->property("clientName").toString();
            if (clientName != sender) {
                cl->write(msg); // Отправка сообщения всем клиентам, кроме отправителя
            }
        }
    } else {
        foreach (auto cl, _clients) {
            auto clientName = cl->property("clientName").toString();
            if (clientName == receiver) {
                cl->write(msg); // Отправка сообщения конкретному клиенту
                return;
            }
        }
    }
}

// Обработка нового подключения клиента
void ServerManager::newClientConnectionReceived()
{
    auto client = _server->nextPendingConnection(); // Получение нового подключения

    auto id = _clients.count() + 1;
    auto clientName = QString("Client (%1)").arg(id); // Назначение имени новому клиенту
    client->setProperty("id", id);
    client->setProperty("clientName", clientName);

    connect(client, &QTcpSocket::disconnected, this, &ServerManager::onClientDisconnected); // Подключение сигнала отключения клиента
    emit newClientConnected(client); // Сигнал о подключении нового клиента

    if (id > 1) {
        auto message = _protocol.setConnectionACKMessage(clientName, _clients.keys());
        client->write(message); // Отправка сообщения о подтверждении соединения

        auto newClientMessage = _protocol.setNewClientMessage(clientName);
        foreach (auto cl, _clients) {
            cl->write(newClientMessage); // Уведомление других клиентов о новом клиенте
        }
    }
    _clients[clientName] = client; // Добавление нового клиента в список
}

// Обработка отключения клиента
void ServerManager::onClientDisconnected()
{
    auto client = qobject_cast<QTcpSocket *>(sender());
    auto clientName = client->property("clientName").toString();
    _clients.remove(clientName); // Удаление клиента из списка
    auto message = _protocol.setClientDisconnectedMessage(clientName);
    foreach (auto cl, _clients) {
        cl->write(message); // Уведомление других клиентов об отключении клиента
    }

    emit clientDisconnected(client); // Сигнал об отключении клиента
}

// Настройка сервера
void ServerManager::setupServer(ushort port)
{
    _server = new QTcpServer(this);
    connect(_server, &QTcpServer::newConnection, this, &ServerManager::newClientConnectionReceived); // Подключение сигнала нового соединения
    _server->listen(QHostAddress::Any, port); // Прослушивание указанного порта
}

// Конструктор MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    seupServer(); // Настройка сервера
}

// Деструктор MainWindow
MainWindow::~MainWindow()
{
    delete ui;
}

// Обработка подключения нового клиента
void MainWindow::newClientConnected(QTcpSocket *client)
{
    auto id = client->property("id").toInt();
    ui->lstClients->addItem(QString("New Client added: %1").arg(id)); // Добавление информации о новом клиенте в список
    auto chatWidget = new ClientChatWidget(client, ui->tbClientsChat);
    ui->tbClientsChat->addTab(chatWidget, QString("Client (%1)").arg(id)); // Добавление вкладки с новым клиентом

    connect(chatWidget, &ClientChatWidget::clientNameChanged, this, &MainWindow::setClientName); // Подключение сигнала изменения имени клиента
    connect(chatWidget, &ClientChatWidget::statusChanged, this, &MainWindow::setClientStatus); // Подключение сигнала изменения статуса клиента
    connect(chatWidget, &ClientChatWidget::isTyping, [this](QString name) {
        this->statusBar()->showMessage(name, 750); // Отображение сообщения "печатает"
    });

    connect(chatWidget, &ClientChatWidget::textForOtherClients, _server, &ServerManager::onTextForOtherClients); // Подключение сигнала отправки текстового сообщения другим клиентам
}

// Обработка отключения клиента
void MainWindow::clientDisconnected(QTcpSocket *client)
{
    auto id = client->property("id").toInt();
    ui->lstClients->addItem(QString("Client disconnected: %1").arg(id)); // Добавление информации об отключении клиента в список

    // Закрытие вкладки отключенного клиента
    for (int i = 0; i < ui->tbClientsChat->count(); ++i) {
        auto chatWidget = qobject_cast<ClientChatWidget *>(ui->tbClientsChat->widget(i));
        if (chatWidget && chatWidget->client() == client) {
            ui->tbClientsChat->removeTab(i);
            break;
        }
    }
}

// Изменение имени клиента
void MainWindow::setClientName(QString prevName, QString name)
{
    auto widget = qobject_cast<QWidget *>(sender());
    auto index = ui->tbClientsChat->indexOf(widget);
    ui->tbClientsChat->setTabText(index, name); // Обновление имени клиента на вкладке

    _server->notifyOtherClients(prevName, name); // Уведомление других клиентов об изменении имени
}

// Изменение статуса клиента
void MainWindow::setClientStatus(ChatProtocol::Status status)
{
    auto widget = qobject_cast<QWidget *>(sender());
    auto index = ui->tbClientsChat->indexOf(widget);
    QString iconName = ":/icons/";
    switch (status) {
    case ChatProtocol::Available:
        iconName.append("available.png");
        break;
    case ChatProtocol::Away:
        iconName.append("away.png");
        break;
    case ChatProtocol::Busy:
        iconName.append("busy.png");
        break;
    default:
        iconName = "";
        break;
    }

    auto icon = QIcon(iconName);
    ui->tbClientsChat->setTabIcon(index, icon); // Установка иконки статуса на вкладке
}

// Настройка сервера
void MainWindow::seupServer()
{
    _server = new ServerManager();
    connect(_server, &ServerManager::newClientConnected, this, &MainWindow::newClientConnected); // Подключение сигнала нового клиента
    connect(_server, &ServerManager::clientDisconnected, this, &MainWindow::clientDisconnected); // Подключение сигнала отключения клиента
}

// Обработка закрытия вкладки клиента
void MainWindow::on_tbClientsChat_tabCloseRequested(int index)
{
    auto chatWidget = qobject_cast<ClientChatWidget *>(ui->tbClientsChat->widget(index));
    chatWidget->disconnect();
    ui->tbClientsChat->removeTab(index); // Удаление вкладки
}
