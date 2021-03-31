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

OBSVersion QtObsWebsocket::getVersion() {
  // No authentication needed for this command
  QJsonObject response = sendRequest("GetVersion");
  return OBSVersion(response);
}

OBSAuthInfo QtObsWebsocket::getAuthInfo() {
  // No authentication needed for this command
  QJsonObject response = sendRequest("GetAuthRequired");
  return OBSAuthInfo(response);
}

bool QtObsWebsocket::authenticate(QString password, OBSAuthInfo authInfo) {
  QString secret = hashEncode(password + authInfo.PasswordSalt);
  QString authResponse = hashEncode(secret + authInfo.Challenge);

  QJsonObject auth_request;
  auth_request["auth"] = authResponse;

  QJsonObject responseJson = sendRequest("Authenticate", auth_request);
  OBSResponse response(responseJson);

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

QString QtObsWebsocket::generateMessageId(const size_t length) {
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
  m_response = message;
}

QString QtObsWebsocket::hashEncode(QString input) {
  QByteArray inputArray(input.toUtf8());
  QByteArray outputArray = QCryptographicHash::hash(inputArray, QCryptographicHash::Sha256);
  return outputArray.toBase64();
}

QJsonObject QtObsWebsocket::responseToJsonObject(QString reponse) {
  QJsonDocument doc = QJsonDocument::fromJson(reponse.toUtf8());
  QJsonObject jsonResponseObject = doc.object();
  return jsonResponseObject;
}

QString QtObsWebsocket::jsonObjectToString(QJsonObject request) {
  QJsonDocument doc(request);
  return QString(doc.toJson(QJsonDocument::Compact));
}

QJsonObject QtObsWebsocket::sendRequest(QString requestType,
                                        std::optional<QJsonObject> requestParameters) {
  const QString messageId = generateMessageId();

  QJsonObject requestBody;
  requestBody["request-type"] = requestType;
  requestBody["message-id"] = messageId;

  if (requestParameters != std::nullopt) {
    QStringList keys = requestParameters->keys();
    for (const auto& key : keys) {
      requestBody[key] = requestParameters->value(key);
    }
  }

  QString message = jsonObjectToString(requestBody);
  m_socket.sendTextMessage(message);

  // Wait for response
  {
    QEventLoop loop;
    QObject::connect(&m_socket, &QWebSocket::textMessageReceived, &loop, &QEventLoop::quit);
    loop.exec();
  }

  QString response = m_response;
  m_response.clear();

  return responseToJsonObject(response);
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
