
#include <QDebug>
#include <QDBusConnection>
#include <QListIterator>
#include <QMetaProperty>
#include <service.h>
#include <manager.h>

#include "NetworkCtl.h"

NetworkCtl* NetworkCtl::_instance = 0;

NetworkCtl::NetworkCtl(QObject *parent): QObject(parent)
{
    m_manager = new Manager(this);
    connect(this->getTechnology("wifi"),SIGNAL(scanCompleted()),this, SIGNAL(wifiListUpdated()));
    connect(this->getTechnology("wifi"),SIGNAL(poweredChanged(bool)),this,SLOT(wifiEnabled(bool)));
    connect(this->getTechnology("ethernet"),SIGNAL(poweredChanged(bool)),this,SLOT(ethernetEnabled(bool)));
    connect(this->getTechnology("wifi"),SIGNAL(connectedChanged()),this,SLOT(wifiConnected()));
    //connect(this->getTechnology("ethernet"),SIGNAL(connectedChanged()),this,SLOT(ethernetConnected()));

    m_agent=new Agent("/com/EMS/Connman", m_manager);


}

QString NetworkCtl::getStateString(Service::ServiceState state)
{
    switch (state) {
    case Service::ServiceState::UndefinedState:
        return  "undefined";
        break;
    case Service::ServiceState::IdleState:
        return  "idle";
        break;
    case Service::ServiceState::FailureState :
        return  "failure";
        break;
    case Service::ServiceState::AssociationState :
        return  "association";
        break;
    case Service::ServiceState::ConfigurationState :
        return  "configuration";
        break;
    case Service::ServiceState::ReadyState :
        return  "ready";
        break;
    case Service::ServiceState::DisconnectState :
        return  "disconnect";
        break;
    case Service::ServiceState::OnlineState :
        return  "online";
        break;

    default:
        return "unknown state";
    }
}

void NetworkCtl::listServices()
{
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        foreach (Service *service, m_manager->services())
        {
            qDebug() << "Service Path: "<< "[ " << service->objectPath().path().toLatin1().constData() << " ]" << endl;
            qDebug() << "Name : "<< service->name();
            qDebug() << "Type : "<< service->type();
            qDebug() << "State : "<< getStateString(service->state());
            if(service->type()!="ethernet")
            {
                qDebug() << "Strength : "<< service->strength();
            }

            if(service->state()==Service::ServiceState::OnlineState || service->state()==Service::ServiceState::ReadyState )
            {
                qDebug() << "Interface : "<< service->ethernet()->interface();
                qDebug() << "IP config : "<< service->ipv4()->method();
                if(service->type()!="ethernet")
                {
                    qDebug() << "Security : "<< service->security().join(", ");
                }
                qDebug() << "Address : "<< service->ipv4()->address();
                qDebug() << "Netmask : "<< service->ipv4()->netmask();
                if(service->state()==Service::ServiceState::OnlineState)
                {
                    qDebug() << "Gateway : "<< service->ipv4()->gateway();

                }
                qDebug() << "MAC adress : "<< service->ethernet()->address();
            }
            qDebug() << endl;
        }
    }
}



void NetworkCtl::scanWifi()
{
    if(isWifiPresent())
    {
        this->getTechnology("wifi")->scan();
        qDebug()<< "scanning wifi" <<endl;
    }

}
void NetworkCtl::wifiEnabled(bool enable)
{
    qDebug()<< "Wifi enabled :" << enable << endl;
}

void NetworkCtl::ethernetEnabled(bool enable)
{
    qDebug()<< "Ethernet enabled :" << enable << endl;
}

void NetworkCtl::ethernetConnected()
{
    qDebug()<< "Ethernet connected :" <<isEthernetConnected()<< endl;
}

void NetworkCtl::wifiConnected()
{
    qDebug()<< "Wifi connected :" <<isWifiConnected();
}

QList<EMSSsid> NetworkCtl::getWifiList()
{
    QList<EMSSsid> ssidList;

    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isWifiPresent())
        {
            qDebug()<< "Acquiring wifi list" ;
            foreach (Service *service, m_manager->services())
            {
                if(service->type() == "wifi")
                {
                    EMSSsid ssidWifi(service->objectPath().path(),
                                  service->name(),
                                  service->type(),
                                  getStateString(service->state()),
                                  service->strength());
                    ssidList.append(ssidWifi);
                }
            }
        }
    }
    return ssidList;
}

Service* NetworkCtl::getWifiByName(QString wifiName)
{
    Service* serviceRequested = NULL;
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isWifiPresent())
        {
            qDebug() << "Acquiring wifi :"<< wifiName;
            QListIterator<Service*> iter(m_manager->services());
            Service* service;
            bool found = false;
            while(iter.hasNext() && !found)
            {
                service = iter.next();
                if(service->type() == "wifi" && service->name() == wifiName)
                {
                   serviceRequested = service;
                   found = true;
                }
            }
        }
        else
        {
            qDebug() << " No wifi detected ";
        }
    }
    return serviceRequested;
}

bool NetworkCtl::isWifiPresent()
{
    bool result = false;
    if(getTechnology("wifi"))
    {
        result = true;
    }
    return result;
}


bool NetworkCtl::isEthernetPresent()
{
    bool result = false;
    if(getTechnology("ethernet"))
    {
        result = true;
    }
    return result;
}

bool NetworkCtl::isWifiConnected()
{
    Technology* technology=getTechnology("wifi");
    if(technology)
    {
        return technology->isConnected();
    }
    return false;
}

bool NetworkCtl::isEthernetConnected()
{
    Technology* technology=getTechnology("ethernet");
    if(technology)
    {
        return technology->isConnected();
    }
    return false;
}

bool NetworkCtl::isWifiEnabled()
{
    Technology* technology=getTechnology("wifi");
    if(technology)
    {
        return technology->isPowered();
    }
    return false;
}

bool NetworkCtl::isEthernetEnabled()
{
    Technology* technology=getTechnology("ethernet");
    if(technology)
    {
        return technology->isPowered();
    }
    return false;
}

void NetworkCtl::listTechnologies()
{
    QList<Technology*> listTechno = m_manager->technologies();
    QListIterator<Technology*> iter( listTechno );
    while( iter.hasNext() )
    {
        Technology* tempTech = iter.next();
        qDebug() << "Name: "<< tempTech->name();
        qDebug() << "Type: "<<tempTech->type();
        qDebug() << "Connected: "<<tempTech->isConnected();
        qDebug() << "Powered: "<<tempTech->isPowered();
        qDebug() << "Tethering: "<<tempTech->tetheringAllowed()<<endl;
    }

}

Technology* NetworkCtl::getTechnology(QString technologyType)
{
    Technology* result = NULL;

    foreach(Technology* technology,m_manager->technologies())
    {
        if(technology->type() == technologyType)
        {
            result = technology;
        }
    }
    return result;
}

void NetworkCtl::enableWifi(bool enable)
{
    Technology* technology = getTechnology("wifi");
    technology->setPowered(enable);
    if(enable)
        qDebug() << "Enable " << technology->name();
    else
        qDebug() << "Disable " << technology->name();
}

void NetworkCtl::enableEthernet(bool enable)
{
    Technology* technology = getTechnology("ethernet");
    technology->setPowered(enable);
    if(enable)
        qDebug() << "Enable " << technology->name();
    else
        qDebug() << "Disable " << technology->name();
}

/*
QList<Service*> Connman::getEthService(Connman* m_manager)
{
    QList<Service*> ethServices;
    if(!(m_manager->services().isEmpty()))
    {
        foreach (Service *service, m_manager->services())
        {
            if(service->type()==QString("ethernet"))
            {
                ethServices.append(service);
            }
        }
    }
    return ethServices;
}
*/

EMSSsid::EMSSsid(QString path, QString name, QString type, QString state, int strength) :
    m_path(path),
    m_name(name),
    m_type(type),
    m_state(state),
    m_strength(strength)
{

}
EMSSsid::EMSSsid()
{

}
// Get methods of EMSSsid class
QString EMSSsid::getName() const
{
    return m_name;
}

QString EMSSsid::getType() const
{
    return m_type;
}

QString EMSSsid::getState() const
{
    return m_state;
}

int EMSSsid::getStrength() const
{
    return m_strength;
}
QString EMSSsid::getPath() const
{
    return m_path;
}

void EMSSsid::setName(QString name)
 {
     m_name = name;
 }

void EMSSsid::setType(QString type)
{
    m_type = type;
}

void EMSSsid::setState(QString state)
{
    m_state = state;
}

void EMSSsid::setStrength(int strength)
{
    m_strength = strength;
}

void EMSSsid::setPath(QString path)
{
    m_path = path;
}

ConnexionRequest::ConnexionRequest(QString path, QString name, QString passphrase, QString state, int timeout) :
    m_path(path),
    m_name(name),
    m_passphrase(passphrase),
    m_state(state),
    m_timeout(timeout)
{

}

ConnexionRequest::ConnexionRequest()
{

}


QString ConnexionRequest::getName() const
{
    return m_name;
}
QString ConnexionRequest::getPath() const
{
    return m_path;
}
int ConnexionRequest::getTimeout() const
{
    return m_timeout;
}
QString ConnexionRequest::getPassphrase() const
{
    return m_passphrase;
}

void ConnexionRequest::setName(QString name)
{
    m_name=name;
}

void ConnexionRequest::setPath(QString path)
{
    m_path=path;
}

void ConnexionRequest::setTimeout(int timeout)
{
    m_timeout=timeout;
}

void ConnexionRequest::setPassphrase(QString passphrase)
{
    m_passphrase=passphrase;
}

ConnexionRequest::~ConnexionRequest()
{

}


NetworkCtl::~NetworkCtl()
{
}


EMSSsid::~EMSSsid()
{
}