#include "ClientChatWidget.h"
#include "ui_ClientChatWidget.h"
#include <QMessageBox>
#include <QDesktopServices>

// Конструктор ClientManager, который принимает IP-адрес и номер порта
ClientManager::ClientManager(QHostAddress ip, ushort port, QObject *parent)
    : QObject{parent},
    _ip(ip),
    _port(port)
{
    _socket = new QTcpSocket(this);
    setupClient(); // Настройка клиента
}

// Конструктор ClientManager, который принимает указатель на клиентский сокет
ClientManager::ClientManager(QTcpSocket *client, QObject *parent)
    : QObject{parent},
    _socket(client)
{
    setupClient(); // Настройка клиента
}

// Подключение к серверу
void ClientManager::connectToServer()
{
    _socket->connectToHost(_ip, _port);
}

// Отключение от сервера
void ClientManager::disconnectFromHost()
{
    _socket->disconnectFromHost();
}

// Отправка сообщения на сервер
void ClientManager::sendMessage(QString message)
{
    QString sender = "Server";
    _socket->write(_protocol.textMessage(message, name(), sender)); // Отправка текстового сообщения с отправителем
}

// Отправка имени на сервер
void ClientManager::sendName(QString name)
{
    _socket->write(_protocol.setNameMessage(name));
}

// Отправка статуса на сервер
void ClientManager::sendStatus(ChatProtocol::Status status)
{
    _socket->write(_protocol.setStatusMessage(status));
}

// Получение имени клиента
QString ClientManager::name() const
{
    auto id = _socket->property("id").toInt();
    auto name = _protocol.name().length() > 0 ? _protocol.name() : QString("Client (%1)").arg(id);
    return name;
}

// Отправка сообщения "печатает" на сервер
void ClientManager::sendIsTyping()
{
    _socket->write(_protocol.isTypingMessage());
}

// Отправка сообщения о начале отправки файла на сервер
void ClientManager::sendInitSendingFile(QString fileName)
{
    _tmpFileName = fileName;
    _socket->write(_protocol.setInitSendingFileMessage(fileName));
}

// Отправка сообщения о принятии файла на сервер
void ClientManager::sendAcceptFile()
{
    _socket->write(_protocol.setAcceptFileMessage());
}

// Отправка сообщения об отклонении файла на сервер
void ClientManager::sendRejectFile()
{
    _socket->write(_protocol.setRejectFileMessage());
}

// Обработка готовности чтения данных из сокета
void ClientManager::readyRead()
{
    auto data = _socket->readAll();
    _protocol.loadData(data);
    switch (_protocol.type()) {
    case ChatProtocol::Text:
        emit textMessageReceived(_protocol.message(), _protocol.receiver(), _protocol.sender());
        break;
    case ChatProtocol::SetName: {
        auto prevName = _socket->property("clientName").toString();
        _socket->setProperty("clientName", name());
        emit nameChanged(prevName, name());
        break;
    }
    case ChatProtocol::SetStatus:
        emit statusChanged(_protocol.status());
        break;
    case ChatProtocol::IsTyping:
        emit isTyping();
        break;
    case ChatProtocol::InitSendingFile:
        emit initReceivingFile(_protocol.name(), _protocol.fileName(), _protocol.fileSize());
        break;
    case ChatProtocol::AcceptSendingFile:
        sendFile();
        break;
    case ChatProtocol::RejectSendingFile:
        emit rejectReceivingFile();
        break;
    case ChatProtocol::SendFile:
        saveFile();
        break;
    default:
        break;
    }
}

// Настройка клиента
void ClientManager::setupClient()
{
    connect(_socket, &QTcpSocket::connected, this, &ClientManager::connected);
    connect(_socket, &QTcpSocket::disconnected, this, &ClientManager::disconnected);
    connect(_socket, &QTcpSocket::readyRead, this, &ClientManager::readyRead);
}

// Отправка файла на сервер
void ClientManager::sendFile()
{
    _socket->write(_protocol.setFileMessage(_tmpFileName));
}

// Сохранение файла на клиенте
void ClientManager::saveFile()
{
    QDir dir;
    dir.mkdir(name());
    auto path = QString("%1/%2/%3_%4")
                    .arg(dir.canonicalPath(), name(), QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"), _protocol.fileName());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(_protocol.fileData());
        file.close();
        emit fileSaved(path);
    }
}

// Конструктор ClientChatWidget, который принимает указатель на клиентский сокет
ClientChatWidget::ClientChatWidget(QTcpSocket *client, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientChatWidget)
{
    ui->setupUi(this);
    _client = new ClientManager(client, this);
    connect(_client, &ClientManager::disconnected, this, &ClientChatWidget::clientDisconnected);
    connect(_client, &ClientManager::textMessageReceived, this, &ClientChatWidget::textMessageReceived);
    connect(_client, &ClientManager::isTyping, this, &ClientChatWidget::onTyping);
    connect(_client, &ClientManager::nameChanged, this, &ClientChatWidget::onClientNameChanged);
    connect(_client, &ClientManager::statusChanged, this, &ClientChatWidget::statusChanged);
    connect(_client, &ClientManager::initReceivingFile, this, &ClientChatWidget::onInitReceivingFile);
    connect(_client, &ClientManager::fileSaved, this, &ClientChatWidget::onFileSaved);
    connect(ui->lnMessage, &QLineEdit::textChanged, _client, &ClientManager::sendIsTyping);

    dir.mkdir(_client->name());
    dir.setPath("./" + _client->name());
}

// Отключение клиента от сервера
void ClientChatWidget::disconnect()
{
    _client->disconnectFromHost();
}

// Деструктор ClientChatWidget
ClientChatWidget::~ClientChatWidget()
{
    delete ui;
}

// Обработка отключения клиента
void ClientChatWidget::clientDisconnected()
{
    ui->wdgSendMessage->setEnabled(false);
}

// Обработка нажатия кнопки отправки сообщения
void ClientChatWidget::on_btnSend_clicked()
{
    auto message = ui->lnMessage->text().trimmed();
    _client->sendMessage(message);
    ui->lnMessage->setText("");
    ui->lstMessages->addItem(message);
}

// Обработка получения текстового сообщения
void ClientChatWidget::textMessageReceived(QString message, QString receiver, QString sender)
{
    if (receiver == "Server" || receiver == "All") {
        ui->lstMessages->addItem(QString("%1: %2").arg(sender, message));
    }
    if(receiver != "Server"){
        emit textForOtherClients(message, receiver, sender);
    }
}

// Обработка события "печатает"
void ClientChatWidget::onTyping()
{
    emit isTyping(QString("%1 is typing...").arg(_client->name()));
}

// Обработка получения сообщения о начале отправки файла
void ClientChatWidget::onInitReceivingFile(QString clientName, QString fileName, qint64 fileSize)
{
    auto message = QString("Client (%1) wants to send a file. Do you want to accept it?\nFile Name:%2\nFile Size: %3 bytes")
    .arg(clientName, fileName)
        .arg(fileSize);
    auto result = QMessageBox::question(this, "Receiving File", message);
    if (result == QMessageBox::Yes) {
        _client->sendAcceptFile();
    } else {
        _client->sendRejectFile();
    }
}

// Обработка сохранения файла
void ClientChatWidget::onFileSaved(QString path)
{
    auto message = QString("File saved here:\n%1").arg(path);
    QMessageBox::information(this, "File saved", message);
}

// Обработка активации ссылки "Открыть папку"
void ClientChatWidget::on_lblOpenFolder_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(_client->name()));
}

// Обработка изменения имени клиента
void ClientChatWidget::onClientNameChanged(QString prevName, QString name)
{
    QFile::rename(dir.canonicalPath(), name);
    emit clientNameChanged(prevName, name);
}

QTcpSocket* ClientManager::socket() const {
    return _socket;
}

QTcpSocket* ClientChatWidget::client() const {
    return _client->socket();
}
