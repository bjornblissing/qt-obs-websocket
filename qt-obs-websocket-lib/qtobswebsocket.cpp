#include "qtobswebsocket.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QtCore/QDebug>

#include <array>

QtObsWebsocket::QtObsWebsocket() {
  connect(std::nullopt);
}

QtObsWebsocket::QtObsWebsocket(std::optional<QString> password) {
  connect(password);
}

QtObsWebsocket::QtObsWebsocket(QUrl url, std::optional<QString> password) : m_url(url) {
  connect(password);
}

QtObsWebsocket::QtObsWebsocket(QString ip_address, quint16 port, std::optional<QString> password) {
  m_url = QUrl(QString("ws://" + ip_address + ":" + QString::number(port)));
  connect(password);
}

QtObsWebsocket::~QtObsWebsocket() {
  disconnect();
}

void QtObsWebsocket::connect(std::optional<QString> password) {
  // Disconnect if already connected
  if (m_socket.state() == QAbstractSocket::ConnectedState) {
    m_socket.close();
  }

  QObject::connect(&m_socket, &QWebSocket::connected, this, &QtObsWebsocket::onConnected);
  QObject::connect(&m_socket,
                   &QWebSocket::textMessageReceived,
                   this,
                   &QtObsWebsocket::onTextMessageReceived);
  QObject::connect(&m_socket, &QWebSocket::disconnected, this, &QtObsWebsocket::onDisconnected);
  m_socket.open(m_url);

  // Wait for connection {
  {
    QEventLoop loop;
    QObject::connect(&m_socket, &QWebSocket::connected, &loop, &QEventLoop::quit);
    loop.exec();
  }

  OBSAuthInfo authInfo = getAuthInfo();

  if (authInfo.AuthRequired) {
    if (password) {
      m_authenticated = authenticate(password.value(), authInfo);
    }
  } else {
    m_authenticated = true;
  }
}

void QtObsWebsocket::disconnect() {
  m_socket.close();
}

const OBSVersion QtObsWebsocket::getVersion() {
  // No authentication needed for this command
  QJsonObject response = sendRequest("GetVersion");
  return OBSVersion(response);
}

const OBSAuthInfo QtObsWebsocket::getAuthInfo() {
  // No authentication needed for this command
  QJsonObject response = sendRequest("GetAuthRequired");
  return OBSAuthInfo(response);
}

const bool QtObsWebsocket::authenticate(QString password, OBSAuthInfo authInfo) {
  QString secret = hashEncode(password + authInfo.PasswordSalt);
  QString auth_response = hashEncode(secret + authInfo.Challenge);

  QJsonObject auth_request;
  auth_request["auth"] = auth_response;

  QJsonObject response_json = sendRequest("Authenticate", auth_request);
  OBSResponse response(response_json);

  if (response.Status == "ok") {
    return true;
  }

  return false;
}

void QtObsWebsocket::startRecording() {
  sendAuthenticatedCommand("StartRecording");
}

void QtObsWebsocket::stopRecording() {
  sendAuthenticatedCommand("StopRecording");
}

void QtObsWebsocket::startStreaming() {
  sendAuthenticatedCommand("StartStreaming");
}

void QtObsWebsocket::stopStreaming() {
  sendAuthenticatedCommand("StopStreaming");
}

void QtObsWebsocket::setCurrentProfile(QString profile) {
  QJsonObject parameters;
  parameters["profile-name"] = profile;
  sendAuthenticatedCommand("SetCurrentProfile", parameters);
}

void QtObsWebsocket::setCurrentScene(QString scene) {
  QJsonObject parameters;
  parameters["scene-name"] = scene;
  sendAuthenticatedCommand("SetCurrentScene", parameters);
}

const QString QtObsWebsocket::generateMessageId(const size_t length) {
  static std::array<char, 63> pool = {
    "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

  QString result{};
  for (size_t i = 0; i < length; ++i) {
    // Random char from the pool above. Minus one to omitt the null terminator
    size_t index = QRandomGenerator::global()->bounded(static_cast<quint32>(pool.size() - 1));
    result.append(pool[index]);
  }

  return result;
}

void QtObsWebsocket::onConnected() {
  qInfo() << "Connected";
}

void QtObsWebsocket::onDisconnected() {
  qInfo() << "Disconnected";
}

void QtObsWebsocket::onTextMessageReceived(QString message) {
  QJsonObject response = responseToJsonObject(message);

  if (response["message-id"].isString()) {
    const QString message_id = response["message-id"].toString();
    if (m_message_handler.find(message_id) != m_message_handler.end()) {
      m_response = response;
      m_message_handler.remove(message_id);
    } else {
      qWarning() << "Warning: No message with id: " << message_id << " have been sent!";
    }
  } else if (response["update-type"].isString()) {
    // Handle an event
    QString eventType = response["update-type"].toString();

    response.remove("message");
    response.remove("update-type");

    // TODO: Emit event
  }
}

const QString QtObsWebsocket::hashEncode(QString input) {
  QByteArray inputArray(input.toUtf8());
  QByteArray outputArray = QCryptographicHash::hash(inputArray, QCryptographicHash::Sha256);
  return outputArray.toBase64();
}

const QJsonObject QtObsWebsocket::responseToJsonObject(QString reponse) {
  QJsonDocument doc = QJsonDocument::fromJson(reponse.toUtf8());
  QJsonObject jsonResponseObject = doc.object();
  return jsonResponseObject;
}

const QString QtObsWebsocket::jsonObjectToString(QJsonObject request) {
  QJsonDocument doc(request);
  return QString(doc.toJson(QJsonDocument::Compact));
}

QJsonObject QtObsWebsocket::sendRequest(QString request_type,
                                        std::optional<QJsonObject> request_parameters) {
  const QString message_id = [&]() -> QString {
    QString new_message_id;
    do {
      new_message_id = this->generateMessageId();
    } while (m_message_handler.find(new_message_id) != m_message_handler.end());
    return new_message_id;
  }();

  QJsonObject requestBody;
  requestBody["request-type"] = request_type;
  requestBody["message-id"] = message_id;

  if (request_parameters != std::nullopt) {
    QStringList keys = request_parameters->keys();
    for (const auto& key : keys) {
      requestBody[key] = request_parameters->value(key);
    }
  }

  m_message_handler.insert(message_id, QJsonObject());
  QString message = jsonObjectToString(requestBody);
  m_socket.sendTextMessage(message);

  // Wait for response
  {
    QEventLoop loop;
    QObject::connect(&m_socket, &QWebSocket::textMessageReceived, &loop, &QEventLoop::quit);
    loop.exec();
  }

  return m_response;
}

void QtObsWebsocket::sendAuthenticatedCommand(QString command,
                                              std::optional<QJsonObject> parameters) {
  if (m_authenticated) {
    QJsonObject responseObject = sendRequest(command, parameters);
    OBSResponse obs_response(responseObject);
    if (obs_response.Status == "error") {
      qWarning() << "Error: " << obs_response.Error;
    }
  } else {
    qWarning() << "Not authenticated";
  }
}
