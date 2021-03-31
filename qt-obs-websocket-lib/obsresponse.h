#ifndef OBSAUTHINFO_H
#define OBSAUTHINFO_H

#include <QJsonObject>

struct OBSResponse {
  QString Status;
  QString MessageId;
  QString Error;
  OBSResponse(QJsonObject data) {
    MessageId = data["message-id"].toString();
    Status = data["status"].toString();
    Error = data["error"].toString();
  }
};

struct OBSAuthInfo : OBSResponse {
  bool AuthRequired;
  QString Challenge;
  QString PasswordSalt;

  OBSAuthInfo(QJsonObject data) : OBSResponse(data) {
    AuthRequired = data["authRequired"].toBool();
    Challenge = data["challenge"].toString();
    PasswordSalt = data["salt"].toString();
    MessageId = data["status"].toString();
  }
};

struct OBSVersion : OBSResponse {
  QString PluginVersion;
  QString OBSStudioVersion;
  QString SupportedImageExportFormats;

  OBSVersion(QJsonObject data) : OBSResponse(data) {
    OBSStudioVersion = data["obs-studio-version"].toString();
    PluginVersion = data["obs-websocket-version"].toString();
    SupportedImageExportFormats = data["supported-image-export-formats"].toString();
  }
};

#endif  // OBSAUTHINFO_H
