#include "mitakuuluu.h"
#include "constants.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QDesktopServices>
#include <QImage>
#include <QImageReader>
#include <QTransform>
#include <QGuiApplication>
#include <QClipboard>
#include <QStandardPaths>

Mitakuuluu::Mitakuuluu(QObject *parent): QObject(parent)
{

    bool ret =
            QDBusConnection::sessionBus().registerService(SERVICE_NAME) &&
            QDBusConnection::sessionBus().registerObject(OBJECT_NAME,
                                                         this,
                                                         QDBusConnection::ExportScriptableSlots);
    if (ret) {
        nam = new QNetworkAccessManager(this);
        _pendingJid = QString();
        connStatus = Unknown;
        translator = 0;

        QStringList locales;
        QStringList localeNames;
        QString baseName("/usr/share/harbour-mitakuuluu2/locales/");
        QDir localesDir(baseName);
        if (localesDir.exists()) {
            locales = localesDir.entryList(QStringList() << "*.qm", QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
            qDebug() << "available translations:" << locales;
        }
        foreach (const QString &locale, locales) {
            localeNames << QString("%1 (%2)")
                                   .arg(QLocale::languageToString(QLocale(locale).language()))
                                   .arg(QLocale::countryToString(QLocale(locale).country()));
        }
        _locales =  locales.isEmpty() ? QStringList() << "en.qm" : locales;
        _localesNames = localeNames.isEmpty() ? QStringList() << "Engineering english" : localeNames;

        settings = new QSettings("coderus", "mitakuuluu2", this);
        _currentLocale = settings->value("settings/locale", QString("%1.qm").arg(QLocale::system().name().split(".").first())).toString();
        setLocale(_currentLocale);

        qDebug() << "Connecting to DBus signals";
        iface = new QDBusInterface(SERVER_SERVICE,
                                   SERVER_PATH,
                                   SERVER_INTERFACE,
                                   QDBusConnection::sessionBus(),
                                   this);
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "messageReceived", this, SIGNAL(messageReceived(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "disconnected", this, SIGNAL(disconnected(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "authFail", this, SIGNAL(authFail(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "authSuccess", this, SIGNAL(authSuccess(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "networkAvailable", this, SIGNAL(networkChanged(bool)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "noAccountData", this, SIGNAL(noAccountData()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "messageStatusUpdated", this, SIGNAL(messageStatusUpdated(QString,QString,int)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "pictureUpdated", this, SIGNAL(pictureUpdated(QString,QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactsChanged", this, SIGNAL(contactsChanged()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "notificationOpenJid", this, SIGNAL(notificationOpenJid(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "setUnread", this, SLOT(onSetUnread(QString,int)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "myAccount", this, SLOT(onMyAccount(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "pushnameUpdated", this, SIGNAL(pushnameUpdated(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactAvailable", this, SIGNAL(presenceAvailable(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactUnavailable", this, SIGNAL(presenceUnavailable(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactLastSeen", this, SIGNAL(presenceLastSeen(QString, int)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactChanged", this, SLOT(onContactChanged(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactSynced", this, SIGNAL(contactSynced(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "newGroupSubject", this, SIGNAL(newGroupSubject(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "downloadProgress", this, SIGNAL(mediaDownloadProgress(QString, QString, )));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "mediaDownloadFinished", this, SIGNAL(mediaDownloadFinished(QString, QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "downloadFailed", this, SIGNAL(mediaDownloadFailed(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "connectionStatusChanged", this, SIGNAL(onConnectionStatusChanged(int)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupParticipant", this, SIGNAL(groupParticipant(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupInfo", this, SIGNAL(groupInfo(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "registered", this, SIGNAL(registered()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "smsTimeout", this, SIGNAL(smsTimeout(int)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "registrationFailed", this, SIGNAL(registrationFailed(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "existsRequestFailed", this, SIGNAL(existsRequestFailed(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "registrationComplete", this, SIGNAL(registrationComplete()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "codeRequestFailed", this, SIGNAL(codeRequestFailed(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "codeRequested", this, SIGNAL(codeRequested(QVariantMap)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "accountExpired", this, SIGNAL(accountExpired(QVariantMap)));;
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupCreated", this, SLOT(groupCreated(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupParticipantAdded", this, SIGNAL(participantAdded(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupParticipantRemoved", this, SIGNAL(participantRemoved(QString, QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactsBlocked", this, SIGNAL(contactsBlocked(QStringList)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactTyping", this, SIGNAL(contactTyping(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "contactPaused", this, SIGNAL(contactPaused(QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "synchronizationFinished", this, SIGNAL(synchronizationFinished()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "synchronizationFailed", this, SIGNAL(synchronizationFailed()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "uploadFailed", this, SIGNAL(uploadMediaFailed(QString,QString)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "groupsMuted", this, SIGNAL(groupsMuted(QStringList)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "dissectError", this, SIGNAL(dissectError()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "logfileReady", this, SIGNAL(logfileReady(QByteArray, bool)));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "pong", this, SLOT(onServerPong()));
        QDBusConnection::sessionBus().connect(SERVER_SERVICE, SERVER_PATH, SERVER_INTERFACE,
                                              "simParameters", this, SLOT(onSimParameters(QString, QString)));
        qDebug() << "Start pinging server";
        pingServer = new QTimer(this);
        QObject::connect(pingServer, SIGNAL(timeout()), this, SLOT(doPingServer()));
        pingServer->setInterval(30000);
        pingServer->setSingleShot(false);
        pingServer->start();
        qDebug() << "Sending ready to daemon";
        iface->call(QDBus::NoBlock, "ready");
    }
    else {
        QGuiApplication::exit(1);
    }
}

Mitakuuluu::~Mitakuuluu()
{
    if (connStatus == LoggedIn)
        setPresenceUnavailable();
}

int Mitakuuluu::connectionStatus()
{
    return connStatus;
}

QString Mitakuuluu::connectionString()
{
    return connString;
}

QString Mitakuuluu::mcc()
{
    return _mcc;
}

QString Mitakuuluu::mnc()
{
    return _mnc;
}

int Mitakuuluu::totalUnread()
{
    return _totalUnread;
}

QString Mitakuuluu::myJid()
{
    return _myJid;
}

void Mitakuuluu::onConnectionStatusChanged(int status)
{
    connStatus = status;
    Q_EMIT connectionStatusChanged();
    switch (connStatus) {
    default:
        connString = tr("Unknown", "Unknown connection status");
        break;
    case WaitingForConnection:
        connString = tr("Waiting for connection", "Waiting for connection connection status");
        break;
    case Connecting:
        connString = tr("Connecting...", "Connecting connection status");
        break;
    case Connected:
        connString = tr("Authentication...", "Authentication connection status");
        break;
    case LoggedIn:
        connString = tr("Logged in", "Logged in connection status");
        break;
    case LoginFailure:
        connString = tr("Login failed!", "Login failed connection status");
        break;
    case Disconnected:
        connString = tr("Disconnected", "Disconnected connection status");
        break;
    case Registering:
        connString = tr("Registering...", "Registering connection status");
        break;
    case RegistrationFailed:
        connString = tr("Registration failed!", "Registration failed connection status");
        break;
    case AccountExpired:
        connString = tr("Account expired!", "Account expired connection status");
        break;
    }
    Q_EMIT connectionStringChanged();
}

void Mitakuuluu::authenticate()
{
    if (iface)
        iface->call(QDBus::NoBlock, "recheckAccountAndConnect");
}

void Mitakuuluu::init()
{
    if (iface)
        iface->call(QDBus::NoBlock, "init");
}

void Mitakuuluu::disconnect()
{
    if (iface)
        iface->call(QDBus::NoBlock, "disconnect");
}

void Mitakuuluu::sendMessage(const QString &jid, const QString &message, const QString &media, const QString &mediaData)
{
    if (iface)
        iface->call(QDBus::NoBlock, "sendMessage", jid, message, media, mediaData);
}

void Mitakuuluu::sendBroadcast(const QStringList &jids, const QString &message)
{
    if (iface)
        iface->call(QDBus::NoBlock, "broadcastSend", jids, message);
}

void Mitakuuluu::sendText(const QString &jid, const QString &message)
{
    if (iface)
        iface->call(QDBus::NoBlock, "sendText", jid, message);
}

void Mitakuuluu::syncContactList()
{
    if (iface)
        iface->call(QDBus::NoBlock, "synchronizeContacts");
}

void Mitakuuluu::setActiveJid(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "setActiveJid", jid);
    Q_EMIT activeJidChanged(jid);
}

QString Mitakuuluu::shouldOpenJid()
{
    return _pendingJid;
}

void Mitakuuluu::startTyping(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "startTyping", jid);
}

void Mitakuuluu::endTyping(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "endTyping", jid);
}

void Mitakuuluu::downloadMedia(const QString &msgId, const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "downloadMedia", msgId, jid);
}

void Mitakuuluu::cancelDownload(const QString &msgId, const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "cancelDownload", msgId, jid);
}

void Mitakuuluu::abortMediaDownload(const QString &msgId, const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "cancelDownload", msgId, jid);
}

QString Mitakuuluu::openVCardData(const QString &name, const QString &data)
{
    QString path = QString("%1/%2.vcf").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                       .arg(name);
    QFile file(path);
    if (file.exists())
        file.remove();
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        file.write(data.toUtf8());
        file.close();
    }
    return path;
}

void Mitakuuluu::getParticipants(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "getParticipants", jid);
}

void Mitakuuluu::getGroupInfo(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "getGroupInfo", jid);
}

void Mitakuuluu::regRequest(const QString &cc, const QString &phone, const QString &method, const QString &password)
{
    if (iface)
        iface->call(QDBus::NoBlock, "regRequest", cc, phone, method, password);
}

void Mitakuuluu::enterCode(const QString &cc, const QString &phone, const QString &code)
{
    if (iface)
        iface->call(QDBus::NoBlock, "enterCode", cc, phone, code);
}

void Mitakuuluu::setGroupSubject(const QString &gjid, const QString &subject)
{
    if (iface)
        iface->call(QDBus::NoBlock, "setGroupSubject", gjid, subject);
}

void Mitakuuluu::createGroup(const QString &subject, const QString &picture, const QStringList &participants)
{
    if (iface) {
        _pendingAvatar = picture;
        _pendingParticipants = participants;
        iface->call(QDBus::NoBlock, "createGroupChat", subject);
    }
}

void Mitakuuluu::groupLeave(const QString &gjid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "requestLeaveGroup", gjid);
}

void Mitakuuluu::setPicture(const QString &jid, const QString &path)
{
    if (iface)
        iface->call(QDBus::NoBlock, "setPicture", jid, path);
}

void Mitakuuluu::removeParticipant(const QString &gjid, const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "removeGroupParticipant", gjid, jid);
}

void Mitakuuluu::addParticipant(const QString &gjid, const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "addGroupParticipant", gjid, jid);
}

void Mitakuuluu::addParticipants(const QString &gjid, const QStringList &jids)
{
    if (iface)
        iface->call(QDBus::NoBlock, "addGroupParticipants", gjid, jids);
}

void Mitakuuluu::refreshContact(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "refreshContact", jid);
}

QString Mitakuuluu::transformPicture(const QString &filename, const QString &jid, int posX, int posY, int sizeW, int sizeH, int maxSize, int rotation)
{
    qDebug() << "Preparing picture" << filename << "- rotation:" << QString::number(rotation);
    QString image = filename;
    image = image.replace("file://","");

    QImage preimg(image);
    qDebug() << "img" << "w:" << QString::number(preimg.width()) << "h:" << QString::number(preimg.height());
    qDebug() << "posx:" << QString::number(posX) << "posy:" << QString::number(posY);
    qDebug() << "sizeW:" << QString::number(sizeW) << "sizeH:" << QString::number(sizeH) << "maxSize:" << maxSize;

    if (sizeW == sizeH) {
        preimg = preimg.copy(posX,posY,sizeW,sizeH);
        if (sizeW > maxSize)
            preimg = preimg.scaledToWidth(maxSize, Qt::SmoothTransformation);
    }
    if (rotation != 0) {
        QTransform rot;
        rot.rotate(rotation);
        preimg = preimg.transformed(rot);
    }
    if (sizeW != sizeH) {
        preimg = preimg.copy(posX,posY,sizeW,sizeH);
        if (sizeW > maxSize)
            preimg = preimg.scaledToWidth(maxSize, Qt::SmoothTransformation);
    }
    QString destDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatar";
    QDir dir(destDir);
    if (!dir.exists())
        dir.mkpath(destDir);
    QString path = QString("%1/%2").arg(destDir)
                                   .arg(jid);
    qDebug() << "Saving to:" << path << (preimg.save(path, "JPG", 90) ? "success" : "error");

    return path;
}

void Mitakuuluu::copyToClipboard(const QString &text)
{
    QClipboard *clip = QGuiApplication::clipboard();
    clip->setText(text);
}

void Mitakuuluu::blockOrUnblockContact(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "blockOrUnblockContact", jid);
}

void Mitakuuluu::sendBlockedJids(const QStringList &jids)
{
    if (iface)
        iface->call(QDBus::NoBlock, "sendBlockedJids", jids);
}

void Mitakuuluu::muteOrUnmuteGroup(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "muteOrUnmuteGroup", jid);
}

void Mitakuuluu::muteGroups(const QStringList &jids)
{
    if (iface)
        iface->call(QDBus::NoBlock, "muteGroups", jids);
}

void Mitakuuluu::getPrivacyList()
{
    if (iface)
        iface->call(QDBus::NoBlock, "getPrivacyList");
}

void Mitakuuluu::getMutedGroups()
{
    if (iface)
        iface->call(QDBus::NoBlock, "getMutedGroups");
}

void Mitakuuluu::forwardMessage(const QStringList &jids, const QString &jid, const QString &msgid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "forwardMessage", jids, jid, msgid);
}

void Mitakuuluu::setMyPushname(const QString &pushname)
{
    if (iface)
        iface->call(QDBus::NoBlock, "changeUserName", pushname);
}

void Mitakuuluu::setMyPresence(const QString &presence)
{
    if (iface)
        iface->call(QDBus::NoBlock, "changeStatus", presence);
}

void Mitakuuluu::sendRecentLogs()
{
    if (iface)
        iface->call(QDBus::NoBlock, "sendRecentLogs");
}

void Mitakuuluu::shutdown()
{
    pingServer->stop();
    if (iface)
        iface->call(QDBus::NoBlock, "exit");
    system("killall -9 harbour-mitakuuluu2-server");
    system("killall -9 harbour-mitakuuluu2");
}

void Mitakuuluu::isCrashed()
{
    if (iface) {
        QDBusPendingCall async = iface->asyncCall("isCrashed");
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
        if (watcher->isFinished()) {
           onReplyCrashed(watcher);
        }
        else {
            QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),this, SLOT(onReplyCrashed(QDBusPendingCallWatcher*)));
        }
    }
}

void Mitakuuluu::requestLastOnline(const QString &jid)
{
    if (iface)
        iface->call(QDBus::NoBlock, "requestQueryLastOnline", jid);
}

void Mitakuuluu::addPhoneNumber(const QString &name, const QString &phone)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "newContactAdd", name, phone);
    }
}

void Mitakuuluu::onReplyCrashed(QDBusPendingCallWatcher *call)
{
    bool value = false;
    QDBusPendingReply<bool> reply = *call;
    if (reply.isError()) {
        qDebug() << "error:" << reply.error().name() << reply.error().message();
    } else {
        value = reply.argumentAt<0>();
    }
    Q_EMIT replyCrashed(value);
    call->deleteLater();
}

void Mitakuuluu::onSimParameters(const QString &mcccode, const QString &mnccode)
{
    _mcc = mcccode;
    Q_EMIT mccChanged();
    _mnc = mnccode;
    Q_EMIT mncChanged();
}

void Mitakuuluu::onSetUnread(const QString &jid, int count)
{
    _unreadCount[jid] = count;
    _totalUnread = 0;
    foreach (int unread, _unreadCount.values())
        _totalUnread += unread;
    Q_EMIT setUnread(jid, count);
    Q_EMIT totalUnreadChanged();
}

void Mitakuuluu::onMyAccount(const QString &jid)
{
    _myJid = jid;
    Q_EMIT myJidChanged();
}

void Mitakuuluu::doPingServer()
{
    if (iface)
        iface->call(QDBus::NoBlock, "ping");
}

void Mitakuuluu::onServerPong()
{

}

void Mitakuuluu::groupCreated(const QString &gjid)
{
    qDebug() << "Group created:" << gjid;
    _pendingGroup = gjid;
}

void Mitakuuluu::onContactChanged(const QVariantMap &data)
{
    if (data["jid"].toString() == _pendingGroup) {
        if (!_pendingAvatar.isEmpty()) {
            setPicture(_pendingGroup, _pendingAvatar);
        }
        if (!_pendingParticipants.isEmpty()) {
            addParticipants(_pendingGroup, _pendingParticipants);
        }
        _pendingAvatar.clear();
        _pendingParticipants.clear();
        _pendingGroup.clear();
    }
    Q_EMIT contactChanged(data);
}

void Mitakuuluu::exit()
{
    qDebug() << "Remote command requested exit";
    QGuiApplication::exit(0);
}

void Mitakuuluu::notificationCallback(const QString &jid)
{
    _pendingJid = jid;
    Q_EMIT notificationOpenJid(jid);
}


void Mitakuuluu::sendMedia(const QStringList &jids, const QString &path)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "sendMedia", jids, path);
    }
}

void Mitakuuluu::sendVCard(const QStringList &jids, const QString &name, const QString &data)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "sendVCard", jids, name, data);
    }
}

QString Mitakuuluu::rotateImage(const QString &path, int rotation)
{
    if (rotation == 0)
        return path;
    QString fname = path;
    fname = fname.replace("file://", "");
    if (QFile(fname).exists()) {
        qDebug() << "rotateImage" << fname << QString::number(rotation);
        QImage img(fname);
        QTransform rot;
        rot.rotate(rotation);
        img = img.transformed(rot);
        fname = fname.split("/").last();
        fname = QString("%1/%2-%3").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                   .arg(QDateTime::currentDateTime().toTime_t())
                                   .arg(fname);
        qDebug() << "destination:" << fname;
        if (img.save(fname))
            return fname;
        else
            return QString();
    }
    return QString();
}

QString Mitakuuluu::saveVoice(const QString &path)
{
    //TODO: remove it
    qDebug() << "Requested to save" << path << "to gallery";
    QString savePath = QDir::homePath() + "/Music/Mitakuuluu";
    if (!path.contains(savePath)) {
        QString cutpath = path;
        cutpath = cutpath.replace("file://", "");
        QFile old(cutpath);
        if (old.exists()) {
            QString fname = cutpath.split("/").last();
            QString destination = QString("%1/%2").arg(savePath).arg(fname);
            old.copy(cutpath, destination);
            return destination;
        }
    }
    return path;
}

QString Mitakuuluu::saveImage(const QString &path)
{
    qDebug() << "Requested to save" << path << "to gallery";
    QString images = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    if (!path.contains(images)) {
        QString cutpath = path;
        cutpath = cutpath.replace("file://", "");
        QFile img(cutpath);
        if (img.exists()) {
            qDebug() << "saveImage" << path;
            QString name = path.split("/").last().split("@").first();
            img.open(QFile::ReadOnly);
            QString ext = "jpg";
            img.seek(1);
            QByteArray buf = img.read(3);
            if (buf == "PNG")
                ext = "png";
            img.close();
            QString destination = QString("%1/%2.%3").arg(images).arg(name).arg(ext);
            img.copy(cutpath, destination);
            qDebug() << "destination:" << destination;
            return name;
        }
    }
    return path;
}

void Mitakuuluu::openProfile(const QString &name, const QString &phone, const QString avatar)
{
    QFile tmp(QString("%1/_persecute-%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                         .arg(phone));
    if (tmp.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&tmp);
        out << "BEGIN:VCARD\n";
        out << "VERSION:3.0\n";
        out << "FN:" << name << "\n";
        out << "N:" << name << "\n";
        out << "TEL:" << phone << "\n";
        if (!avatar.isEmpty()) {

        }
        out << "END:VCARD";
        tmp.close();

        QDesktopServices::openUrl(tmp.fileName());
    }
}

void Mitakuuluu::removeAccount()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "removeAccount");
    }
}

void Mitakuuluu::syncContacts(const QStringList &numbers, const QStringList &names, const QStringList &avatars)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "syncContacts", numbers, names, avatars);
    }
}

void Mitakuuluu::setPresenceAvailable()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "setPresenceAvailable");
    }
}

void Mitakuuluu::setPresenceUnavailable()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "setPresenceUnavailable");
    }
}

void Mitakuuluu::syncAllPhonebook()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "synchronizePhonebook");
    }
}

void Mitakuuluu::removeAccountFromServer()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "removeAccountFromServer");
    }
}

void Mitakuuluu::forceConnection()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "forceConnection");
    }
}

void Mitakuuluu::setLocale(const QString &localeName)
{
    if (translator) {
        QGuiApplication::removeTranslator(translator);
        delete translator;
        translator = 0;
    }

    QString locale = localeName.split(".").first();

    translator = new QTranslator(this);

    qDebug() << "Loading translation:" << locale;
    if (translator->load(locale, "/usr/share/harbour-mitakuuluu2/locales", QString(), ".qm")) {
        qDebug() << "Translator loaded";
        qDebug() << (QGuiApplication::installTranslator(translator) ? "Translator installed" : "Error installing translator");
    }
    else {
        qDebug() << "Translation not available";
    }
}

void Mitakuuluu::setLocale(int index)
{
    QString locale = _locales[index];
    settings->setValue("settings/locale", locale);
    setLocale(locale);
}

int Mitakuuluu::getExifRotation(const QString &image)
{
    if ((image.toLower().endsWith("jpg") || image.toLower().endsWith("jpeg")) && QFile(image).exists()) {
        ExifData *ed;
        ed = exif_data_new_from_file(image.toLocal8Bit().data());
        if (!ed) {
            qDebug() << "File not readable or no EXIF data in file" << image;
        }
        else {
            ExifEntry *entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
            if (entry) {
                char buf[1024];

                /* Get the contents of the tag in human-readable form */
                exif_entry_get_value(entry, buf, sizeof(buf));

                int rotation = 0;

                QString value = QString(buf).toLower();
                //qDebug() << value << image;

                if (value == "right-top")
                    rotation = 90;
                else
                    rotation = 0;

                return rotation;
            }
        }
    }
    return 0;
}

void Mitakuuluu::windowActive()
{
    if (iface) {
        iface->call(QDBus::NoBlock, "windowActive");
    }
}

bool Mitakuuluu::checkLogfile()
{
    return QFile("/tmp/mitakuuluu2.log").exists();
}

bool Mitakuuluu::checkAutostart()
{
    QString autostartUser = QString(AUTOSTART_USER).arg(QDir::homePath());
    QFile service(autostartUser);
    return service.exists();
}

void Mitakuuluu::setAutostart(bool enabled)
{
    QString autostartUser = QString(AUTOSTART_USER).arg(QDir::homePath());
    if (enabled) {
        QString autostartDir = QString(AUTOSTART_DIR).arg(QDir::homePath());
        QDir dir(autostartDir);
        if (!dir.exists())
            dir.mkpath(autostartDir);
        QFile service(AUTOSTART_SERVICE);
        service.link(autostartUser);
    }
    else {
        QFile service(autostartUser);
        service.remove();
    }
}

void Mitakuuluu::sendLocation(const QStringList &jids, const QString &latitude, const QString &longitude, int zoom, bool googlemaps)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "sendLocation", jids, latitude, longitude, zoom, googlemaps);
    }
}

void Mitakuuluu::renewAccount(const QString &method, int years)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "renewAccount", method, years);
    }
}

QString Mitakuuluu::checkIfExists(const QString &path)
{
    if (QFile(path).exists())
        return path;
    return QString();
}

void Mitakuuluu::unsubscribe(const QString &jid)
{
    if (iface) {
        iface->call(QDBus::NoBlock, "unsubscribe", jid);
    }
}

QString Mitakuuluu::getAvatarForJid(const QString &jid)
{
    QString dirname = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatar";
    QString fname = QString("%1/%2").arg(dirname).arg(jid);
    return fname;
}

void Mitakuuluu::rejectMediaCapture(const QString &path)
{
    QFile file(path);
    if (file.exists())
        file.remove();
}

QStringList Mitakuuluu::getLocalesNames()
{
    return _localesNames;
}

int Mitakuuluu::getCurrentLocaleIndex()
{
    if (_locales.contains(_currentLocale)) {
        return _locales.indexOf(_currentLocale);
    }
    else
        return 0;
}

// Settings

void Mitakuuluu::save(const QString &key, const QVariant &value)
{
    if (settings) {
        settings->setValue(key, value);
        settings->sync();
    }
}

QVariant Mitakuuluu::load(const QString &key, const QVariant &defaultValue)
{
    if (settings) {
        settings->sync();
        QVariant value = settings->value(key, defaultValue);
        switch (defaultValue.type()) {
        case QVariant::Bool:
            return value.toBool();
        case QVariant::Double:
            return value.toDouble();
        case QVariant::Int:
            return value.toInt();
        case QVariant::String:
            return value.toString();
        case QVariant::StringList:
            return value.toStringList();
        case QVariant::List:
            return value.toList();
        default:
            return value;
        }
    }
    return QVariant();
}

QVariantList Mitakuuluu::loadGroup(const QString &name)
{
    if (settings) {
        settings->sync();
        settings->beginGroup(name);
        QVariantList result;
        foreach (const QString &key, settings->childKeys()) {
            QVariantMap item;
            item["jid"] = key;
            item["value"] = settings->value(key, 0);
            result.append(item);
        }
        settings->endGroup();
        return result;
    }
    return QVariantList();
}

void Mitakuuluu::clearGroup(const QString &name)
{
    if (settings) {
        settings->beginGroup(name);
        settings->remove("");
        settings->endGroup();
        settings->sync();
    }
}
