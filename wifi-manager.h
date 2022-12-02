#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>
#include <QDir>

#include "lib_global.h"
#include <nymea-networkmanager/networkmanager.h>

class LIBSHARED_EXPORT WifiManager : public QObject {


    Q_OBJECT
    Q_PROPERTY(const QString currentSSID READ currentSSID NOTIFY currentSSIDChanged)
    Q_PROPERTY(int currentWifiStrength READ getCurrentWifiStrength NOTIFY currentWifiStrengthChanged)
    Q_PROPERTY(bool isConnected READ isConnectedToWifi NOTIFY networkStateChanged)

public:
    enum AccessPointState{
        Connected,
        Disconnected,
    };
    Q_ENUM(AccessPointState)

    enum ConnectionStatus{
        Successful,
        Failed,
        WrongProtocol,
    };
    Q_ENUM(ConnectionStatus)

    struct AccessPoint
    {
        QString apName;
        double apFreq;
        AccessPointState apState;
        bool apSecurity;
        int apSignalStrength;

        bool operator==(const AccessPoint &a) {
            return this->apName == a.apName;
        }

        bool operator>(const AccessPoint &a) const {
            return this->apSignalStrength > a.apSignalStrength;
        }
    };
    explicit WifiManager(QObject *_parent = nullptr);
    ~WifiManager();

    const static QString kWirelessInterface;
    const static QDir    kNetworkConnectionDir;

    NetworkManager* networkManager() const;

    QList<WirelessAccessPoint*> wirelessAccessPoints() const;
    QVector<AccessPoint> getAccessPoints() const;

    void checkSavedAccesspoint();
    QString getMacAddress() const;
    bool isConnectedToWifi(){;
        return m_networkManager->state() == NetworkManager::NetworkManagerStateConnectedGlobal;
    }

private:
    NetworkManager *m_networkManager = nullptr;
    QList<WirelessNetworkDevice *> m_wirelessNetworkDevices;
    QList<WirelessAccessPoint *> m_wirelessAccessPoints;
    QVector<AccessPoint> setAccessPoints;
    WirelessNetworkDevice *m_wlan_dev = nullptr;    
    WirelessAccessPoint *m_current_AP = nullptr;

public Q_SLOTS:
    const QString currentSSID();
    void scan();
    int getSignalStrength(QString SSID);
    void getApList();
    QString state();
    void run();
    ConnectionStatus connectBySsid(QString ssid, QString passphrase, bool hidden);
    void deleteConnection();
    void disconnectDevice();
    void reloadConnections();
    void removeActiveConnection();
    void onNetworkManagerStateChanged(const NetworkManager::NetworkManagerState &state);
    void onNetworkManagerWirelessEnabledChanged(bool enabled);
    void onWlanStateChanged(const NetworkDevice::NetworkDeviceState &state);
    int getCurrentWifiStrength();
    void removeAllConnectionFiles();

Q_SIGNALS:
    void currentSSIDChanged(const QString &currentSSID);
    void scanCompleted(QVector<AccessPoint> &accessPoints);
    void activeConnectionRemoved();
    void networkStateChanged(const NetworkManager::NetworkManagerState &state);
    void onNetworkManagerAvailableChanged(const bool &available);
    void onNetworkManagerNetworkingEnabledChanged(bool enabled);
    void onNetworkManagerWirelessDeviceAdded(WirelessNetworkDevice *wirelessDevice);
    void onNetworkManagerWirelessDeviceRemoved(const QString &interface);
    void onNetworkManagerSignalStrengthChanged();
    void connectedGlobal();
    void savedAccesspointAvalaible();
    void noSavedAccesspointAvalaible();
    void passwordAuthFailed();
    void passwordAuthSucceeded();
    void currentWifiStrengthChanged();
    void wlanDeviceFound();

};

#endif // WIFIMANAGER_H
