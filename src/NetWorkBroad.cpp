#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <errno.h>
#include <map>
#include <fstream>
#include <vector>
#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <hv/UdpServer.h>
//#include "yaml-cpp/yaml.h"

//#define NET_MIC_CONFIG_PATH "/etc/netplan/50-cloud-init.yaml"
using namespace hv;

__attribute__((unused))  static bool  OnGetLocateIpAdd(int hSocket,const char *ifname, char *ip)
{
    struct ifreq ifr = {0};
    bool fRet = false;
    
    memset(&ifr,0, sizeof(ifr));
    memcpy(ifr.ifr_name, ifname, strlen(ifname));
    
    if(ioctl(hSocket, SIOCGIFADDR, &ifr) <0)
    {
        printf("ioctl error!\n");
        return fRet;
    }
    //sock_ntop_host(&ifr.ifr_addr, sizeof(struct sockaddr), ip, 32);
    struct sockaddr_in *sin = (struct sockaddr_in*)&ifr.ifr_addr;
    if(NULL != ip) 
    {
        strcpy(ip, inet_ntoa(sin->sin_addr));
        fRet = true;
    }
    return fRet;
}


static bool  OnGetAllNetInfo(int hSocket, char *ipAddr)
{
    if(hSocket < 1)
        return false;
    

    bool fRet = false;
    struct ifreq ifr[32] = {0};
    struct ifconf ifc = {0};

    ifc.ifc_len = sizeof ifr;
    ifc.ifc_buf = (caddr_t)ifr;
    if(ioctl(hSocket, SIOCGIFCONF, (char*)&ifc))
    {
        printf("ioctl error!\n");
        return fRet;
    }

    rapidjson::StringBuffer strBuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
    writer.StartObject();
    writer.Key("NetWork");
    writer.StartArray();
    int iIntrface = ifc.ifc_len / sizeof(struct ifreq);
    while ((iIntrface--) > 0)
    {
        writer.StartObject();
        if(!ioctl(hSocket, SIOCGIFADDR, &ifr[iIntrface]))
        {
            writer.Key("NICName");
            writer.String(ifr[iIntrface].ifr_name);
            writer.Key("IPAddr");
            writer.String(inet_ntoa(((struct sockaddr_in*)(&ifr[iIntrface].ifr_addr))->sin_addr));

        }
        //     sprintf(szCurIpInfo,"%s IP:%s",ifr[iIntrface].ifr_name, inet_ntoa(((struct sockaddr_in*)(&ifr[iIntrface].ifr_addr))->sin_addr));
        // else
        //     printf("ioctl  SIOCGIFADDR error!\n");
        //strcat(ipAddr,szCurIpInfo);
		
		char szMacAddrInfo[64] = {0};
		if(!ioctl(hSocket, SIOCGIFHWADDR, (char *)&ifr[iIntrface]))
		{
		 sprintf(szMacAddrInfo,"%02x-%02x-%02x-%02x-%02x-%02x",
			(unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[0],
            (unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[1],
            (unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[2],
            (unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[3],
            (unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[4],
			(unsigned char)ifr[iIntrface].ifr_hwaddr.sa_data[5]);
		}
		else
            printf("ioctl  SIOCGIFADDR error!\n");
		writer.Key("MACAddr");
        writer.String(szMacAddrInfo);
		//strcat(ipAddr,szMacAddrInfo);
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    std::string result = strBuf.GetString();
    strcpy(ipAddr,result.c_str());
    //printf("1111 = %s\r\n",ipAddr);
    return fRet;
}

static bool OnJsonObject2StdMap(rapidjson::Value &jValue,std::map<std::string,std::string> &mapData)
{
    if(jValue.IsNull())
        return false;
    for (rapidjson::Value::MemberIterator itItem = jValue.MemberBegin(); itItem != jValue.MemberEnd(); ++itItem) 
    {
        std::string keyItem = itItem->name.GetString();
        
        if(itItem->value.IsString())
           mapData[keyItem] = itItem->value.GetString();
        else if(itItem->value.IsInt())
            mapData[keyItem] = std::to_string(itItem->value.GetInt());
        else if(itItem->value.IsInt())
            mapData[keyItem] = std::to_string(itItem->value.GetDouble());
    }
    return true;
}

static bool OnParseJsonData(std::string strJson,std::string strLabel,std::map<std::string,std::string> &mapData)
{
    bool fRet = false;
    if(strJson.empty())
        return fRet;
    rapidjson::Document document;
    if (document.Parse(strJson.c_str()).HasParseError())
    {
        std::cout << "json parse error" <<std::endl;
        return false;
    }
        
    if (!document.IsObject()) 
    {
        std::cout << "error json is not object " <<std::endl;
        return false;
    }
    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) 
    {
        std::string key = itr->name.GetString();
        rapidjson::Value jValue;
        jValue.CopyFrom(itr->value, document.GetAllocator());
        if(jValue.IsObject())
        {
            if(!strcasecmp(strLabel.c_str(),key.c_str()))
            {
                if(OnJsonObject2StdMap(jValue, mapData))
                    break;
            }
        }
    }
    return true;
}

#if 0
static YAML::Node OnOpenYml(const std::string strFilePath) 
{
    YAML::Node configContext; 
    if(strFilePath.empty())
        return configContext;

    try {
        std::ifstream ifs(strFilePath);
        configContext = YAML::Load(ifs);
        ifs.close();
    }
    catch (YAML::BadFile& e) {
        std::cout << "read error!" <<e.what()<< std::endl;
        return configContext;
    } 
    std::cout << "read finsh!" << std::endl;
    return configContext;    
}

static bool OnSaveYml(YAML::Node nodeContext, std::string strYmlFile)
{
    if(strYmlFile.empty() || nodeContext.IsNull())
        return false;
    YAML::Emitter strYmlOut;
    strYmlOut.SetNullFormat(YAML::LowerNull);
    strYmlOut << nodeContext;

    if(YAML::Load(strYmlOut.c_str()).IsNull())
        return  false;
        
    std::ofstream fout(strYmlFile, std::ios::trunc); //保存config为yaml文件
    fout << strYmlOut.c_str();
    std::cout << strYmlOut.c_str() <<std::endl;
    fout.close();
    // uid_t uid = getuid();
    // if (setuid(0)) 
    // {
    //     std::cout << "111111" <<std::endl;
    //     return -1;
    // }
        

    if(!std::system("sudo -S</etc/.network_cnf netplan apply")) // systemctl restart NetworkManager
        return true;

    // setuid(uid);
    return true;    
}

__attribute__((unused))   static bool OnUpdteYmlConf(std::string strYmlFile,std::map<std::string,std::string> mapData)
{
    if(strYmlFile.empty() || mapData.empty())
        return false;
    const char *szIpAddName = "addresses";
    const char *szDhcp4 = "dhcp4";
    const char *szGateWay = "gateway4";
    YAML::Node configContext = OnOpenYml(strYmlFile);
    if(configContext.IsNull())
        return false;  
    std::string strNetCardName = mapData["ethName"];
    YAML::Node netNetCardInfo = configContext["network"]["ethernets"][strNetCardName];

    if(!strcasecmp(mapData[szDhcp4].c_str(),"true"))
    {
        netNetCardInfo[szDhcp4] = mapData[szDhcp4];
        netNetCardInfo.remove(szIpAddName);
        netNetCardInfo.remove(szGateWay);
        netNetCardInfo.remove("nameservers");
    }
    else
    {
        netNetCardInfo[szDhcp4]=mapData[szDhcp4];
        netNetCardInfo[szGateWay]= mapData[szGateWay];
        char szIpAddrData[64] = {0};
        sprintf(szIpAddrData, "%s/23",mapData[szIpAddName].c_str());
        netNetCardInfo[szIpAddName][0] = szIpAddrData;
        netNetCardInfo["nameservers"]["addresses"][0] = mapData[szGateWay];
        netNetCardInfo["nameservers"]["addresses"][1] = "8.8.8.8";
    }
    return OnSaveYml(configContext, strYmlFile);

    // for (const auto &itemNetCard : netNetCardInfo)
    // {
    //     auto itemKey = itemNetCard.first;    
    //     auto itemValue = itemNetCard.second;
    //     if(itemValue.Type() == YAML::NodeType::Scalar)
    //         std::cout << itemKey.as<std::string>()<< " : " << itemValue.as<std::string>()  <<std::endl;
    //     else if(itemValue.Type() == YAML::NodeType::Sequence)
    //     {
    //         size_t szCnt = itemValue.size();
    //         std::cout <<  itemKey.as<std::string>() << " : " << itemValue[0].as<std::string>() << " " << szCnt<<std::endl;
    //     }
     
    //     //std::cout << itemKey.as<std::string>() <<" : " << itemValue.as<std::string>() <<std::endl;
    // }
   // return true;
}
#endif

__attribute__((unused))  static  int setip(const char *netName,const char *ip)
{
    struct ifreq temp;
    struct sockaddr_in *addr;
    int fd = 0;
    int ret = -1;
    strcpy(temp.ifr_name, netName );
    if ((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        return -1;
    }
    std::cout << netName << " " << ip <<std::endl;
    addr = (struct sockaddr_in *)&(temp.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    ret = ioctl(fd, SIOCSIFADDR, &temp);
    close(fd);
    if (ret < 0)
        return -1;
    return 0;
}

static void OnUdpServerInfo(UdpServer &srv, int iPort)
{
  
    int bindfd = srv.createsocket(iPort);
    if (bindfd < 0) 
        return ;

    printf("server bind on port %d, bindfd=%d ...\n", iPort, bindfd);
    srv.onMessage = [](const SocketChannelPtr& channel, Buffer* buf) 
    {
        // echo
        printf("< %.*s\n", (int)buf->size(), (char*)buf->data());
        std::map<std::string,std::string> mapData;
        if(OnParseJsonData((char*)buf->data(),"updateNetwork",mapData))
        {
            //setip(mapData["ethName"].c_str(), mapData["addresses"].c_str());
            char szCmd[256] = {0};
            sprintf(szCmd,"sudo -S</etc/.network_cnf net_modify_eth.sh %s %s %s %s %s",
                    mapData["ethName"].c_str(), mapData["dhcp4"].c_str(), mapData["addresses"].c_str(), mapData["gateway4"].c_str(), mapData["gateway4"].c_str());
            if(!std::system(szCmd)) // systemctl restart NetworkManager
                channel->write("success");
            else
                channel->write("fail");
        }
        else
            channel->write("fail");
    };

    srv.onWriteComplete = [](const SocketChannelPtr& channel, Buffer* buf) 
    {
        printf("> %.*s\n", (int)buf->size(), (char*)buf->data());
    };

    srv.start();
}

int main()
{
    int hSocket;
    int iSendbytes;
    int iOptval = 1;
    struct sockaddr_in Addr;
    UdpServer srv;
    if ((hSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("socket fail\n");
        return -1;
    }

    if (setsockopt(hSocket, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int)) < 0)
    {
        printf("setsockopt failed!");
    }
    memset(&Addr, 0, sizeof(struct sockaddr_in));
    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    Addr.sin_port = htons(8899);


    OnUdpServerInfo(srv,10022);
    // char ipWlanAddr[32] = { 0 };
    // const char *wlanName = "wlan0";
    // OnGetLocateIpAdd(hSocket, wlanName, ipWlanAddr);

    // char ipEth[32] = { 0 };
    // const char *ethName = "eth0";
    // OnGetLocateIpAdd(hSocket, ethName, ipEth);   
    int iIndex = 15;
    while (iIndex > 0)
    {
        //iIndex --;
        char ipNetInfo[1024] = {0};
        OnGetAllNetInfo(hSocket,ipNetInfo);
        if ((iSendbytes = sendto(hSocket, ipNetInfo, strlen(ipNetInfo), 0, (struct sockaddr*)&Addr, sizeof(struct sockaddr))) == -1)
        {
            printf("sendto data fail,please check errno=%d\n", errno);
            iIndex = 15;
        }
        //printf("sendto %s\n", ipNetInfo);
        sleep(10);     
    }
    close(hSocket);
    hSocket = 0;

    return 0;
}