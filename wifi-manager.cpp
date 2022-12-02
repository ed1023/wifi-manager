#include <QDebug>
#include <QMetaEnum>

#include "wifi-manager.h"

#define NETWORK_CONNECTION_PATH "/etc/NetworkManager/system-connections/"

const QString WifiManager::kWirelessInterface = "wlan0";
const QDir WifiManager::kNetworkConnectionDir = QDir(NETWORK_CONNECTION_PATH);

WifiManager::WifiManager(QObject *parent): QObject(parent)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::availableChanged, this, &WifiManager::onNetworkManagerAvailableChanged, Qt::DirectConnection);
    connect(m_networkManager, &NetworkManager::stateChanged, this, &WifiManager::onNetworkManagerStateChanged, Qt::DirectConnection);
    connect(m_networkManager, &NetworkManager::networkingEnabledChanged, this, &WifiManager::onNetworkManagerNetworkingEnabledChanged, Qt::DirectConnection);
    connect(m_networkManager, &NetworkManager::wirelessEnabledChanged, this, &WifiManager::onNetworkManagerWirelessEnabledChanged, Qt::DirectConnection);
    connect(m_networkManager, &NetworkManager::wirelessDeviceAdded, this, &WifiManager::onNetworkManagerWirelessDeviceAdded, Qt::DirectConnection);
    connect(m_networkManager, &NetworkManager::wirelessDeviceRemoved, this, &WifiManager::onNetworkManagerWirelessDeviceRemoved, Qt::DirectConnection);
}

WifiManager::~WifiManager(){
    qDebug() << "Shutting down network-manager service";
    delete m_networkManager;
    m_networkManager = nullptr;
}

NetworkManager *WifiManager::networkManager() const
{
    return m_networkManager;
}

QList<WirelessAccessPoint *> WifiManager::wirelessAccessPoints() const
{
    return m_wirelessAccessPoints;
}

QVector<WifiManager::AccessPoint> WifiManager::getAccessPoints() const
{
    return setAccessPoints;
}

void WifiManager::checkSavedAccesspoint()
{
    if(kNetworkConnectionDir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries).count() == 0)
    {
        Q_EMIT noSavedAccesspointAvalaible();
    }
    else
    {
        Q_EMIT savedAccesspointAvalaible();
    }
}

QString WifiManager::getMacAddress() const
{
    return m_wlan_dev->macAddress();
}

/* Connect to a a new acesspoint and removes previous active connection if needed*/
WifiManager::ConnectionStatus WifiManager::connectBySsid(QString ssid, QString passphrase, bool hidden)
{
    bool exist = false;

    qDebug() << "Connect to: " << ssid << " pswd: " << passphrase << " hidden: " << hidden;

    removeAllConnectionFiles();

    Q_FOREACH(auto ap, setAccessPoints)
    {
        if(ap.apName == ssid)
        {
            exist = true;
            break;
        }
    }
    if(!exist && !hidden)
    {
        qDebug() << "Wrong Security Protocol";
        return ConnectionStatus::WrongProtocol;
    }


    NetworkManager::NetworkManagerError result = m_networkManager->connectWifi(m_wirelessNetworkDevices.first()->interface(),
                                                                               ssid, 
                                                                               passphrase, 
                                                                               hidden);
    if(result == NetworkManager::NetworkManagerErrorNoError)
    {
        for(int i=0; i < setAccessPoints.size(); i++)
        {
            if(setAccessPoints[i].apName == ssid)
            {
                setAccessPoints[i].apState = Connected;
            }
            else
            {
                setAccessPoints[i].apState = Disconnected;
            }
        }
        qDebug() << "connectbyssid";
        Q_EMIT scanCompleted(setAccessPoints);
        return ConnectionStatus::Successful;
    }
    else 
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<NetworkManager::NetworkManagerError>();
        qDebug() << "NetworkError code: " << metaEnum.valueToKey(result);
        removeAllConnectionFiles();
        return ConnectionStatus::Failed;
    }

}

/* Deletes the current active connection*/
void WifiManager::deleteConnection()
{
    if(m_networkManager->networkSettings()->connections().isEmpty()){
        qDebug() << "No connection to delete";
        return;
    }

    while(!m_networkManager->networkSettings()->connections().isEmpty()){
        m_networkManager->networkSettings()->connections().first()->deleteConnection();
    }
    qDebug() << "Connection deleted";
}

/* Disconnect the network device wlan0*/
void WifiManager::disconnectDevice()
{
    m_networkManager->wirelessNetworkDevices().first()->disconnectDevice();
}

/* Reconnect using known configurations*/
void WifiManager::reloadConnections()
{
    qDebug() << "Checking available connections and reloading";
    //DevNote: The dbus networkManagerInterface has been removed from 
    //         nymea-networkmanager lib

//    for(int i=0; i < m_networkManager->networkSettings()->connections().size(); i++ ){
//        QDBusMessage query = m_networkManager->networkManagerInterface()->
//                call("ActivateConnection",
//                     QVariant::fromValue(m_networkManager->networkSettings()->connections()[i]->objectPath()),
//                     QVariant::fromValue(m_networkManager->wirelessNetworkDevices().first()->objectPath()),
//                     QVariant::fromValue(QDBusObjectPath("/")));
//        if(query.type() != QDBusMessage::ReplyMessage) {
//            //            qCWarning(dcNetworkManager()) << query.errorName() << query.errorMessage();
//            qDebug() << "Fail to activate";
//            continue;
//        }
//        break;
//    }
}

/* Deactivate or disconnect from the current active connection*/
void WifiManager::removeActiveConnection()
{
      //DevNote: The dbus networkManagerInterface has been removed from 
      //         nymea-networkmanager lib
//    QString name = m_wlan_dev->activeAccessPoint()->ssid();
//    QDBusMessage query = m_networkManager->networkManagerInterface()->
//            call("DeactivateConnection",
//                 QVariant::fromValue(m_wlan_dev->activeAccessPoint()->objectPath()));

//    if(query.type() != QDBusMessage::ReplyMessage) {
//        //qCWarning(dcNetworkManager()) << query.errorName() << query.errorMessage();
//        qDebug() << "Fail to Remove active connection";
//        qDebug() << query.errorName() << query.errorMessage();
//        return;
//    }
//    for(int i = 0; i< setAccessPoints.size(); i++){
//        if(setAccessPoints[i].apName == name){
//            setAccessPoints[i].apState = Disconnected;
//            continue;
//        }
//    }
    Q_EMIT activeConnectionRemoved();
}

/* Initializes Networkmanager through dbus*/
void WifiManager::run()
{
    m_networkManager->start();
    m_wirelessNetworkDevices = m_networkManager->wirelessNetworkDevices();

    if(m_wirelessNetworkDevices.isEmpty())
    {
        qWarning() << "Wifi unavailable";
        return;
    }

    qDebug() << "Available network devices are: " << m_networkManager->networkDevices();
    Q_FOREACH(auto netDev, m_networkManager->wirelessNetworkDevices())
    {
        if(netDev->interface()== kWirelessInterface)
        {
            m_wlan_dev = netDev;
            Q_EMIT wlanDeviceFound();
        }
    }

    connect(m_wlan_dev, &NetworkDevice::stateChanged, this, &WifiManager::onWlanStateChanged, Qt::DirectConnection);


    if(m_networkManager->networkSettings()->connections().isEmpty())
    {
        Q_EMIT noSavedAccesspointAvalaible();
    }
    else
    {
        if(m_networkManager->networkSettings()->connections().isEmpty())
        {
            m_current_AP = m_wirelessNetworkDevices[0]->activeAccessPoint();
            connect(m_current_AP, 
                    &WirelessAccessPoint::signalStrengthChanged,
                    this, &WifiManager::currentWifiStrengthChanged);
        }
        Q_EMIT savedAccesspointAvalaible();
    }

    if(m_networkManager->state() == NetworkManager::NetworkManagerStateDisconnected
       && !m_networkManager->networkSettings()->connections().isEmpty())
    {
        reloadConnections();
    }
}

/* Get active connection ssid*/
const QString WifiManager::currentSSID()
{
    QString result = "";
    for(int i=0; i < setAccessPoints.size(); i++)
    {
        if(setAccessPoints[i].apState == Connected)
        {
            qDebug() << "\033[1;35m currentSSID lib active: \033[0m" << m_wirelessNetworkDevices[0]->activeAccessPoint()->ssid();
            qDebug() << "currentSSID: " << setAccessPoints[i].apName << " State: " << setAccessPoints[i].apState << " frequency: " <<  setAccessPoints[i].apFreq ;
            result = setAccessPoints[i].apName;
        }
    }

    return result;
}

/* Perform a scan of available networks*/
void WifiManager::scan()
{

    QString activeName;

    if(m_wirelessNetworkDevices.isEmpty())
    {
        qDebug() << "wirelessNetworkDeviceslis empty in scan";
        return;
    }
    //    qDebug() << "Total number of wireless Network devices is: " << m_wirelessNetworkDevices.size();
    m_wirelessNetworkDevices.first()->scanWirelessNetworks();

    Q_FOREACH(auto d, m_wirelessNetworkDevices) 
    {
        if(d->deviceStateString() == "NetworkDeviceStateDisconnected") 
        {
            qDebug() << "Device state is: " << d->deviceStateString();
            qDebug() << "Available connections: " << d->accessPoints();
        }
    }
    m_wirelessAccessPoints.clear();
    m_wirelessAccessPoints = m_networkManager->wirelessNetworkDevices().first()->accessPoints();

    if(m_networkManager->state() != NetworkManager::NetworkManagerStateDisconnected)
    {
        activeName = m_networkManager->wirelessNetworkDevices().first()->activeAccessPoint()->ssid();
    }

    if(m_wirelessAccessPoints.isEmpty()){
        qDebug() << "Wireless AP list empty";
        return;
    }

    setAccessPoints.clear();
    for(int i = 0; i < m_wirelessAccessPoints.size(); i++)
    {
        //DevNote: 1. Only add access points to list that are secured networks
        //         2. Only add networks with an ssid
        if(m_wirelessAccessPoints[i]->isProtected() && m_wirelessAccessPoints[i]->ssid() != "")
        {
            setAccessPoints.append({m_wirelessAccessPoints[i]->ssid(),
                                    m_wirelessAccessPoints[i]->frequency(),
                                    m_wirelessAccessPoints[i]->ssid() == activeName ? Connected: Disconnected,
                                    m_wirelessAccessPoints[i]->isProtected(),
                                    m_wirelessAccessPoints[i]->signalStrength()});
            //qDebug() << "\033[1;36mWifi list is:\033[0m " << m_wirelessAccessPoints[i]->ssid();
        }
        else
        {
            qDebug() << "Unsecured: " << m_wirelessAccessPoints[i];
        }
    }
    std::sort(setAccessPoints.begin(), 
              setAccessPoints.end(), 
              [](const AccessPoint& lhs, const AccessPoint& rhs) 
              {return lhs.apSignalStrength > rhs.apSignalStrength; });

    qDebug() << "Scan from scan";
    Q_EMIT scanCompleted(setAccessPoints);
}

void WifiManager::getApList()
{
    if(m_networkManager->wirelessNetworkDevices().isEmpty())
    {
        qDebug() << "wirelessNetworkDeviceslis empty in get list";
        return;
    }

    Q_EMIT scanCompleted(setAccessPoints);
}



/* Get the current State of the Network*/
QString WifiManager::state()
{
    qDebug() << "***********" << FILE_NAME_AND_LINE_CSTR << " State String: " << m_networkManager->stateString();
    qDebug() << "***********" << FILE_NAME_AND_LINE_CSTR << " connectivity State String: " << m_networkManager->connectivityState();
    return m_networkManager->stateString();
}

void WifiManager::onNetworkManagerStateChanged(const NetworkManager::NetworkManagerState &state)
{
    qDebug() << "Network State changed in Wifi manager:" << state;
    Q_EMIT networkStateChanged(state);
    switch (state) 
    {
        case NetworkManager::NetworkManagerStateConnectedGlobal:
          Q_EMIT connectedGlobal();
          Q_EMIT currentWifiStrengthChanged();
          break;
        case NetworkManager::NetworkManagerStateConnectedSite:
          // We are somehow in the network
          break;
        case NetworkManager::NetworkManagerStateUnknown:
          break;
        case NetworkManager::NetworkManagerStateAsleep:
          break;
        case NetworkManager::NetworkManagerStateDisconnected:
          Q_EMIT noSavedAccesspointAvalaible();
          break;
        case NetworkManager::NetworkManagerStateConnectedLocal:
          // Everything else is not connected, start the service
          qDebug() << __FUNCTION__ << "Not connected to anything";
          break;
        default:
          break;
    }
}


void WifiManager::onNetworkManagerWirelessEnabledChanged(bool enabled)
{
    qDebug() << "Networkmanager wireless networking is now" << (enabled ? "enabled" : "disabled");
}

void WifiManager::onWlanStateChanged(const NetworkDevice::NetworkDeviceState &state)
{
    qDebug() << "Network Device State changed in Wifi manager:" << state;

    switch (state) 
    {
        case NetworkDevice::NetworkDeviceStateDisconnected:
          break;
        case NetworkDevice::NetworkDeviceStateActivated:
          qDebug() << "Password authentication succeeded in WifiManager";
          Q_EMIT passwordAuthSucceeded();
          break;
        case NetworkDevice::NetworkDeviceStateFailed:
          qDebug() << "Password authentication failed in WifiManager";
          Q_EMIT passwordAuthFailed();
          break;
        default:
          break;
    }
}

int WifiManager::getCurrentWifiStrength()
{
    if(m_wirelessNetworkDevices[0]->activeAccessPoint())
    {
        return m_wirelessNetworkDevices[0]->activeAccessPoint()->signalStrength();
    }
    else 
    {
        return 0;
    }
}

void WifiManager::removeAllConnectionFiles()
{
    QDir dir(kNetworkConnectionDir);
    dir.setNameFilters(QStringList() << "*.*");
    dir.setFilter(QDir::Files);
    Q_FOREACH(QString dirFile, dir.entryList())
    {
        dir.remove(dirFile);
    }
}

int WifiManager::getSignalStrength(QString SSID)
{
    if ( SSID.isNull() || SSID.isEmpty() )
         return 0;

    for(int i = 0; i < setAccessPoints.size(); i++)
    {
        if(SSID == setAccessPoints[i].apName)
        {
            return setAccessPoints[i].apSignalStrength;
        }
    }
    return 0;
}
