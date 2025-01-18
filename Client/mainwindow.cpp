#include "ChatItemWidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

// Конструктор класса MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this); // Инициализация пользовательского интерфейса
    setupClient(); // Настройка клиента
}

// Деструктор класса MainWindow
MainWindow::~MainWindow()
{
    delete ui; // Освобождение памяти
}

// Метод для настройки клиента
void MainWindow::setupClient()
{
    _client = new ClientManager();

    // Подключение сигналов и слотов для различных событий клиента
    connect(_client, &ClientManager::connected, [this](){
        ui->centralwidget->setEnabled(true); // Включение центрального виджета при подключении
    });
    connect(_client, &ClientManager::disconnected, [this](){
        ui->centralwidget->setEnabled(false); // Отключение центрального виджета при отключении
    });
    connect(_client, &ClientManager::textMessageReceived, this, &MainWindow::dataReceived);
    connect(_client, &ClientManager::isTyping, this, &MainWindow::onTyping);
    connect(_client, &ClientManager::initReceivingFile, this, &MainWindow::onInitReceivingFile);
    connect(_client, &ClientManager::rejectReceivingFile, this, &MainWindow::onRejectReceivingFile);
    connect(ui->lnMessage, &QLineEdit::textChanged, _client, &ClientManager::sendIsTyping);
    connect(_client, &ClientManager::connectionACK, this, &MainWindow::onConnectionACK);
    connect(_client, &ClientManager::newClientConnectedToServer, this, &MainWindow::onNewClientConnectedToServer);
    connect(_client, &ClientManager::clientDisconnected, this, &MainWindow::onClientDisconnected);
    connect(_client, &ClientManager::clientNameChanged, this, &MainWindow::onClientNameChanged);
}

// Метод для обработки действия "Подключиться"
void MainWindow::on_actionConnect_triggered()
{
    _client->connectToServer(); // Подключение к серверу
}

// Метод для обработки нажатия кнопки "Отправить"
void MainWindow::on_btnSend_clicked()
{
    auto message = ui->lnMessage->text().trimmed(); // Получение и обрезка текста сообщения
    auto receiver = ui->cmbDestination->currentText();
    _client->sendMessage(message, receiver); // Отправка сообщения
    ui->lnMessage->setText(""); // Очистка поля ввода
    ui->lnMessage->setFocus(); // Установка фокуса на поле ввода

    // Добавление сообщения в список сообщений
    auto chatWidget = new ChatItemWidget();
    auto sender = "You";
    chatWidget->setMessage(message, sender, receiver, true); // Устанавливаем отправителя и получателя
    auto listWidgetItem = new QListWidgetItem();
    listWidgetItem->setSizeHint(QSize(0, 65));
    ui->lstMessages->addItem(listWidgetItem);
    ui->lstMessages->setItemWidget(listWidgetItem, chatWidget);
}

// Метод для обработки получения данных
void MainWindow::dataReceived(QString message, QString sender)
{
    auto chatWidget = new ChatItemWidget();
    auto receiver = "You";
    chatWidget->setMessage(message, sender, receiver); // Устанавливаем отправителя и получателя
    auto listWidgetItem = new QListWidgetItem();
    listWidgetItem->setSizeHint(QSize(0, 65));
    ui->lstMessages->addItem(listWidgetItem);
    listWidgetItem->setBackground(QColor(167, 255, 237)); // Установка фона для полученного сообщения
    ui->lstMessages->setItemWidget(listWidgetItem, chatWidget);
}

// Метод для обработки завершения редактирования имени клиента
void MainWindow::on_lnClientName_editingFinished()
{
    auto name = ui->lnClientName->text().trimmed(); // Получение и обрезка имени клиента
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name", "Name cannot be empty."); // Показать сообщение об ошибке
        return; // Не отправляем пустое имя
    }
    _client->sendName(name); // Отправка имени клиента
    _client->setClientName(name); // Устанавливаем имя клиента в свойство сокета
    setWindowTitle(name); // Изменение заголовка окна
}

// Метод для обработки изменения статуса в комбобоксе
void MainWindow::on_cmbStatus_currentIndexChanged(int index)
{
    auto status = static_cast<ChatProtocol::Status>(index); // Преобразование индекса в статус
    _client->sendStatus(status); // Отправка статуса
}

// Метод для обработки события "Печатает"
void MainWindow::onTyping()
{
    statusBar()->showMessage("Server is typing...", 750); // Отображение сообщения о том, что сервер печатает
}

// Метод для обработки нажатия кнопки "Отправить файл"
void MainWindow::on_btnSendFile_clicked()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Select a file", "/home"); // Открытие диалогового окна для выбора файла
    _client->sendInitSendingFile(fileName); // Инициализация отправки файла
}

// Метод для обработки отклонения получения файла
void MainWindow::onRejectReceivingFile()
{
    QMessageBox::critical(this, "Sending File", "Operation rejected..."); // Отображение сообщения об ошибке
}

// Метод для обработки инициализации получения файла
void MainWindow::onInitReceivingFile(QString clientName, QString fileName, qint64 fileSize)
{
    auto message = QString("Client (%1) wants to send a file. Do you want to accept it?\nFile Name:%2\nFile Size: %3 bytes")
    .arg(clientName, fileName)
        .arg(fileSize); // Формирование сообщения о получении файла
    auto result = QMessageBox::question(this, "Receiving File", message); // Отображение вопроса о принятии файла
    if (result == QMessageBox::Yes) {
        _client->sendAcceptFile(); // Принятие файла
    } else {
        _client->sendRejectFile(); // Отклонение файла
    }
}

// Метод для обработки подтверждения подключения
void MainWindow::onConnectionACK(QString myName, QStringList clientsName)
{
    ui->cmbDestination->clear(); // Очистка списка получателей
    clientsName.prepend("All"); // Добавление опции "Все"
    clientsName.prepend("Server"); // Добавление опции "Сервер"
    foreach (auto client, clientsName) {
        ui->cmbDestination->addItem(client); // Добавление клиента в список получателей
    }
    setWindowTitle(myName); // Установка заголовка окна
}

// Метод для обработки нового подключения клиента к серверу
void MainWindow::onNewClientConnectedToServer(QString clientName)
{
    ui->cmbDestination->addItem(clientName); // Добавление нового клиента в список получателей
}

// Метод для обработки изменения имени клиента
void MainWindow::onClientNameChanged(QString prevName, QString clientName)
{
    for (int i = 0; i < ui->cmbDestination->count(); ++i) {
        if (ui->cmbDestination->itemText(i) == prevName) {
            ui->cmbDestination->setItemText(i, clientName); // Изменение имени клиента в списке получателей
            return;
        }
    }
}

// Метод для обработки отключения клиента
void MainWindow::onClientDisconnected(QString clientName)
{
    for (int i = 0; i < ui->cmbDestination->count(); ++i) {
        if (ui->cmbDestination->itemText(i) == clientName) {
            ui->cmbDestination->removeItem(i); // Удаление клиента из списка получателей
            return;
        }
    }
}

// Конструктор класса ClientManager
ClientManager::ClientManager(QHostAddress ip, ushort port, QObject *parent)
    : QObject{parent},
    _ip(ip),
    _port(port)
{
    setupClient(); // Настройка клиента
}

// Метод для подключения к серверу
void ClientManager::connectToServer()
{
    _socket->connectToHost(_ip, _port); // Подключение к хосту
}

// Метод для отправки сообщения
void ClientManager::sendMessage(QString message, QString receiver)
{
    QString sender = _socket->property("clientName").toString();
    if (_socket->property("isServer").toBool()) {
        sender = "Server";
    }
    _socket->write(_protocol.textMessage(message, receiver, sender)); // Отправка текстового сообщения с отправителем
}

// Метод для отправки имени
void ClientManager::sendName(QString name)
{
    if (_socket->property("isServer").toBool()) {
        _socket->setProperty("clientName", "Server");
    } else {
        _socket->setProperty("clientName", name);
    }
    _socket->write(_protocol.setNameMessage(name));
}

// Метод для установки имени клиента
void ClientManager::setClientName(const QString &name)
{
    _socket->setProperty("clientName", name); // Устанавливаем имя клиента в свойство сокета
}

// Метод для отправки статуса
void ClientManager::sendStatus(ChatProtocol::Status status)
{
    _socket->write(_protocol.setStatusMessage(status)); // Отправка сообщения со статусом
}

// Метод для отправки информации о том, что пользователь печатает
void ClientManager::sendIsTyping()
{
    _socket->write(_protocol.isTypingMessage()); // Отправка сообщения о том, что пользователь печатает
}

// Метод для инициализации отправки файла
void ClientManager::sendInitSendingFile(QString fileName)
{
    _tmpFileName = fileName; // Сохранение имени файла
    _socket->write(_protocol.setInitSendingFileMessage(fileName)); // Отправка сообщения с инициализацией отправки файла
}

// Метод для принятия файла
void ClientManager::sendAcceptFile()
{
    _socket->write(_protocol.setAcceptFileMessage()); // Отправка сообщения о принятии файла
}

// Метод для отклонения файла
void ClientManager::sendRejectFile()
{
    _socket->write(_protocol.setRejectFileMessage()); // Отправка сообщения об отклонении файла
}

// Метод для обработки готовности чтения данных
void ClientManager::readyRead()
{
    auto data = _socket->readAll(); // Чтение всех данных из сокета
    _protocol.loadData(data); // Загрузка данных в протокол
    switch (_protocol.type()) { // Обработка типа сообщения
    case ChatProtocol::Text:
        emit textMessageReceived(_protocol.message(), _protocol.sender()); // Сигнал о получении текстового сообщения с отправителем
        break;
    case ChatProtocol::SetName:
        emit nameChanged(_protocol.name()); // Сигнал об изменении имени
        break;
    case ChatProtocol::SetStatus:
        emit statusChanged(_protocol.status()); // Сигнал об изменении статуса
        break;
    case ChatProtocol::IsTyping:
        emit isTyping(); // Сигнал о том, что кто-то печатает
        break;
    case ChatProtocol::InitSendingFile:
        emit initReceivingFile(_protocol.name(), _protocol.fileName(), _protocol.fileSize()); // Сигнал об инициализации получения файла
        break;
    case ChatProtocol::AcceptSendingFile:
        sendFile(); // Отправка файла
        break;
    case ChatProtocol::RejectSendingFile:
        emit rejectReceivingFile(); // Сигнал об отклонении получения файла
        break;
    case ChatProtocol::ConnectionACK:
        emit connectionACK(_protocol.myName(), _protocol.clientsName()); // Сигнал о подтверждении подключения
        break;
    case ChatProtocol::NewClient:
        emit newClientConnectedToServer(_protocol.clientName()); // Сигнал о новом подключении клиента
        break;
    case ChatProtocol::ClientDisconnected:
        emit clientDisconnected(_protocol.clientName()); // Сигнал об отключении клиента
        break;
    case ChatProtocol::ClientName:
        emit clientNameChanged(_protocol.prevName(), _protocol.clientName()); // Сигнал об изменении имени клиента
        break;
    default:
        break;
    }
}

// Метод для настройки клиента
void ClientManager::setupClient()
{
    _socket = new QTcpSocket(this); // Создание сокета
    connect(_socket, &QTcpSocket::connected, this, &ClientManager::connected); // Подключение сигнала о подключении
    connect(_socket, &QTcpSocket::disconnected, this, &ClientManager::disconnected); // Подключение сигнала об отключении
    connect(_socket, &QTcpSocket::readyRead, this, &ClientManager::readyRead); // Подключение сигнала о готовности чтения данных
}

// Метод для отправки файла
void ClientManager::sendFile()
{
    _socket->write(_protocol.setFileMessage(_tmpFileName)); // Отправка сообщения с файлом
}
