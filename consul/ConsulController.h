#ifndef CONSULCONTROLLER_H
#define CONSULCONTROLLER_H

#include <string>
#include <vector>
#include <map>

#include <QJsonObject>

namespace funnyconsul {

typedef struct CCRequest {
    std::string url;
    std::string body;
}CCRequest_tag;

typedef struct CCResponse {
    std::string header;
    std::string body;
}CCResponse_tag;


/***************************************************************************************/

typedef struct CCUpStream {
    bool IsEmpty() const {
        return true;
    }
}CCUpStream_tag;

typedef struct CCProxyConfig {
    bool IsEmpty() const {
        return true;
    }
}CCProxyConfig_tag;

// CCService 中 Kind="connect-proxy"    port必填
typedef struct CCProxy {
    std::string destination_service_name;                  // (必填)
    std::string destination_service_id;
    std::string local_service_address;
    int local_service_port;                                    // （可选）
    CCProxyConfig config;
    std::vector<CCUpStream> upstreams;

    bool IsEmpty() const {
        return destination_service_name.empty();
    }
}CCProxy_tag;

typedef struct CCConnect {
    bool IsEmpty() const {
        return true;
    }
}CCConnect_tag;

typedef struct CCCheck {
    bool IsEmpty() const {
        return true;
    }
}CCCheck_tag;

//Weights
typedef struct CCWeights {
    int passing = 1;
    int warning = 1;

    bool IsEmpty() const {
        return true;
    }
}CCWeights_tag;

typedef struct CCService {
    std::string Name;                                                    // 服务名    多个服务可能用同一个名字    （必填）
    std::string ID;                                                          // 唯一ID   为空时默认使用name                (唯一)
    std::vector<std::string> Tags;                               // 标签列表   用于发现服务
    std::string Address;                                                // 服务地址   为空时默认本机地址
    std::map<std::string, std::string> Meta;              // 键值对
    int Port = 0;                                                           // 端口   为空时默认值0
//    std::string Kind;                                                      // 服务类型   默认为空   也可以是 "connect-proxy"
    ////std::string ProxyDestination                              // 已弃用   1.3.0起  目前版本1.4.4
//    CCProxy Proxy;                                                      // 代理
//    CCConnect Connect;                                             // 连接
//    CCCheck Check;                                                    // 检测
//    std::vector<CCCheck> Checks;                            // 检测列表
//    bool EnableTagOverride = false;                         // 标签是否可以被修改
//    CCWeights Weights;                                            // 权重

    bool IsEmpty() const {
        return ID.empty() && Name.empty();
    }
}CCService_tag;

class ConsulController
{
public:
    enum CCCommandStatus {
        CCCommandStatus_OK = 0,
        CCCommandStatus_FAILED
    };

    enum CCStatus {
        CCSTATUS_OK = 0,
        CCSTATUS_FAILED,
        CCSTATUS__INVALIDPARAM,
        CCSTATUS_INITFAILED,
        CCSTATUS__GETFAILD,
        CCSTATUS__POSTFAILD,
    };

public:
    static ConsulController& GetInstance()
    {
        static ConsulController instance; // Guaranteed to be destroyed.
                              // Instantiated on first use.
        return instance;
    }

public:
    CCCommandStatus GetAllService(std::vector<CCService>& service_vec);
    CCCommandStatus GetOneService(CCService& service, const std::string& service_id);

    CCCommandStatus RegisterService(const CCService& service);
    CCCommandStatus DeregisterService(const std::string& service_id);

public:
    static bool JsonToService(CCService& service, const QJsonObject& json_obj);
    static bool ServiceToJson(QJsonObject& json_obj, const CCService& service);
    
public:
    CCStatus DoGet(CCResponse& response, const CCRequest& request);
    CCStatus DoPost(CCResponse& response, const CCRequest& request);
    // 对比post  幂等   即多次请求等效  post倾向于每个请求产生一个不同的对象   put作用于同一对象
    CCStatus DoPut(CCResponse& response, const CCRequest& request);

    bool Init();
    void Uninit();

protected:
    ConsulController();
    ~ConsulController();

};

}

#endif // CONSULCONTROLLER_H
